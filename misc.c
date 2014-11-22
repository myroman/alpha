#include "unp.h"
#include "misc.h"

void rmnl(char* s) {
	int ln = strlen(s) - 1;
	if (s[ln] == '\n')
	    s[ln] = '\0';
	printf("%c\n",s[ln]);	
}

char* createTmplFilename(){
	char*s = malloc(10);
	s[0]='r';
	s[1]='O';
	s[2]='m';
	s[3]='X';
	s[4]='X';
	s[5]='X';
	s[6]='X';
	s[7]='X';
	s[8]='X';
	s[9]='\0';
	return s;
}

void printOK() {
	printf("OK\n");
}
void printFailed() {
	printf("FAILED: %s\n", strerror(errno));
}

char * printIPHuman(in_addr_t ip){
	struct in_addr ipIa;
	ipIa.s_addr = ip;
	return inet_ntoa(ipIa);
}