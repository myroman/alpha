#include "unp.h"
#include "debug.h"
#include "oapi.h"
#include "odrProc.h"
#include "misc.h"
#include "hw_addrs.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

SockAddrUn servaddr;
NetworkInterface* ifHead = NULL;
char* callbackClientName = NULL;
//TODO: use prhwaddrs to use arbitrary MAC	
unsigned char roman_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*our MAC address*/		
int newClientPortNumber = 1024;//seed
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

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
		printf("Interface #%d, IP:%s, HW_addr:", ptr->interfaceIndex, ptr->ipAddr, ptr->macAddress);
		int i=0;
		for(i=0;i<6;++i) {
			printf("%.2x:", ptr->macAddress[i]);
		}
		printf("\n");
	}

	pthread_t unixDmnListener, networkListener;
	pthread_create(&unixDmnListener, NULL, (void *)&respondToHostRequestsRoutine, (void*)unixDomainFd);
	pthread_create(&networkListener, NULL, (void *)&respondToNetworkRequestsRoutine, (void*)unixDomainFd);

	pthread_join(unixDmnListener, NULL);
	pthread_join(networkListener, NULL);	
	
	unlink(ODR_UNIX_PATH);
	printf("ODR terminated\n");
}

void* respondToHostRequestsRoutine (void *arg) {
	socklen_t clilen;
	
	int unixDomainFd = (int)arg;	
	char* buffer = malloc(MAXLINE);
	SockAddrUn senderAddr;
	for(;;) {
		printf("%s Waiting from %d...\n", ut(), unixDomainFd);
		int l = sizeof(senderAddr);
		int length = recvfrom(unixDomainFd, buffer, MAXLINE, 0, (SockAddrUn *)&senderAddr, &l);

		//Parse string
		SendDto* dto = malloc(sizeof(SendDto));
		int res = deserializeApiReq(buffer, length, dto);		
		if (addCurrentNodeAddressAsSource(dto) == 0) {
			continue;
		}

		if (dto->msgType == CLIENT_MSG_TYPE) {
			strcpy(callbackClientName, senderAddr.sun_path);			
			
			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), dto->msg, senderAddr.sun_path);						
		}
		//src_mac and dest mac should be used in another way!
		//add current
		odrSend(dto, roman_mac, roman_mac);
	}	

	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int sockfd, 
		srcPort, 
		unixDomainFd = (int)arg;
	
	if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    printf("%s Socket failed ", nt());
	    exit (EXIT_FAILURE);
	}
	int z = 0, n = 1;
	for(;;) {
		if (n != 0) {
			printf("%s waiting for PF_PACKET...\n", nt());
		}
		FrameUserData* userData = malloc(sizeof(FrameUserData));
		n = odrRecv(sockfd, userData);
		if (n == 0) {

			//continue;
		}
		// find out if it's from client or from server
		printf("%s got a message: %s from IP %s, port %d \n", nt(), userData->msg, userData->srcIpAddr, userData->srcPortNumber);

		// compare dest IP from request and the node's current IP
		int atDestination = atDestination = (strcmp(ifHead->ipAddr, userData->ipAddr) == 0);
		if (atDestination == 0) {
			printf("%s We're at intermediate node with IP=%s", nt(), userData->ipAddr);
			odrSend(userData, roman_mac, roman_mac);
		} else {
			printf("%s We're at dest node with IP=%s\n", nt(), userData->ipAddr);		
			// check if it is request to server
			handlePacketAtDestinationNode(userData, unixDomainFd);
		}
		//TODO: remove later. added to avoid too much cycles if sth works wrong
		if (z++ == 4) {
			printf("Exit\n");
			return;
		}		
	}
}

char* nt() {
	return "Network thread:";
}

char* ut() {
	return "Unix thread:";
}

int deserializeApiReq(char* buffer, size_t bufLen, SendDto* dto) {
	char* s = malloc(bufLen);
	strcpy(s, buffer);

	char *tok = NULL, *delim = "|";
    int len = 0, member = 0;      	
    tok = strtok(s, delim);
	while (tok) {
    	switch(member++){
        	case 0:
        		dto->msgType = atoi(tok);
        		break;
			case 1:
				strcpy(dto->destIp, tok);
				break;
			case 2:
				dto->destPort = atoi(tok);
				break;
			case 3:
				dto->msg = malloc(strlen(tok));
				strcpy(dto->msg, tok);
				break;
			case 4:
				dto->forceRedisc = atoi(tok);
				break;
        }
        tok = strtok(NULL, delim);
    } 
    return 1;
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
		if (niPtr != ifHead) {
			niPtr->next = malloc(sizeof(NetworkInterface));
			niPtr = niPtr->next;
		}

		if (strcmp(hwa->if_name, "eth0") == 0) {
			niPtr->isEth0 = 1;
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
		if (p->isEth0) {
			return p;
		}
		p = p->next;
	}

	return NULL;
}

int addCurrentNodeAddressAsSource(SendDto* dto) {
	pthread_mutex_lock(&lock);

	NetworkInterface* nodeIf = getCurrentNodeInterface();
	if (nodeIf == NULL) {
		printf("%s Error: client node doesn't have eth0\n", ut());
		return 0;
	}
	strcpy(dto->srcIp, nodeIf->ipAddr);	
	dto->srcPort = newClientPortNumber++;
	
	pthread_mutex_unlock(&lock);
	return 1;
}

void handlePacketAtDestinationNode(FrameUserData* userData, int unixDomainFd) {	
	SockAddrUn appAddr;

	printf("%s Port number:%d\n", nt(), userData->portNumber);
	if (userData->portNumber == SRV_PORT_NUMBER) {
		appAddr = createSockAddrUn(SRV_UNIX_PATH);
		printf("%s Sending to a server Unix file %s\n", nt(), appAddr.sun_path);
		int x = sendto(unixDomainFd, userData->msg, strlen(userData->msg), 0, (SockAddrUn*)&appAddr, sizeof(appAddr));
		printf("Sent result = %d\n", x);
	} else {
		if (callbackClientName != NULL) {
			appAddr = createSockAddrUn(callbackClientName);			
			printf("%s Sending to a client Unix file %s, %s\n", nt(), appAddr.sun_path, userData->msg);
			sendto(unixDomainFd, userData->msg, strlen(userData->msg), 0, (SockAddrUn *)&appAddr, sizeof(appAddr));
		} else{
			printf("Callback filename for client is NULL\n");
		}
	}
}