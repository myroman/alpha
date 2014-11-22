#include "unp.h"
#include "misc.h"
#include "debug.h"
#include "odrProc.h"
#include "oapi.h"
#include "hw_addrs.h"
#include "payloadHdr.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

int odrSend(int odrSockFd, PayloadHdr* ph, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex) {
	printf("ODR: Gonna send from MAC ");
	printMac(srcMac);
	printf(" to ");
	printMac(destMac);
	printf("\n");

	SockAddrLl socket_address;/*target address*/	
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*buffer for ethernet frame*/	
	unsigned char* etherhead = buffer;/*pointer to ethenet header*/	
	unsigned char* data = buffer + 2*ETH_ALEN + 2;/*userdata in ethernet frame*/	
	struct ethhdr *eh = (struct ethhdr *)etherhead;/*another pointer to ethernet header*/	 
	
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
	int16_t prot = 0x1C9;
	memcpy((void*)buffer+2*ETH_ALEN, &prot, 2);
	eh->h_proto = 0x00;

	// SENDING PART	
	int payloadLength;
	void* payload = packPayload(ph, &payloadLength);
	memcpy(data, payload, payloadLength);
	free(payload);
	
	/*send the packet*/
	
	printf("ODR:sending PF_PACKET with msg %s...", ph->msg);
	int res = sendto(odrSockFd, buffer, ETH_FRAME_LEN, 0, (SA* )&socket_address, sizeof(socket_address));
	if (res == -1) { 
		printFailed();
		
		free(buffer);
		return 1;
	}
	printOK();
	free(buffer);
}

int odrRecv(int odrSockFd, PayloadHdr* ph, void* buffer) {	
	SockAddrLl senderAddr;	
	int sz = sizeof(senderAddr);
	bzero(buffer, ETH_FRAME_LEN);
	int length = recvfrom(odrSockFd, buffer, ETH_FRAME_LEN, 0, (SA *)&senderAddr, &sz);
	if (length == -1) {
		return 0;
	}
	if (ntohs(senderAddr.sll_protocol) != PROTOCOL_NUMBER) {				
		//printf("%d,", ntohs(senderAddr.sll_protocol));
		return 0;
	}	

	debug("\nODR: Received PF_PACKET, length=%d", length);
	unpackPayload(buffer + 14, ph);
	return 1;	
}

void printMac(unsigned char mac[6]) {
	int i=0;
	for(i=0;i < 6; ++i) {
		printf("%.2x:", mac[i]);
	}
}