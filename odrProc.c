#include "unp.h"
#include "debug.h"
#include "oapi.h"
#include "odrProc.h"
#include "misc.h"
#include "hw_addrs.h"
#include "payloadHdr.h"
#include "routingTable.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include "portPath.h"

SockAddrUn servaddr;
NetworkInterface* ifHead = NULL;
char* callbackClientName = NULL;
int hack = 1;
RouteEntry *headEntry = NULL;
RouteEntry *tailEntry = NULL;
PortPath *headEntryPort = NULL;
PortPath *tailEntryPort = NULL;
int newClientPortNumber = 1024;//seed
int STALENESS;
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
	
	if(argc != 2){
		printf("Staleness Value was not given.. Setting it to 5 by default.");
		STALENESS = 5;
	}
	else{
		STALENESS = atoi(argv[1]);
		debug("Staleness %d %s", STALENESS, argv[1]);
	}
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
	
	addPortPath(SRV_UNIX_PATH, SRV_PORT_NUMBER, 0, 1, &headEntryPort, &tailEntryPort);
	printPortTable(&headEntryPort, &tailEntryPort);


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
		PortPath * check = findAndUpdatePath(senderAddr.sun_path, &headEntryPort, &tailEntryPort);
		if(check == NULL){
			debug("\n\nService not present in Table. Adding\n\n");
			addPortPath(senderAddr.sun_path, newClientPortNumber, 0, 0, &headEntryPort, &tailEntryPort);
			newClientPortNumber++;
			printPortTable(&headEntryPort, &tailEntryPort);
		}
		else{
			debug("Service was already present in the Table. Not adding.");
			printPortTable(&headEntryPort, &tailEntryPort);
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
			payload.srcPort = (findAndUpdatePath(senderAddr.sun_path, &headEntryPort, &tailEntryPort))->port_number; //newClientPortNumber++;

			strcpy(callbackClientName, senderAddr.sun_path);						
			//debug("\n\nClient Msg: %d\n\n", payload.msgType);

			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			//debug("\n\nServer Msg: %d\n\n", payload.msgType);
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), payload.msg, senderAddr.sun_path);						
		}	
		payload.msgType = 2;//Fogot to set MsgType field to MSG payload	
		// Let's lock to show consistent output		
		odrSend(odrSockFd, payload, currentNode->macAddress, currentNode->macAddress, currentNode->interfaceIndex);		
	}
	free(buffer);
	close(odrSockFd);
	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int srcPort, rawSockFd,
		unixDomainFd = (intptr_t)arg;//,
	//if(hack == 1){
		rawSockFd = createOdrSocket();
		//hack++;
	//}
		
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
	SockAddrLl senderAddr;	
	int sz = sizeof(senderAddr);
	int n = 1; //flag handling bad incoming packets
	for(;;) {
		if (n != -1) {
			printf("%s waiting for PF_PACKETs..\n", nt());
		}		

		// Receive PF_PACKET
		int sz = sizeof(struct sockaddr_ll);
		bzero(buffer, ETH_FRAME_LEN);
		n = recvfrom(rawSockFd, buffer, ETH_FRAME_LEN, 0, (SA *)&senderAddr, &sz);
		if (n == -1 || ntohs(senderAddr.sll_protocol) != PROTOCOL_NUMBER) {
			n = -1;
			continue;
		}
		debug("\nODR: Received PF_PACKET, length=%d", n);
		unpackPayload(buffer + 14, &ph);
		
		lockm();
		handleIncomingPacket(rawSockFd, unixDomainFd, ph, currentNode, senderAddr);		
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
		} while (++i < ETH_ALEN);

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
		if (strcmp(hwa->if_name, "lo") == 0) {
			niPtr->isLo = 1;
		}

		char* s = Sock_ntop_host(sa, sizeof(*sa));
		niPtr->ipAddr = inet_addr(s);

		if (prflag) {
			ptr = hwa->if_haddr;
			i = ETH_ALEN;
			int j;
			do {
				j = ETH_ALEN - i;
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
void handlePacketAtDestinationNode(int unixDomainFd, PayloadHdr* ph) {	
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

void handleIncomingPacket(int rawSockFd, int unixDomainFd, PayloadHdr ph, NetworkInterface* currentNode, SockAddrLl sndAddr) {
	printf("%s got a packet message: MsgType %d: %s from %s:%d to %s:%d \n", nt(), ph.msgType, ph.msg, printIPHuman(ph.srcIp), ph.srcPort, printIPHuman(ph.destIp), ph.destPort);
	RouteEntry* destEntry = findRouteEntry(ph.destIp, &headEntry, &tailEntry);
	// Don't increase hop count if
	//1) the request comes from the same host
	//2)if comes RREP, and we don't have destination route (it's exceptional case)
	int weDontKnowRREP = (ph.msgType == MT_RREP) && (destEntry == NULL);
	if (currentNode->ipAddr != ph.srcIp) {
		ph.hopCount++;
	}
	// Insert/update reverse path entry
	insertOrUpdateRouteEntry(ph.srcIp, sndAddr.sll_addr, ph.hopCount, sndAddr.sll_ifindex, &headEntry, &tailEntry);
	RouteEntry* srcEntry = findRouteEntry(ph.srcIp, &headEntry, &tailEntry);
	
	printf("%s MsgType:%d, from %s:%d \n",nt(), ph.msgType, printIPHuman(ph.srcIp), ph.srcPort);
	// It's a RREQ
	if (ph.msgType == MT_RREQ) {
		// At intermediate node
		if (ph.destIp != currentNode->ipAddr) {
			if (destEntry != NULL) {	
				printf("%s INT.NODE: RREQ received. I got a route, so RREP back and propagate\n", nt());

				// RREP back and propagate
				PayloadHdr respHdr = convertToResponse(ph);
				rrepBack(rawSockFd, respHdr, currentNode->macAddress, sndAddr);
				sendToRoute(rawSockFd, ph, currentNode->macAddress, *destEntry);
			}
			// Flood if we don't know the route
			else {
				printf("%s INT.NODE: RREQ received. Don't have a route, so flood\n", nt());
				flood(rawSockFd, ph, currentNode->macAddress, sndAddr);
			}
		} 
		// At destination
		else {		  
			printf("%s DEST.NODE: RREQ received. So send RREP\n", nt());
			PayloadHdr respHdr = convertToResponse(ph);	   
			respHdr.hopCount = 0;
			rrepBack(rawSockFd, respHdr, currentNode->macAddress, sndAddr);
		}
	}

	// It's a RREP
	else if (ph.msgType == MT_RREP) {
		// At intermediate node
		if (ph.destIp != currentNode->ipAddr) {
			// disregard it - we don't have a reverse path for it
			if (destEntry == NULL) {
				return;
			}
			printf("%s INT.NODE: RREP received. Send RREP back to the REQQuestor\n", nt());
			sendToRoute(rawSockFd, ph, currentNode->macAddress, *destEntry);
		}
		// At destination
		else {
			printf("%s DEST.NODE: RREP received. Send Hi!\n", nt());
			PayloadHdr msg = convertToResponse(ph);
			msg.hopCount = 0;
			strcpy(msg.msg, "Hi!\0");
			sendToRoute(rawSockFd, msg, currentNode->macAddress, *srcEntry);
		}
	}
	// It's an APP payload packet
	else {
		// At intermediate node
		if (ph.destIp != currentNode->ipAddr) {
			if (destEntry != NULL) {		
				printf("%s INT.NODE: Forward message:%s\n", nt(), ph.msg);
				sendToRoute(rawSockFd, ph, currentNode->macAddress, *destEntry);
			}			
		}		
		else{
			//We're at destination, get UNIX file by port and send payload to the process
			printf("%s DEST.NODE: Got a payload message: %s. Pushing it up to the process...\n", nt(), ph.msg);
			handlePacketAtDestinationNode(unixDomainFd, &ph);
		}
	}
}

void rrepBack(int rawSockFd, PayloadHdr respH, unsigned char currentMac[ETH_ALEN], SockAddrLl recvAddr) {
	odrSend(rawSockFd, respH, currentMac, recvAddr.sll_addr, recvAddr.sll_ifindex);
}

void sendToRoute(int rawSockFd, PayloadHdr ph, unsigned char currentMac[ETH_ALEN], RouteEntry destEntry) {
	odrSend(rawSockFd, ph, currentMac, destEntry.next_hop, destEntry.interfaceInd);
}

void flood(int rawSockFd, PayloadHdr ph, unsigned char currentMac[ETH_ALEN], SockAddrLl senderAddr) {
	unsigned char toAll[ETH_ALEN];

	//Send to all interfaces except incoming interface, eth0 and lo.
	NetworkInterface* ptr;
	for(ptr = ifHead;ptr != NULL;ptr = ptr->next) {
		if (senderAddr.sll_ifindex == ptr->interfaceIndex) {
			continue;
		}
		if (ptr->isEth0 == 1 || ptr->isLo == 1) {
			continue;
		}

		odrSend(rawSockFd, ph, currentMac, toAll, ptr->interfaceIndex);
	}	
}