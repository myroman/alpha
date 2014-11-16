#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "hw_addrs.h"

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6]);
int odrRecv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	unsigned char macAddress[IF_HADDR];
	int interfaceIndex;
	NetworkInterface* next;
};

#endif