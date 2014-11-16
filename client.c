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
	
	if ( (lstFd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");
	Signal(SIGINT, sig_int);

	// Setup listening filename
	listenFn = malloc(10);
	strncpy(listenFn, createTmplFilename(), 10);
	mkstemp(listenFn);
	unlink(listenFn); //don't remove it. It helps to resolve filename later.

	SockAddrUn addr = createSockAddrUn(listenFn);
	bind(lstFd, (SockAddrUn *)&addr, sizeof(addr));
	
	char* s = malloc(ETHFR_MAXDATA_LEN);
	strcpy(s, "Hi!\n");
	rmnl(s);	
	int forceRediscovery = 0, res;

	res = msg_send(lstFd, ROMAN_IP_TEST, SRV_PORT_NUMBER, s, forceRediscovery);

	//return;
	printf("Requested time...\n");
	char* destIpAddr, *timestamp = malloc(ETH_MAX_MSG_LEN);
	int destPort;
	while ((n = msg_recv(lstFd, timestamp, destIpAddr, &destPort))) {
	printf("asdf\n");
		printf("Timestamp: %s", timestamp);
		return;
		sleep(2);
	}
	
	if (n < 0)
		err_sys("read error");
	exit(0);
}
