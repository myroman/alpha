#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "hw_addrs.h"

struct frameUserData {
	char* ipAddr;
	int portNumber;
	char* msg;
	char* srcIpAddr;
	int srcPortNumber;
};
typedef struct frameUserData FrameUserData;
// odrProc.c
int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6]);
int odrRecv(int sockfd, FrameUserData* userData);
int addCurrentNodeAddressAsSource(SendDto* dto);
void handlePacketAtDestinationNode(FrameUserData* userData, int unixDomainFd);

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	unsigned char macAddress[IF_HADDR];
	int interfaceIndex;
	char* ipAddr;
	int isEth0;
	NetworkInterface* next;
};
NetworkInterface* getCurrentNodeInterface();

// odrImpl.c
void serialFrameUdata(FrameUserData dto, unsigned char* out);
void deserialFrameUdata(char* src, FrameUserData* out);
char* cpyAndMovePtr2(unsigned char* destPtr, const char* src);
char* addDlm2(unsigned char* destPtr);
char* itostr2(int val);
#endif