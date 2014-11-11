#include "unp.h"
#include "misc.h"

int main(int argc, char **argv)
{
	int					sockfd, n;
	socklen_t			len;
	char				recvline[MAXLINE + 1];
	struct sockaddr_un	servaddr;
	if ( (sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXSTR_PATH);
	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;	
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if (n < 0)
		err_sys("read error");

	exit(0);
}
