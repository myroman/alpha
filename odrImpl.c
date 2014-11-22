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

int odrSend(PayloadHdr* ph, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex) {
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
	eh->h_proto = 0x00;

	// SENDING PART
	int sd;
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    free(buffer);
	    exit (EXIT_FAILURE);
	}
	int payloadLength;
	void* payload = packPayload(ph, &payloadLength);
	memcpy(data, payload, payloadLength);
	free(payload);
	
	/*send the packet*/
	printf("ODR:sending PF_PACKET with msg %s...", ph->msg);
	int res = sendto(sd, buffer, ETH_FRAME_LEN, 0, (SA* )&socket_address, sizeof(socket_address));
	if (res == -1) { 
		printFailed();
		
		free(buffer);
		return 1;
	}
	printOK();
	free(buffer);
}

int odrRecv(PayloadHdr* ph) {	
	int sockfd;
	if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    printf("%s Socket failed ", nt());
	    return 0;
	}

	SockAddrLl senderAddr;	
	int sz = sizeof(senderAddr);
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/	
	int length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (SA *)&senderAddr, &sz);
	if (length == -1) {		
		free(buffer);
		return 0;
	}
	if (ntohs(senderAddr.sll_protocol) != PROTOCOL_NUMBER) {				
		free(buffer);
		return 0;
	}	
	debug("ODR: Received PF_PACKET, length=%d", length);
	unpackPayload(buffer + 14, ph);
	free(buffer);	
	return 1;	
}