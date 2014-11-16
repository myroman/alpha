#include "unp.h"
#include "misc.h"
#include "oapi.h"

char* listenFn;

void sig_int(int signo) {
	unlink(listenFn);
	printf("I'm cleaned up %s before termination\n", listenFn);
	exit(0);
}

int main(int argc, char **argv)
{
	int	lstFd, n;
	socklen_t len;
	char recvline[MAXLINE + 1];
	if ( (lstFd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");
	Signal(SIGINT, sig_int);

	// Setup listening filename
	listenFn = malloc(10);
	strncpy(listenFn, createTmplFilename(), 10);
	mkstemp(listenFn);

	SockAddrUn addr = createSockAddrUn(listenFn);

	bind(lstFd, (SockAddrUn *)&addr, sizeof(addr));
	
	char* s = malloc(ETHFR_MAXDATA_LEN);
	strcpy(s, "Hi!\n");
	rmnl(s);	
	int forceRediscovery = 0, res;

	res = msg_send(lstFd, "192.168.123.123\0", SRV_PORT_NUMBER, s, forceRediscovery);

	//return;
	printf("Requested time...\n");
	char* destIpAddr;
	int destPort;
	while ((n = msg_recv(lstFd, recvline, destIpAddr, &destPort))) {
		printf("Got a response, n = %d\n", n);
		recvline[n-1] = 0;	
		printf("hey");
		printf("Timestamp: %s", recvline);		
	}
	
	if (n < 0)
		err_sys("read error");
	exit(0);
}
