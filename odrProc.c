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
#include "bidTable.h"

SockAddrUn servaddr;
NetworkInterface* ifHead = NULL;
char* callbackClientName = NULL;
int hack = 1;
RouteEntry *headEntry = NULL;
RouteEntry *tailEntry = NULL;
PortPath *headEntryPort = NULL;
PortPath *tailEntryPort = NULL;
BidEntry *headBid = NULL;
BidEntry *tailBid = NULL;
int newClientPortNumber = 1024;//seed
int maxRawSd = 0;
int STALENESS;
int bid = 1;
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
	int odrSockFd = createOdrSocket();
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
	//close(odrSockFd);
	close(unixDomainFd);
	ptr = ifHead;
	
	for(;ptr != NULL;ptr = ptr->next) {
		close(ptr->sd);
		free(ptr->sockAddr);
	}
}

void* respondToHostRequestsRoutine (void *arg) {	
	int unixDomainFd = (intptr_t)arg;
	int rawSockFd = createOdrSocket();
	if (rawSockFd == -1) {
		debug("bad socket");
		return;
	}
	SockAddrLl ssa;
	bzero(&ssa, sizeof(struct sockaddr_ll));
	bind(rawSockFd, (SockAddrLl*)&ssa, sizeof(struct sockaddr_ll));

	addPortPath(SRV_UNIX_PATH, SRV_PORT_NUMBER, 0, 1, &headEntryPort, &tailEntryPort);
	printPortTable(&headEntryPort, &tailEntryPort);
	
	void* buffer = malloc(MAXLINE);//change to 1500 or less
	SockAddrUn senderAddr;
	PayloadHdr payload;
	
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
		if (payload.destIp == ntohl(LOCAL_INET_IP)){
			debug("DestIP is local!");
			payload.destIp = currentNode->ipAddr;
		}
		
		if (payload.msgType == MT_RREQ) {
			payload.srcPort = (findAndUpdatePath(senderAddr.sun_path, &headEntryPort, &tailEntryPort))->port_number; //newClientPortNumber++;

			strcpy(callbackClientName, senderAddr.sun_path);						
			//debug("\n\nClient Msg: %d\n\n", payload.msgType);

			printf("%s Got a msg from client (filepath=%s)\n", ut(), senderAddr.sun_path);			
		} else {
			//debug("\n\nServer Msg: %d\n\n", payload.msgType);
			printf("%s Got a msg from server '%s' (filepath=%s).\n", ut(), payload.msg, senderAddr.sun_path);						
		}	
		payload.msgType = MT_PLD;//Fogot to set MsgType field to MSG payload	
		// Let's lock to show consistent output		
		SockAddrLl sndAddr;
		bzero(&sndAddr, sizeof(sndAddr));

		lockm();		
		handleIncomingPacket(rawSockFd, unixDomainFd, &payload, currentNode, sndAddr);
		unlockm();
		//unsigned char toAll[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		//sndAddr.sll_ifindex = currentNode->interfaceIndex;
		//flood(rawSockFd, payload, sndAddr);
		//odrSend(rawSockFd, payload, currentNode->macAddress, toAll, currentNode->interfaceIndex);		
	}
	free(buffer);
	close(rawSockFd);
	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int srcPort;
		int unixDomainFd = (intptr_t)arg;
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
	
	fd_set set;
	int maxfd;	
	struct timeval tv;

	for(;;) {
	rep:
		bzero(&tv, sizeof(struct timeval));
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		// set each descriptor
		NetworkInterface* p = ifHead;
		for(p = ifHead;p != NULL;p=p->next) {
			if ((p->isLo) == 0) {
				FD_SET(p->sd, &set);	
			}				
		}

		maxfd = maxRawSd;
		maxfd++;
		if (n != -2) {
			printf("%s waiting for PF_PACKETs..\n", nt());	
		}

		n = select(maxfd, &set, NULL, NULL, &tv);		
		for(p=ifHead;p!=NULL;p=p->next) {
			// don't receive from lo and eth0
			if ((p->isLo|p->isEth0) != 0 || !FD_ISSET(p->sd, &set)) {				
				n = -2;
				continue;
			}
			// Receive PF_PACKET
			int sz = sizeof(struct sockaddr_ll);
			bzero(buffer, ETH_FRAME_LEN);

			n = recvfrom(p->sd, buffer, ETH_FRAME_LEN, 0, (SA *)&senderAddr, &sz);
			FD_CLR(p->sd, &set);
			if (n == -1 || n == 1) {
				n = -2;
				goto rep;
			}
			if (ntohs(senderAddr.sll_protocol) != PROTOCOL_NUMBER) {
				n = -2;
				goto rep;
			}				
			unpackPayload(buffer + 14, &ph);
			// filter out pingback caused by flooding from this host
			if ((ph.msgType == MT_RREQ || ph.msgType == MT_RREP) && findMatchByIp(ph.srcIp) != NULL) {
				printf("Skip flood-back RREQs:\n");					
				n = -2;
				continue;
			}
			if (hasMatch(senderAddr.sll_addr) == 1) {
				printf("Skip equal macs");
				n = -2;
				continue;
			}

			printf("\nODR: Received PF_PACKET on MAC:");
			printMac(p->macAddress);
			printf(" from MAC: ");
			printMac(senderAddr.sll_addr);
			printf(", msg type = %d\n", ph.msgType);

			//, length=%d", n);
			sleep(2);

			lockm();
			handleIncomingPacket(p->sd, unixDomainFd, &ph, p, senderAddr);		
			unlockm();
		}
	}
	free(buffer);
	pthread_exit(0);
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
		niPtr->ipAddr = ntohl(inet_addr(s));

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
		//create sockets for all if except lo, but listen only for those except eth0 and lo
		if ((niPtr->isLo) == 0) {
			niPtr->sd = createOdrSocket();
			//if (niPtr->isEth0 == 0) {
				if (niPtr->sd > maxRawSd) {
					maxRawSd = niPtr->sd;
				}
			//}

			//bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
			niPtr->sockAddr = malloc(sizeof(struct sockaddr_ll));
			bzero(niPtr->sockAddr, sizeof(struct sockaddr_ll));
			//bind(niPtr->sd, (struct sockaddr_ll*)niPtr->sockAddr, sizeof(struct sockaddr_ll));
		}
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
	if ((sd = socket (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
	    printf("Creating ODR socket failed\n");
	    return -1;
	}
	return sd;
}

void handleIncomingPacket(int rawSockFd, int unixDomainFd, PayloadHdr* ph, NetworkInterface* incomingIntf, SockAddrLl sndAddr) {
	//DONt join this printf statements
	printf("%s got a packet message: MsgType %d: %s from %s:%u ", nt(), ph->msgType, ph->msg, printIPHuman(ph->srcIp), ph->srcPort);
	printf("to %s:%u \n", printIPHuman(ph->destIp), ph->destPort);
	
	NetworkInterface* ifThatSenderWants = findMatchByIp(ph->destIp);
	int atDest = ifThatSenderWants != NULL;

	// Destination IP should be compared with the IP of ONE of the interfaces. If you find at least one match - it's destination node.
	printRoutingTable(headEntry, tailEntry);
	in_addr_t destIp = ph->destIp;
	RouteEntry* destEntry = findRouteEntry(destIp, &headEntry, &tailEntry);
	if (destEntry == NULL && atDest == 0) {
		printf("No forward IP %s in routing table\n", printIPHuman(destIp));
	}

	// if we're not a destination, we may want to change PLD->RREQ, because request comes from client who sends payload
	if (ph->srcIp != ph->destIp && ph->msgType == MT_PLD && destEntry == NULL && atDest == 0) {
		bid++;
		ph->broadcastId = bid;
		ph->msgType = MT_RREQ;
		printf("%s MT_PLD->MT_RREQ. New BID given %d\n", nt(), ph->broadcastId);
	}
	
	RouteEntry* srcEntry = findRouteEntry(ph->srcIp, &headEntry, &tailEntry);	

	// Don't increase hop count if
	//1) the request comes from the same host
	//2)if comes RREP, and we don't have destination route (it's exceptional case)
	int weDontKnowRREP = (ph->msgType == MT_RREP) && (destEntry == NULL);
	if (incomingIntf->ipAddr != ph->srcIp) {
		ph->hopCount++;
	}
	// Insert/update reverse path entry
	insertOrUpdateRouteEntry(ph->srcIp, sndAddr.sll_addr, ph->hopCount, sndAddr.sll_ifindex, &headEntry, &tailEntry);
	if (ph->msgType == MT_RREP && srcEntry != NULL) {
		return;
	}
	
	printf("%s MsgType:%d, from %s:%d \n",nt(), ph->msgType, printIPHuman(ph->srcIp), ph->srcPort);
	// It's a RREQ
	if (ph->msgType == MT_RREQ) {
		// At intermediate node
		if (atDest == 0) {
			if (destEntry != NULL) {	
				printf("%s INT.NODE: RREQ received. I got a route, so RREP back and propagate\n", nt());
				int aBRet = addBidEntry(ph->srcIp, ph->broadcastId, ph->hopCount, &headBid, &tailBid);
				if(aBRet == 0){
					printf("\nBID: RREP already sent that was more efficient. Disreguarding.\n");
					return;
				}
				else{
					printf("BID: RREP needs to be sent/resent.");
				}
				// RREP back and propagate
				PayloadHdr respHdr = convertToResponse(*ph);
				rrepBack(rawSockFd, respHdr, incomingIntf->macAddress, sndAddr);
				sendToRoute(rawSockFd, *ph, incomingIntf->macAddress, *destEntry);
			}
			// Flood if we don't know the route
			else {
				printf("%s INT.NODE: RREQ received. Don't have a route, so flood\n", nt());
				flood(rawSockFd, *ph, sndAddr);
			}
		} 
		// At destination
		else {		  
			printf("%s DEST.NODE: RREQ received. So send RREP\n", nt());
			PayloadHdr respHdr = convertToResponse(*ph);	   
			respHdr.hopCount = 0;
			rrepBack(rawSockFd, respHdr, incomingIntf->macAddress, sndAddr);
		}
	}

	// It's a RREP
	else if (ph->msgType == MT_RREP) {
		// At intermediate node
		if (atDest == 0) {
			// disregard it - we don't have a reverse path for it
			if (destEntry == NULL) {
				return;
			}
			int aBRet = addBidEntry(ph->srcIp, ph->broadcastId, ph->hopCount, &headBid, &tailBid);
			if(aBRet == 0){
				printf("\nBID: RREP already sent that was more efficient. Disreguarding.\n");
				return;
			}
			else{
				printf("BID: RREP needs to be sent/resent.");
			}
			printf("%s INT.NODE: RREP received. Send RREP back to the REQQuestor\n", nt());
			sendToRoute(rawSockFd, *ph, incomingIntf->macAddress, *destEntry);
		}
		// At destination
		else {
			printf("%s DEST.NODE: RREP received. Send Hi!\n", nt());
			PayloadHdr rph = convertToResponse(*ph);
			rph.hopCount = 0;
			strcpy(rph.msg, "Hi!\0");
			sendToRoute(rawSockFd, rph, incomingIntf->macAddress, *srcEntry);
		}
	}
	// It's an APP payload packet
	else {
		// At intermediate node
		if (atDest == 0) {
			if (destEntry != NULL) {		
				printf("%s INT.NODE: Forward message:%s\n", nt(), ph->msg);
				sendToRoute(rawSockFd, *ph, incomingIntf->macAddress, *destEntry);
			}			
		}		
		else{
			//We're at destination, get UNIX file by port and send payload to the process
			printf("%s DEST.NODE: Got a payload message: %s. Pushing it up to the process...\n", nt(), ph->msg);
			handlePacketAtDestinationNode(unixDomainFd, ph);
		}
	}
}

void rrepBack(int rawSockFd, PayloadHdr respH, unsigned char currentMac[ETH_ALEN], SockAddrLl recvAddr) {
	odrSend(rawSockFd, respH, currentMac, recvAddr.sll_addr, recvAddr.sll_ifindex);
}

void sendToRoute(int rawSockFd, PayloadHdr ph, unsigned char currentMac[ETH_ALEN], RouteEntry destEntry) {
	odrSend(rawSockFd, ph, currentMac, destEntry.next_hop, destEntry.interfaceInd);
}

void flood(int rawSockFd, PayloadHdr ph, SockAddrLl senderAddr) {
	unsigned char toAll[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//{0x00,0x0c,0x29,0xde,0x6a,0x6c};//
	//Send to all interfaces except incoming interface, eth0 and lo.
	printf("***BROADCAST***\n");
	NetworkInterface* ptr;
	for(ptr = ifHead;ptr != NULL;ptr = ptr->next) {
		if (senderAddr.sll_ifindex == ptr->interfaceIndex) {
			continue;
		}
		if (ptr->isLo == 1) {
			continue;
		}
		//allow to use eth0 only for the same host
		if (ph.srcIp != ph.destIp && ptr->isEth0 == 1) {
			continue;
		}

		odrSend(rawSockFd, ph, ptr->macAddress, toAll, ptr->interfaceIndex);
	}	
	printf("***BROADCAST FINISHED***\n");
}

int hasMatch(unsigned char mac[ETH_ALEN]) {
	NetworkInterface* p = NULL;
	for(p = ifHead; p!=NULL; p=p->next) {
		int i, found = 1;
		for(i=0;i<ETH_ALEN;++i) {
			if(p->macAddress[i] != mac[i]){
				found = 0;
				break;
			}
		}
		if (found == 1) {
			return 1;
		}
	}
	return 0;
}

NetworkInterface* findMatchByIp(in_addr_t ip) {
	NetworkInterface* p = NULL;
	for(p = ifHead;p!=NULL;p=p->next) {
		if (p->ipAddr == ip) {
			return p;
		}
	}

	return p;
}