#include "unp.h"
#include "odrProc.h"
#include "oapi.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6]) {	
	SockAddrLl socket_address;/*target address*/	
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*buffer for ethernet frame*/	
	unsigned char* etherhead = buffer;/*pointer to ethenet header*/	
	unsigned char* data = buffer + 2*ETH_ALEN + 2;/*userdata in ethernet frame*/	
	struct ethhdr *eh = (struct ethhdr *)etherhead;/*another pointer to ethernet header*/	 
	int send_result = 0;

	/*prepare sockaddr_ll*/	
	socket_address.sll_family   = PF_PACKET;/*RAW communication*/
	socket_address.sll_protocol = htons(52456);	/*we don't use a protocoll above ethernet layer->just use anything here*/
	socket_address.sll_ifindex  = 3; /*index of the network device see full code later how to retrieve it*/	
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
	
	// copy not only msg, but also dest IP
	FrameUserData frameUserData;
	frameUserData.portNumber = dto->destPort;
	frameUserData.ipAddr = malloc(IP_ADDR_LEN);
	strcpy(frameUserData.ipAddr, dto->destIp);
	frameUserData.msg = malloc(ETH_MAX_MSG_LEN);
	strcpy(frameUserData.msg, dto->msg);	
	serialFrameUdata(frameUserData, data);
	printf("Here\n");

	int sd;
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	}
	/*send the packet*/
	printf("ODR:sendto PF_PACKET %d\n", sd);
	send_result = sendto(sd, buffer, ETH_FRAME_LEN, 0, (SockAddrLl*)&socket_address, sizeof(socket_address));
	if (send_result == -1) { 
		printf("send result == -1\n");
		return 1;
	}
	printf("send res=%d\n", send_result);
}

int odrRecv(int sockfd, FrameUserData* userData) {
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0;
	
	SockAddrLl senderAddr;
	int sz = sizeof(senderAddr);

	length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (SockAddrLl*)&senderAddr, &sz);
	if (length == -1) { 
		printf("Error when received");
		return 0;
	}
	int protocol = ntohs(senderAddr.sll_protocol);
	printf("protocol:%d", protocol);
	if (protocol != 52456) {
		return 0;
	}

	// extract msg, IP, etc	
	char* rawUserData = malloc(ETHFR_MAXDATA_LEN);
	strcpy(rawUserData, (char*)(buffer + 14));
	printf("Raw:%s\n", rawUserData);
	deserialFrameUdata(rawUserData, userData);

	printf("ODR: Received msg = %s\n", userData->msg);
	char* srcMac = (char*)(buffer + ETH_ALEN);

	return 1;
}

void serialFrameUdata(FrameUserData dto, unsigned char* out) {
	unsigned char* ptrPaste = out;
	char* portNum = itostr2(dto.portNumber);	
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
				out->ipAddr = malloc(IP_ADDR_LEN);
				strcpy(out->ipAddr, ptr);
				break;
			case 1:
				out->portNumber = atoi(ptr);
				break;
			case 2:
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
	sprintf(res, "%d\0", val);
	return res;
}