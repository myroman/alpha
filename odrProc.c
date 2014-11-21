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
}

void* respondToHostRequestsRoutine (void *arg) {
	socklen_t clilen;
	
	int unixDomainFd = (intptr_t)arg;	
	char* buffer = malloc(MAXLINE);
	SockAddrUn senderAddr;
	for(;;) {
		printf("%s Waiting from %d...\n", ut(), unixDomainFd);
		int l = sizeof(senderAddr);
		int length = recvfrom(unixDomainFd, buffer, MAXLINE, 0, (SA *)&senderAddr, &l);

		//Parse string
		SendDto* dto = malloc(sizeof(SendDto));
		int res = deserializeApiReq(buffer, length, dto);		
		if (addCurrentNodeAddressAsSource(dto) == 0) {
			continue;
		}
		handleLocalDestMode(dto);
		printf("%s Got a msg: type %d, src:%s:%d, dest:%s:%d\n", ut(), dto->msgType, dto->srcIp, dto->srcPort, dto->destIp, dto->destPort);

		if (dto->msgType == CLIENT_MSG_TYPE) {
			strcpy(callbackClientName, senderAddr.sun_path);			
			
			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), dto->msg, senderAddr.sun_path);						
		}
		
		// TODO: probably we don't have to lock it because it's reading.
		pthread_mutex_lock(&lock);
		NetworkInterface* currentNode = getCurrentNodeInterface();
		if (currentNode == NULL) {
			debug("Currenet node if is NULL.Exit");
			return;
		}
		pthread_mutex_unlock(&lock);
		debug("%s Gonna send", ut());
		odrSend(dto, currentNode->macAddress, currentNode->macAddress, currentNode->interfaceIndex);
	}	

	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int sockfd, 
		srcPort, 
		unixDomainFd = (intptr_t)arg;
	
	if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    printf("%s Socket failed ", nt());
	    exit (EXIT_FAILURE);
	}
	int z = 0, n = 1;
	pthread_mutex_lock(&lock);
	NetworkInterface* currentNode = getCurrentNodeInterface();	
	pthread_mutex_unlock(&lock);
	if (currentNode == NULL) {
		printf("Currnode is null, return");
		return;
	}
	for(;;) {
		if (n != 0) {
			printf("%s waiting for PF_PACKET...\n", nt());
		}
		FrameUserData* userData = malloc(sizeof(FrameUserData));
		n = odrRecv(sockfd, userData);
		if (n == 0) {
			continue;
		}
		printf("%s got a frame message: %s from %s:%d to %s:%d \n", nt(), userData->msg, userData->srcIpAddr, userData->srcPortNumber, userData->ipAddr, userData->portNumber);

		// compare dest IP from request and the node's current IP
		int atDestination = atDestination = (strcmp(currentNode->ipAddr, userData->ipAddr) == 0);
		if (atDestination == 0) {
			printf("%s We're at intermediate node with IP=%s", nt(), userData->ipAddr);
			//odrSend(&dto, currentNode->macAddress, currentNode->macAddress, currentNode->interfaceIndex);
		} else {
			printf("%s We're at dest node with IP=%s\n", nt(), userData->ipAddr);		
			// check if it is request to server
			handlePacketAtDestinationNode(userData, unixDomainFd);
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

int handleLocalDestMode(SendDto* dto) {
	pthread_mutex_lock(&lock);

	NetworkInterface* nodeIf = getCurrentNodeInterface();
	if (nodeIf == NULL) {
		printf("%s Error: client node doesn't have eth0\n", ut());
		return 0;
	}
	if (strcmp(dto->destIp, "loc") == 0) {
		strcpy(dto->destIp, nodeIf->ipAddr);
	}
	
	pthread_mutex_unlock(&lock);
}

void handlePacketAtDestinationNode(FrameUserData* userData, int unixDomainFd) {	
	SockAddrUn appAddr;
	char* buf = malloc(ETHFR_MAXDATA_LEN);
	printf("%s Port number:%d\n", nt(), userData->portNumber);
	if (userData->portNumber == SRV_PORT_NUMBER) {
		appAddr = createSockAddrUn(SRV_UNIX_PATH);
		printf("%s Sending to a server Unix file %s\n", nt(), appAddr.sun_path);
		
		serializeServerDto(*userData, buf);
		debug("NETWORK:Sending to server buf=%s, l=%d", buf, (int)strlen(buf));
		int x = sendto(unixDomainFd, buf, strlen(buf), 0, (SA *)&appAddr, sizeof(appAddr));
		printf("Sent result = %d\n", x);
		if(x == -1) {
			printf("Err: %s\n", strerror(errno));
		}
	} else {
		if (callbackClientName != NULL) {
			appAddr = createSockAddrUn(callbackClientName);			
			printf("%s Sending to a client Unix file %s, %s\n", nt(), appAddr.sun_path, userData->msg);
			serializeServerDto(*userData, buf);
			debug("NETWORK:Sending to client buf=%s", buf);
			sendto(unixDomainFd, buf, strlen(buf), 0, (SA *)&appAddr, sizeof(appAddr));
		} else{
			printf("Callback filename for client is NULL\n");
		}
	}
}

void serializeServerDto(FrameUserData dto, char* out) {
	char* ptrPaste = out;
	char* srcPortRaw = itostr2(dto.srcPortNumber);	
	char* forceRed = itostr2(0);//add later to FrameUserData	
	char* msgType;
	if (dto.portNumber == SRV_PORT_NUMBER) {
		msgType = itostr2(CLIENT_MSG_TYPE);
	} else {
		msgType = itostr2(SRV_MSG_TYPE);
	}
	ptrPaste = cpyAndMovePtr2(ptrPaste, msgType);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, dto.srcIpAddr);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, srcPortRaw);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, dto.msg);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, forceRed);
	ptrPaste = cpyAndMovePtr2(ptrPaste, "\0");
	free(srcPortRaw);
	free(forceRed);
	free(msgType);
}