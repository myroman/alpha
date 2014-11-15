#include "unp.h"
#include "misc.h"
#include "oapi.h"

int main(int argc, char **argv)
{
	int	lstFd, n;
	socklen_t len;
	char recvline[MAXLINE + 1];
	struct sockaddr_un cliaddr;
	if ( (lstFd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");

	// Setup listening filename
	char* listenFn = malloc(10);
	strncpy(listenFn, createTmplFilename(), 10);
	mkstemp(listenFn);
	cliaddr = createSA(listenFn);

	bind(lstFd, (SA *)&cliaddr, sizeof(cliaddr));
	
	char* s = malloc(ETHFR_MAXDATA_LEN);
	strcpy(s, "Hi!\n");
	rmnl(s);	
	int forceRediscovery = 0, res;

	res = msg_send(lstFd, "192.168.123.123\0", DAYTIME_PORT, s, forceRediscovery);	

	unlink(listenFn);
	
	printf("Requested time...\n");
	char* destIpAddr;
	int destPort;
	while ((n = msg_recv(lstFd, recvline, destIpAddr, &destPort))) {
		printf("Got a response, n = %d\n", n);
		recvline[n] = 0;	
		printf("Timestamp: %s", recvline);		
	}
	
	if (n < 0)
		err_sys("read error");

	unlink(listenFn);
	exit(0);
}
