#include	"unp.h"
#include	<time.h>
#include "misc.h"

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen);

void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated\n", pid);
	}
	return;
}

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	socklen_t			clilen;
	struct sockaddr_un	cliaddr, servaddr;
	void				sig_chld(int);	

	listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

	unlink(UNIXDG_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXDG_PATH);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Signal(SIGCHLD, sig_chld);

	replyTs(listenfd, (SA *)&cliaddr, sizeof(cliaddr));
}

void replyTs(int sockfd, SA *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];
	char				buff[MAXLINE];
	time_t				ticks;

	for ( ; ; ) {
		len = clilen;
		n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		printf("Received request\n");
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		Sendto(sockfd, buff, sizeof(buff), 0, pcliaddr, len);
	}
}
