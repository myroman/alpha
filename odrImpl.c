#include "unp.h"
#include "odrProc.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

int odrSend(int sockfd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery, unsigned char srcMac[6], unsigned char destMac[6]) {	
	struct sockaddr_ll socket_address;/*target address*/	
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*buffer for ethernet frame*/	
	unsigned char* etherhead = buffer;/*pointer to ethenet header*/	
	unsigned char* data = buffer + 2*ETH_ALEN + 2;/*userdata in ethernet frame*/	
	struct ethhdr *eh = (struct ethhdr *)etherhead;/*another pointer to ethernet header*/	 
	int send_result = 0;

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
		socket_address.sll_addr[i]  = destMac[i];
	}
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/

	/*set the frame header*/
	memcpy((void*)buffer, (void*)destMac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)srcMac, ETH_ALEN);
	eh->h_proto = 0x00;
	
	strcpy(data, msg);

	int sd;
	//s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	}
	/*send the packet*/
	printf("ODR:sendto PF_PACKET %d\n", sd);
	send_result = sendto(sd, buffer, ETH_FRAME_LEN, 0, (SA*)&socket_address, sizeof(socket_address));
	if (send_result == -1) { 
		printf("send result == -1\n");
		return 1;
	}
}

int odrRecv(int sockfd, char* msg, char* srcIpAddr, int* srcPort) {
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 

	length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
	if (length == -1) { 
		printf("Error when received");
		return;
	}

	char* _msg = (char*)(buffer + 14);
	strcpy(msg, _msg);
	printf("Received msg = %s\n", msg);

	char* srcMac = (char*)(buffer + ETH_ALEN);
	//TODO: find out IP basing on the MAC
}