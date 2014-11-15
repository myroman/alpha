#ifndef __OAPI_H
#define __OAPI_H
int msg_send(int callbackFd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery);

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);

struct sockaddr_un createSA(const char* filename);
#endif