#ifndef __odr_h_
#define __odr_h_

int odrSend(int sockfd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery, unsigned char srcMac[6], unsigned char destMac[6]);
int odrRecv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);

#endif