#include "unp.h"
#include <time.h>
#include "misc.h"
#include "oapi.h"
#include "debug.h"
void replyTs(int sockfd);

int main(int argc, char **argv)
{	
	int listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(SRV_UNIX_PATH);
	SockAddrUn servaddr = createSockAddrUn(SRV_UNIX_PATH);
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));	
	
	replyTs(listenfd);
}

void replyTs(int sockfd)
{
	char* msg = malloc(MAXLINE);
	char buff[MAXLINE];
	bzero(buff, MAXLINE);
	time_t ticks;
	char srcIpAddr[IP_ADDR_LEN];
	int srcPort, forceRediscovery = 0;

	for ( ; ; ) {
		debug("Waiting for request...");
		int n = msg_recv(sockfd, msg, srcIpAddr, &srcPort);
		if (n == -1){
			printf("Timeout msg_recv\n");
			continue;
		}
		printf("Received msg:%s from %s:%d\n", msg, srcIpAddr, srcPort);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
        
        printf("Sending %s\n", buff);  
        //"10.255.14.128\0"  
        //TODO     
		msg_send(sockfd, srcIpAddr, srcPort, buff, forceRediscovery);
	}

	free(msg);
	free(srcIpAddr);
}