#include "unp.h"
#include "oapi.h"
#include "odrProc.h"
#include "misc.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

void* respondToHostRequestsRoutine (void *arg);

void* respondToNetworkRequestsRoutine (void *arg);

char* ut();
char* nt();
int deserializeApiReq(char* buffer, size_t bufSz, SendDto* dto);

struct sockaddr_un servaddr;

// 2 MACs of Vm1 and Vm2
//00:0c:29:49:3f:5b vm1
//00:0c:29:d9:08:ec vm2

/* Listen to: 1) UNIX domain sockets for messages from client/server 
2) PF_PACKETs from another ODR
1) When received UNIX - deserialize the data into arguments which they sent: for msg_send or msg_recv.
Then call a function from odrImpl.c
2) When you get a response from ODR, serialize it and send back to the needed socket	*/
int main(int argc, char **argv) {
	
	int unixDomainFd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(UNIXDG_PATH);
	servaddr = createSA(UNIXDG_PATH);	
	Bind(unixDomainFd, (SA *) &servaddr, sizeof(servaddr));	

	pthread_t unixDmnListener, networkListener;
	pthread_create(&unixDmnListener, NULL, (void *)&respondToHostRequestsRoutine, (void*)unixDomainFd);
	pthread_create(&networkListener, NULL, (void *)&respondToNetworkRequestsRoutine, (void*)unixDomainFd);

	pthread_join(unixDmnListener, NULL);
	pthread_join(networkListener, NULL);	
	
	unlink(UNIXDG_PATH);
	printf("ODR terminated\n");
}

void* respondToHostRequestsRoutine (void *arg) {
	//TODO: use prhwaddrs to use arbitrary MAC	
	unsigned char src_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*our MAC address*/	
	unsigned char dest_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*other host MAC address*/

	socklen_t clilen;
	
	int listenfd = (int)arg;	
	char* buffer = malloc(MAXLINE);
	struct sockaddr_un cliaddr;
	for(;;) {
		printf("%s Waiting from %d...\n", ut(), listenfd);
		int l = sizeof(cliaddr);
		int length = recvfrom(listenfd, buffer, MAXLINE, 0, (SA *)&cliaddr, &l);
		printf("%s Got a request, length=%d\n",ut(),length);
		
		//Parse string
		SendDto* dto = malloc(sizeof(dto));
		int res = deserializeApiReq(buffer, length, dto);
		printf("%s Deser: msgtype=%d, destIp=%s\n", ut(), dto->msgType, dto->destIp);

		if (dto->msgType == CLIENT_MSG_TYPE) {
			printf("%s Got a msg from client. Sending msg %s\n", ut(), dto->msg);
			odrSend(dto, src_mac, dest_mac);
		} else {
			printf("Another ODR request: length=%d\n", length);
		}
	}	

	pthread_exit(0);
}

void* respondToNetworkRequestsRoutine (void *arg) {
	int sockfd, 
		srcPort, 
		unixDomainFd = (int)arg;
	char* msg = malloc(MAXLINE); //TODO: or MAX ETHERNET LENGTH?
	char* srcIpAddr = malloc(15);
	
	if ((sockfd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    printf("%s Socket failed ", nt());
	    exit (EXIT_FAILURE);
	}
	
	for(;;) {
		printf("%s waiting for PF_PACKET...\n", nt());
		int n = odrRecv(sockfd, msg, srcIpAddr, &srcPort);
		printf("%s got a message: %s from IP %s, port %d \n", nt(), msg, srcIpAddr, srcPort);
		sendto(unixDomainFd, msg, strlen(msg), 0, (SA *)&servaddr, sizeof(servaddr));
		return;		
	}
}

char* nt() {
	return "Network thread:";
}

char* ut() {
	return "Unix thread:";
}

int deserializeApiReq(char* buffer, size_t bufLen, SendDto* dto) {
	char* s = malloc(bufLen);
	strcpy(s, buffer);

	char *tok = NULL, *delim = "|";
    int len = 0, member = 0;     
 	
    tok = strtok(s, delim);
	while (tok) {
    	switch(member++){
        	case 0:
        		dto->msgType = atoi(tok);
        		break;
			case 1:
				dto->destIp = malloc(strlen(tok));
				strcpy(dto->destIp, tok);
				break;
			case 2:
				dto->destPort = atoi(tok);
				break;
			case 3:
				dto->msg = malloc(strlen(tok));
				strcpy(dto->msg, tok);
				break;
			case 4:
				dto->forceRedisc = atoi(tok);
				break;
			case 5:
				dto->callbackFd = atoi(tok);
				break;
        }
        tok = strtok(NULL, delim);
    }    

    return 1;
}