#include "unp.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>



int msg_send(int sockfd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery) {
	//00:0c:29:49:3f:5b vm1
	//00:0c:29:d9:08:ec vm2

	/*target address*/
	struct sockaddr_ll socket_address;
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN); 
	/*pointer to ethenet header*/
	unsigned char* etherhead = buffer;		
	/*userdata in ethernet frame*/
	unsigned char* data = buffer + 14;		
	/*another pointer to ethernet header*/
	struct ethhdr *eh = (struct ethhdr *)etherhead;
	 
	int send_result = 0;
	//TODO: use prhwaddrs to use arbitrary MAC	
	unsigned char src_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*our MAC address*/	
	unsigned char dest_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*other host MAC address*/

	/*prepare sockaddr_ll*/	
	socket_address.sll_family   = PF_PACKET;/*RAW communication*/
	socket_address.sll_protocol = htons(ETH_P_IP);	/*we don't use a protocoll above ethernet layer->just use anything here*/
	socket_address.sll_ifindex  = 2; /*index of the network device see full code later how to retrieve it*/	
	socket_address.sll_hatype   = ARPHRD_ETHER;	/*ARP hardware identifier is ethernet*/	
	socket_address.sll_pkttype  = PACKET_OTHERHOST;/*target is another host*/	
	socket_address.sll_halen    = ETH_ALEN;		/*address length*/
	/*MAC - begin*/	
	int i;
	for(i = 0;i < ETH_ALEN; ++i) {
		socket_address.sll_addr[i]  = dest_mac;
	}
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/

	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = 0x00;
	/*fill the frame with data*/	
	strcpy(data, msg);

	int sd;
	//s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	}
	/*send the packet*/
	send_result = sendto(sd, buffer, ETH_FRAME_LEN, 0, (SA*)&socket_address, sizeof(socket_address));
	if (send_result == -1) { 
		printf("send result == -1\n");
		return 1;
	}
}

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort) {
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 
	
	length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
	if (length == -1) { 
		printf("%s\n", "Length=-1");
	}
}