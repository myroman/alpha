#include "unp.h"
#include <time.h>
#include "misc.h"
#include "oapi.h"

void replyTs(int sockfd);

int main(int argc, char **argv)
{	
	int listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(SRV_UNIX_PATH);
	SockAddrUn servaddr = createSockAddrUn(SRV_UNIX_PATH);
	bind(listenfd, (SockAddrUn *) &servaddr, sizeof(servaddr));	
	
	replyTs(listenfd);
}

void replyTs(int sockfd)
{
	char* msg = malloc(ETH_MAX_MSG_LEN);
	char buff[MAXLINE];
	time_t ticks;
	char* srcIpAddr = malloc(IP_ADDR_LEN);
	int srcPort, forceRediscovery = 0;

	for ( ; ; ) {
		printf("Waiting for request...");
		int n = msg_recv(sockfd, msg, srcIpAddr, &srcPort);
		if (n == -1){
			printf("Timeout msg_recv returned -1");
			continue;
		}
		printf("Received msg:%s from %s:%d\n", msg, srcIpAddr, srcPort);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\0", ctime(&ticks));
        
        printf("Sending %s\n", buff);        
		msg_send(sockfd, ROMAN_IP_TEST, 1024, buff, forceRediscovery);
	}

	free(msg);
	free(srcIpAddr);
}