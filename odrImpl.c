#include "unp.h"
#include "misc.h"
#include "debug.h"
#include "odrProc.h"
#include "oapi.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex) {
	char* nl = createTmplFilename();
	printf("Created misc nl:%s\n", nl);

	SockAddrLl socket_address;/*target address*/	
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*buffer for ethernet frame*/	
	unsigned char* etherhead = buffer;/*pointer to ethenet header*/	
	unsigned char* data = buffer + 2*ETH_ALEN + 2;/*userdata in ethernet frame*/	
	struct ethhdr *eh = (struct ethhdr *)etherhead;/*another pointer to ethernet header*/	 
	int send_result = 0;

	/*prepare sockaddr_ll*/	
	socket_address.sll_family   = PF_PACKET;/*RAW communication*/
	socket_address.sll_protocol = htons(PROTOCOL_NUMBER);	/*we don't use a protocoll above ethernet layer->just use anything here*/
	socket_address.sll_ifindex  = destInterfaceIndex; /*index of the network device see full code later how to retrieve it*/	
	socket_address.sll_hatype   = ARPHRD_ETHER;	/*ARP hardware identifier is ethernet*/	
	socket_address.sll_pkttype  = PACKET_OTHERHOST;/*target is another host*/	
	socket_address.sll_halen    = ETH_ALEN;		/*address length*/
	/*MAC - begin*/	
	int i;
	for(i = 0;i < ETH_ALEN; ++i) {
		socket_address.sll_addr[i]  = destMac[i];
	}
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/

	/*set the frame header*/
	memcpy((void*)buffer, (void*)destMac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)srcMac, ETH_ALEN);
	eh->h_proto = 0x00;
	// copy SendDto to FrameUserData
	FrameUserData frameUserData;
	frameUserData.portNumber = dto->destPort;
	strcpy(frameUserData.ipAddr, dto->destIp);
	
	frameUserData.srcPortNumber = dto->srcPort;
	strcpy(frameUserData.srcIpAddr, dto->srcIp);

	frameUserData.msg = malloc(ETH_MAX_MSG_LEN);
	strcpy(frameUserData.msg, dto->msg);

	serialFrameUdata(frameUserData, data);
	int sd;
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	}
	/*send the packet*/
	debug("ODR:sending PF_PACKET with data %s", data);
	send_result = sendto(sd, buffer, ETH_FRAME_LEN, 0, (SA* )&socket_address, sizeof(socket_address));
	if (send_result == -1) { 
		debug("send result == -1");
		return 1;
	}
	debug("ODR: send result:%d", send_result);
}

int odrRecv(int sockfd, FrameUserData* userData) {
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0;
	
	SockAddrLl senderAddr;
	int sz = sizeof(senderAddr);
	
	length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (SA *)&senderAddr, &sz);
	if (length == -1) { 
		printf("Error when received");
		return 0;
	}
	if (ntohs(senderAddr.sll_protocol) != PROTOCOL_NUMBER) {
		//printf(".");
		return 0;
	}
	debug("ODR:received something");
	
	// extract msg, IP, etc	
	char* rawUserData = malloc(length);
	strcpy(rawUserData, (char*)(buffer + 14));
	
	debug("ODR: Raw data:%s", rawUserData);
	deserialFrameUdata(rawUserData, userData);
	char* srcMac = (char*)(buffer + ETH_ALEN);

	return 1;
}

void serialFrameUdata(FrameUserData dto, unsigned char* out) {
	unsigned char* ptrPaste = out;
	char* portNum = itostr2(dto.portNumber);
	char* srcPortNum = itostr2(dto.srcPortNumber);
	ptrPaste = cpyAndMovePtr2(ptrPaste, dto.srcIpAddr);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, srcPortNum);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, dto.ipAddr);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, portNum);
	ptrPaste = addDlm2(ptrPaste);
	ptrPaste = cpyAndMovePtr2(ptrPaste, dto.msg);
	ptrPaste = cpyAndMovePtr2(ptrPaste, "\0");	
	free(portNum);
}

void deserialFrameUdata(char* src, FrameUserData* out) {
	char* ptr = strtok(src, "|");
	int member = 0;
	while(ptr) {
		switch(member++) {
			printf("Member #%d=%s", member, ptr);
			case 0:
				strcpy(out->srcIpAddr, ptr);
				break;
			case 1:
				out->srcPortNumber = atoi(ptr);
				break;
			case 2:
				strcpy(out->ipAddr, ptr);
				break;
			case 3:
				out->portNumber = atoi(ptr);
				break;
			case 4:
				out->msg = malloc(ETH_MAX_MSG_LEN);
				strcpy(out->msg, ptr);
				break;
		};
		ptr = strtok(NULL, "|");
	}
}

char* cpyAndMovePtr2(unsigned char* destPtr, const char* src) {
	strcpy(destPtr, src);
	destPtr += strlen(src);
	return destPtr;
}
char* addDlm2(unsigned char* destPtr) {
	char* delimiter = "|\0";
	return cpyAndMovePtr(destPtr, delimiter);
}
char* itostr2(int val){
	char* res = malloc(10);
	bzero(res, 10);
	sprintf(res, "%d", val);
	return res;
}