#include	"unp.h"
#include	<time.h>
#include "misc.h"

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
	char				buff[MAXLINE];
	time_t				ticks;

	listenfd = Socket(AF_LOCAL, SOCK_STREAM, 0);

	unlink(UNIXSTR_PATH);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strcpy(servaddr.sun_path, UNIXSTR_PATH);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	Signal(SIGCHLD, sig_chld);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		connfd = Accept(listenfd, (SA *) &cliaddr, &clilen);

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        Write(connfd, buff, strlen(buff));

		Close(connfd);
	}
}
