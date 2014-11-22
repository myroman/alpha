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

// 2 MACs of Vm1 and Vm2
//00:0c:29:49:3f:5b vm1
//00:0c:29:d9:08:ec vm2

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
		printf("Interface #%d, IP:%s, eth0:%d,HW_addr:", ptr->interfaceIndex, ptr->ipAddr, ptr->isEth0);
		int i=0;
		for(i=0;i<6;++i) {
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
}

void* respondToHostRequestsRoutine (void *arg) {
	socklen_t clilen;
	
	int unixDomainFd = (intptr_t)arg;	
	void* buffer = malloc(MAXLINE);//change to 1500 or less
	SockAddrUn senderAddr;
	PayloadHdr payload;
	
	for(;;) {
		printf("%s Waiting from %d...\n", ut(), unixDomainFd);
		int l = sizeof(senderAddr), length;
		if ((length = recvfrom(unixDomainFd, buffer, MAXLINE, 0, (SA *)&senderAddr, &l)) == -1) {			
			continue;
		}
		lockm();
		debug("%s Received buffer, length=%d", ut(), length);

		unpackPayload(buffer, &payload);	
		debug("h");	
		
		debug("h");
		//print info
		printf("%s", ut());
		printPayloadContents(&payload);

		if (addCurrentNodeAddressAsSource(&payload) == 0) {			
			unlockm();
			continue;
		}

		handleLocalDestMode(&payload);		

		if (payload.msgType == CLIENT_MSG_TYPE) {
			strcpy(callbackClientName, senderAddr.sun_path);			
			
			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), payload.msg, senderAddr.sun_path);						
		}
		
		// Let's lock to show consistent output
		
		NetworkInterface* currentNode = getCurrentNodeInterface();
		if (currentNode == NULL) {
			debug("Current node if is NULL.Exit");
			unlockm();
			free(buffer);
			return;
		}					
		odrSend(&payload, currentNode->macAddress, currentNode->macAddress, currentNode->interfaceIndex);		
		unlockm();
	}
	free(buffer);
	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int srcPort, 
		unixDomainFd = (intptr_t)arg;	
	
	int z = 0;
	//pthread_mutex_lock(&lock);
	NetworkInterface* currentNode = getCurrentNodeInterface();	
	//pthread_mutex_unlock(&lock);
	if (currentNode == NULL) {
		printf("Current node is null, return\n");
		return;
	}
	PayloadHdr ph;
	int n = 1; //flag handling bad incoming packets
	for(;;) {
		if (n != 0) {
			printf("%s waiting for PF_PACKET..\n", nt());
		}
		
		n = odrRecv(&ph);
		if (n == 0) {
			continue;
		}
		lockm();
		n = 1;
		printf("%s got a packet message: %s from %s:%d to %s:%d \n", nt(), ph.msg, printIPHuman(ph.srcIp), ph.srcPort, printIPHuman(ph.destIp), ph.destPort);
		
		// compare dest IP from request and the node's current IP
		//TODO: USE inet IP for comparison
		int atDestination = atDestination = (strcmp(currentNode->ipAddr, printIPHuman(ph.destIp)) == 0);
		if (atDestination == 0) {
			printf("%s We're at intermediate node with IP=%s", nt(), printIPHuman(ph.destIp));
		} else {
			printf("%s We're at dest node with IP=%s\n", nt(), printIPHuman(ph.destIp));
			// check if it is request to server
			handlePacketAtDestinationNode(&ph, unixDomainFd);
		}
		unlockm();
	}
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

	//unsigned char dest_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*other host MAC address*/
	ifHead = malloc(sizeof(NetworkInterface));
	NetworkInterface* niPtr = ifHead;

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		/*
		if( (strcmp(hwa->if_name, "lo") ==0 ) || (strcmp(hwa->if_name, "eth0") == 0) ){
			continue;
		}
*/

//		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");		
		if ( (sa = hwa->ip_addr) != NULL)
//			printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));				
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
		}

		if ((strcmp(hwa->if_name, "eth0") == 0)||(strcmp(hwa->if_name, "wlan0") == 0)) {
			niPtr->isEth0 = 1;
			printf("Etho is found\n");
		}
		strcpy(niPtr->ipAddr, Sock_ntop_host(sa, sizeof(*sa)));

		if (prflag) {
//			printf("         HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			int j;
			do {
				j = IF_HADDR - i;
				niPtr->macAddress[j] = *ptr++ & 0xff;

//				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);

		}

		niPtr->interfaceIndex = hwa->if_index;
//		printf("\n         interface index = %d\n\n", hwa->if_index);
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

// We fill srcPort from eth0 interface
int addCurrentNodeAddressAsSource(PayloadHdr* ph) {
	//pthread_mutex_lock(&lock);

	NetworkInterface* nodeIf = getCurrentNodeInterface();
	if (nodeIf == NULL) {
		printf("%s Error: client node doesn't have eth0\n", ut());
		
		return 0;
	}
	ph->srcIp = inet_addr(nodeIf->ipAddr);
	ph->srcPort = newClientPortNumber++;
	
	//pthread_mutex_unlock(&lock);
	return 1;
}


int handleLocalDestMode(PayloadHdr* ph) {
	//pthread_mutex_lock(&lock);

	NetworkInterface* nodeIf = getCurrentNodeInterface();
	if (nodeIf == NULL) {
		printf("%s Error: client node doesn't have eth0\n", ut());
		return 0;
	}
	// we check the passcode
	if (ph->destIp == LOCAL_INET_IP){
		debug("DestIP is local!");
		ph->destIp = inet_addr(nodeIf->ipAddr);		
	}
	
	//pthread_mutex_unlock(&lock);
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

		//pthread_mutex_lock(&lock);
		printf("%s:Sending to a server UNIX file %s the buffer message %s...", ut(), appAddr.sun_path, ph->msg);
		if ((res = sendto(unixDomainFd, buf, bufLen, 0, (SA *)&appAddr, sizeof(appAddr))) == -1) {
			printFailed();
			//pthread_mutex_unlock(&lock);
			free(buf);
			return;
		} 
		printOK();
		free(buf);
		//pthread_mutex_unlock(&lock);
		
		return;
	}

	if (callbackClientName != NULL) {
		appAddr = createSockAddrUn(callbackClientName);			
		buf = packPayload(ph, &bufLen);

		//pthread_mutex_lock(&lock);
		printf("%s Sending to a client Unix file %s message %s...", ut(), appAddr.sun_path, ph->msg);		
		if ((res = sendto(unixDomainFd, buf, bufLen, 0, (SA *)&appAddr, sizeof(appAddr))) == -1) {
			printFailed();
			//pthread_mutex_unlock(&lock);
			free(buf);
			return;
		}
		printOK();
		free(buf);		
		//pthread_mutex_unlock(&lock);
		return;
	}
	printf("Callback filename for client is NULL\n");	
}