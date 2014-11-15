#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6]);
int odrRecv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);

#endif