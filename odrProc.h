#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "hw_addrs.h"

struct frameUserData {
	char* ipAddr;
	int portNumber;
	char* msg;
};
typedef struct frameUserData FrameUserData;

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6]);
int odrRecv(int sockfd, FrameUserData* userData);

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	unsigned char macAddress[IF_HADDR];
	int interfaceIndex;
	NetworkInterface* next;
};

void serialFrameUdata(FrameUserData dto, unsigned char* out);
void deserialFrameUdata(char* src, FrameUserData* out);
char* cpyAndMovePtr2(unsigned char* destPtr, const char* src);
char* addDlm2(unsigned char* destPtr);
char* itostr2(int val);
#endif