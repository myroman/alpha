#include "unp.h"
#include "misc.h"

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
	strcpy(cliaddr.sun_path, tmpnam(NULL));
	bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);
	
	char* s="";
	sendto(sockfd, s, 0, 0, (SA*)&servaddr, sizeof(servaddr));
	printf("Sent to %d\n", sockfd);
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if (n < 0)
		err_sys("read error");

	exit(0);
}
