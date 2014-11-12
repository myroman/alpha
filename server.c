#include "unp.h"
#include <time.h>
#include "misc.h"
#include "oapi.h"

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen);

/*
void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated\n", pid);
	}
	return;
}
*/
int main(int argc, char **argv)
{
	int					listenfd, sd;
	socklen_t			clilen;
	struct sockaddr_un	cliaddr, servaddr;
	//void				sig_chld(int);	

	listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	/*if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	}*/
	unlink(UNIXDG_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	//Signal(SIGCHLD, sig_chld);

	replyTs(listenfd, (SA *)&cliaddr, sizeof(cliaddr));
}

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];
	char				buff[MAXLINE];
	time_t				ticks;
	char* srcIpAddr = malloc(20);
	int srcPort, forceRediscovery = 0;

	for ( ; ; ) {
		len = clilen;

		// TODO: It should work, but now it doesn't because of what is inside
		n = msg_recv(sockfd, mesg, srcIpAddr, &srcPort);
		//n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		printf("Received request\n");		
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		//Sendto(sockfd, buff, sizeof(buff), 0, pcliaddr, len);

		msg_send(sockfd, srcIpAddr, srcPort, buff, forceRediscovery);
	}
}