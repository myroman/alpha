#include "unp.h"
#include <time.h>
#include "misc.h"
#include "oapi.h"

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen);

int main(int argc, char **argv)
{	
	int listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(SRV_UNIX_PATH);
	SockAddrUn servaddr = createSockAddrUn(SRV_UNIX_PATH);
	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));	
	SockAddrUn cliaddr;
	replyTs(listenfd, (SA *)&cliaddr, sizeof(cliaddr));
}

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char* msg[MAXLINE];
	char buff[MAXLINE];
	time_t ticks;
	char* srcIpAddr = malloc(20);
	int srcPort, forceRediscovery = 0;

	for ( ; ; ) {
		len = clilen;
		printf("Waiting for request...");
		n = msg_recv(sockfd, msg, srcIpAddr, &srcPort);
		printf("Received. ");
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        printf("Send [%s] back", buff);
        
		msg_send(sockfd, srcIpAddr, srcPort, buff, forceRediscovery);
	}
}