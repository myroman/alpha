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
		printf("Received msg:%s, %s:%d\n", msg, srcIpAddr, srcPort);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        printf("Sent %s\n", buff);
        
		msg_send(sockfd, "172.23.11.2", 42135, buff, forceRediscovery);
	}

	free(msg);
	free(srcIpAddr);
}