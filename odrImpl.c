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

int odrSend(int odrSockFd, PayloadHdr ph, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex) {
	//debug("\n\n\n\nNetwork interface %u, odrSockFd %u\n\n\n\n", destInterfaceIndex, odrSockFd);
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

	// TODO: htons for sll_familty and prot
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
	int16_t prot = htons(0x1C9);
	memcpy((void*)buffer+2*ETH_ALEN, &prot, 2);
	eh->h_proto = 0x00;

	// SENDING PART	
	int payloadLength;
	void* payload = packPayload(&ph, &payloadLength);
	memcpy(data, payload, payloadLength);
	free(payload);
	
	/*send the packet*/
	//printf("SrcIP %s:%u, DestIP %s:%u\n", printIPHuman(ph.srcIp), ph.srcPort, printIPHuman(ph.destIp), ph.destPort);
	printf("ODR:sending PF_PACKET with msg %s msgType %d...", ph.msg, ph.msgType);

	int res = sendto(odrSockFd, buffer, ETH_FRAME_LEN, 0, (SA* )&socket_address, sizeof(socket_address));
	if (res == -1) { 
		printFailed();
		
		free(buffer);
		return 1;
	}
	printOK();
	debug("Res = %d", res);
	free(buffer);
}

void printMac(unsigned char mac[6]) {
	int i=0;
	for(i=0;i < 6; ++i) {
		printf("%.2x:", mac[i]);
	}
}