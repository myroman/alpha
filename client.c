#include "unp.h"
#include "misc.h"
#include "oapi.h"

int main(int argc, char **argv)
{
	int					sockfd, n;
	socklen_t			len;
	char				recvline[MAXLINE + 1];
	struct sockaddr_un	servaddr, cliaddr;
	if ( (sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");

	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strcpy(cliaddr.sun_path, mkstemp(getUnique6()));
	bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);

	char* s = malloc(ETHFR_MAXDATA_LEN);
	rmnl(s);	
	int forceRediscovery = 0;

	//TODO: how should I use forceRediscovery?
	msg_send(sockfd, "192.168.123.123", DAYTIME_PORT, s, forceRediscovery);	

	return;	
	
	printf("Requested time...\n");
	char* destIpAddr;
	int destPort;
	while ((n = msg_recv(sockfd, recvline, destIpAddr, &destPort))) {
		printf("Got a response, n = %d\n", n);
		recvline[n] = 0;	
		printf("Timestamp: %s", recvline);		
	}

	/*while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}*/
	if (n < 0)
		err_sys("read error");

	exit(0);
}
