#include "unp.h"
#include "debug.h"
#include "oapi.h"
#include "odrProc.h"
#include "misc.h"
#include "hw_addrs.h"
#include "payloadHdr.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

SockAddrUn servaddr;
NetworkInterface* ifHead = NULL;
char* callbackClientName = NULL;
int newClientPortNumber = 1024;//seed
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void lockm() {
	pthread_mutex_lock(&lock);
}

void unlockm() {
	pthread_mutex_unlock(&lock);
}

/* Listen to: 1) UNIX domain sockets for messages from client/server 
2) PF_PACKETs from another ODR
1) When received UNIX - deserialize the data into arguments which they sent: for msg_send or msg_recv.
Then call a function from odrImpl.c
2) When you get a response from ODR, serialize it and send back to the needed socket	*/
int main(int argc, char **argv) {
	
	int unixDomainFd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(ODR_UNIX_PATH);
	servaddr = createSockAddrUn(ODR_UNIX_PATH);	
	bind(unixDomainFd, (SA *) &servaddr, sizeof(servaddr));	
	callbackClientName = malloc(50);
	fillInterfaces();

	NetworkInterface* ptr = ifHead;
	
	printf("*** List of interfaces ***\n");
	for(;ptr != NULL;ptr = ptr->next) {
		printf("Interface #%d, IP:%s, eth0:%d,HW_addr:", ptr->interfaceIndex, printIPHuman(ptr->ipAddr), ptr->isEth0);
		int i=0;
		for(i=0;i < 6; ++i) {
			printf("%.2x:", ptr->macAddress[i]);
		}
		printf("\n");
	}

	pthread_t unixDmnListener, networkListener;
	pthread_create(&unixDmnListener, NULL, (void *)&respondToHostRequestsRoutine, (void*)(intptr_t)unixDomainFd);
	pthread_create(&networkListener, NULL, (void *)&respondToNetworkRequestsRoutine, (void*)(intptr_t)unixDomainFd);

	pthread_join(unixDmnListener, NULL);
	pthread_join(networkListener, NULL);	
	
	unlink(ODR_UNIX_PATH);
	printf("ODR terminated\n");
	free(callbackClientName);
	free(ifHead);
	close(unixDomainFd);
}

void* respondToHostRequestsRoutine (void *arg) {	
	int unixDomainFd = (intptr_t)arg,
		odrSockFd = createOdrSocket();	
	void* buffer = malloc(MAXLINE);//change to 1500 or less
	SockAddrUn senderAddr;
	PayloadHdr payload;
	if (odrSockFd == -1) {
		return;
	}
	for(;;) {
		printf("%s Waiting from UNIX socket...\n", ut());
		int l = sizeof(struct sockaddr_un), length;
		if ((length = recvfrom(unixDomainFd, buffer, MAXLINE, 0, (SA *)&senderAddr, &l)) == -1) {			
			continue;
		}
		debug("%s Received buffer, length=%d", ut(), length);

		unpackPayload(buffer, &payload);	
		//print info
		printf("%s", ut());
		printPayloadContents(&payload);

		lockm();		
		NetworkInterface* currentNode = getCurrentNodeInterface();
		if (currentNode == NULL) {
			debug("Current node if is NULL.Exit");
			unlockm();
			free(buffer);
			return;
		}						
		unlockm();

		payload.srcIp = currentNode->ipAddr;				
		// we check the passcode in case client choose 'loc'
		if (payload.destIp == LOCAL_INET_IP){
			debug("DestIP is local!");
			payload.destIp = currentNode->ipAddr;
		}

		if (payload.msgType == CLIENT_MSG_TYPE) {
			payload.srcPort = newClientPortNumber++;

			strcpy(callbackClientName, senderAddr.sun_path);						
			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), payload.msg, senderAddr.sun_path);						
		}		
		// Let's lock to show consistent output		
		odrSend(odrSockFd, &payload, currentNode->macAddress, currentNode->macAddress, currentNode->interfaceIndex);		
	}
	free(buffer);
	close(odrSockFd);
	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int srcPort, 
		unixDomainFd = (intptr_t)arg,
		rawSockFd = createOdrSocket();	
	if (rawSockFd == -1) {
		return;
	}
	
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	
	lockm();
	NetworkInterface* currentNode = getCurrentNodeInterface();	
	unlockm();

	if (currentNode == NULL) {
		printf("Current node is null, return\n");
		return;
	}

	PayloadHdr ph;
	int n = 1; //flag handling bad incoming packets
	for(;;) {
		if (n != 0) {
			printf("%s waiting for PF_PACKETs..\n", nt());
		}
		
		if ((n = odrRecv(rawSockFd, &ph, buffer)) == 0) {
			continue;
		}
		
		lockm();
		printf("%s got a packet message: %s from %s:%d to %s:%d \n", nt(), ph.msg, printIPHuman(ph.srcIp), ph.srcPort, printIPHuman(ph.destIp), ph.destPort);
		
		// compare dest IP from request and the node's current IP
		//TODO: USE inet IP for comparison
		int atDestination = currentNode->ipAddr == ph.destIp;
		if (atDestination == 0) {
			printf("%s We're at intermediate node with IP=%s", nt(), printIPHuman(ph.destIp));
		} else {
			printf("%s We're at destination node with IP=%s\n", nt(), printIPHuman(ph.destIp));
			// check if it is request to server
			handlePacketAtDestinationNode(&ph, unixDomainFd);
		}
		unlockm();
	}
	free(buffer);
	close(rawSockFd);
}

char* nt() {
	return "Network thread:";
}

char* ut() {
	return "Unix thread:";
}

void fillInterfaces() {
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag;

	ifHead = malloc(sizeof(NetworkInterface));
	NetworkInterface* niPtr = ifHead;
	niPtr->next = NULL;
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		// At this point we understand, that current interface has useful info
		if (hwa != hwahead) {
			niPtr->next = malloc(sizeof(NetworkInterface));
			niPtr = niPtr->next;
			niPtr->next = NULL;
		}

		if ((strcmp(hwa->if_name, "eth0") == 0)||(strcmp(hwa->if_name, "wlan0") == 0)) {
			niPtr->isEth0 = 1;
			printf("Etho is found\n");
		}

		char* s = Sock_ntop_host(sa, sizeof(*sa));
		niPtr->ipAddr = inet_addr(s);

		if (prflag) {
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			int j;
			do {
				j = IF_HADDR - i;
				niPtr->macAddress[j] = *ptr++ & 0xFF;
			} while (--i > 0);

		}

		niPtr->interfaceIndex = hwa->if_index;
	}

	free_hwa_info(hwahead);
	return;
}

NetworkInterface* getCurrentNodeInterface() {
	if (ifHead == NULL) {
		return NULL;
	}
	NetworkInterface* p = ifHead;
	while(p != NULL) {
		if (p->isEth0 == 1) {
			return p;
		}
		p = p->next;
	}
	debug("getCurrentNodeInterface is null");
	return NULL;
}

// Handles scenario when the packet arrived at destination
// 2 basic cases: we're at server or client node.
void handlePacketAtDestinationNode(PayloadHdr* ph, int unixDomainFd) {	
	SockAddrUn appAddr;
	void* buf;
	int bufLen, res;

	printf("%s Port number:%d\n", nt(), ph->destPort);
	if (ph->destPort == SRV_PORT_NUMBER) {
		appAddr = createSockAddrUn(SRV_UNIX_PATH);
		
		buf = packPayload(ph, &bufLen);

		printf("%s:Sending to a server UNIX file %s the buffer message %s...", ut(), appAddr.sun_path, ph->msg);
		if ((res = sendto(unixDomainFd, buf, bufLen, 0, (SA *)&appAddr, sizeof(appAddr))) == -1) {
			printFailed();
			free(buf);
			return;
		} 
		free(buf);
		printOK();		
		return;
	}

	if (callbackClientName != NULL) {
		appAddr = createSockAddrUn(callbackClientName);			
		buf = packPayload(ph, &bufLen);
		
		printf("%s Sending to a client Unix file %s message %s...", ut(), appAddr.sun_path, ph->msg);		
		if ((res = sendto(unixDomainFd, buf, bufLen, 0, (SA *)&appAddr, sizeof(appAddr))) == -1) {
			printFailed();
			free(buf);
			return;
		}
		free(buf);		
		printOK();
		return;
	}
	printf("Callback filename for client is NULL\n");	
}

int createOdrSocket() {
	int sd;
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    printf("Creating ODR socket failed\n");
	    return -1;
	}
	return sd;
}