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

int odrSend(int odrSockFd, PayloadHdr ph, unsigned char srcMac[6], unsigned char destMac[6], int srciFindex) {
	debug("ODR: Network interface %d, odrSockFd %d", srciFindex, odrSockFd);
	printf("ODR: Gonna send from MAC ");
	printMac(srcMac);
	printf(" to ");
	printMac(destMac);
	printf("\n");

	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*buffer for ethernet frame*/	
	unsigned char* data = (buffer + 2*ETH_ALEN + 2);/*userdata in ethernet frame*/	
	struct ethhdr *eh = (struct ethhdr *)buffer;/*another pointer to ethernet header*/	 
	eh->h_proto = htons(PROTOCOL_NUMBER);//htons();
	
	/*set the frame header*/
	memcpy((void*)buffer, (void*)destMac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)srcMac, ETH_ALEN);
	SockAddrLl socket_address;/*target address*/	
	socket_address.sll_family   = PF_PACKET;/*RAW communication*/
	socket_address.sll_protocol = htons(PROTOCOL_NUMBER);
	socket_address.sll_ifindex  = srciFindex; /*index of the network device see full code later how to retrieve it*/	
	socket_address.sll_halen    = ETH_ALEN;		/*address length*/
	/*MAC - begin*/	
	int i;
	for(i = 0;i < ETH_ALEN; ++i) {
		socket_address.sll_addr[i]  = destMac[i];
	}
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/


	// SENDING PART	
	int payloadLength;
	void* payload = packPayload(&ph, &payloadLength);	
	memcpy(data, payload, payloadLength);
	free(payload);
	
	/*send the packet*/
	printf("ODR:sending PF_PACKET with msg %s msgType %d to socket %d...", ph.msg, ph.msgType,odrSockFd);

	int res = sendto(odrSockFd, buffer, ETH_FRAME_LEN, 0, (SockAddrLl* )&socket_address, sizeof(struct sockaddr_ll));
	if (res == -1) { 
		printFailed();
		
		free(buffer);
		return 1;
	}
	printOK();
	free(buffer);
}

void printMac(unsigned char mac[6]) {
	int i=0;
	for(i=0;i < 6; ++i) {
		printf("%.2x:", mac[i]);
	}
}