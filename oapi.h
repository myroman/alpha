#ifndef __OAPI_H
#define __OAPI_H

#include "misc.h"
int msg_send(int callbackFd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery);

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);

SockAddrUn createSockAddrUn(const char* filename);

char* cpyAndMovePtr(char* destPtr, const char* src);
char* addDlm(char* destPtr);

// SERIALIZED MSG_SEND STRING: MSG_TYPE|DESTIP|DESTPORT|MSG|FORCEREDISCOVERY|CALLBACKFD
struct sendDto {
	int msgType;	
	char srcIp[IP_ADDR_LEN];
	int srcPort;
	char destIp[IP_ADDR_LEN];
	int destPort;	
	char* msg;
	int forceRedisc;
}; 
typedef struct sendDto SendDto;
#endif