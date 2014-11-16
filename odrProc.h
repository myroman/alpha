#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "hw_addrs.h"

struct frameUserData {
	char ipAddr[IP_ADDR_LEN];
	int portNumber;
	char* msg;
	char srcIpAddr[IP_ADDR_LEN];
	int srcPortNumber;
};
typedef struct frameUserData FrameUserData;
// odrProc.c
void* respondToHostRequestsRoutine (void *arg);
void* respondToNetworkRequestsRoutine (void *arg);
char* ut();
char* nt();
int deserializeApiReq(char* buffer, size_t bufSz, SendDto* dto);
void fillInterfaces();

int odrSend(SendDto* dto, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex);
int odrRecv(int sockfd, FrameUserData* userData);
int addCurrentNodeAddressAsSource(SendDto* dto);
void handlePacketAtDestinationNode(FrameUserData* userData, int unixDomainFd);

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	unsigned char macAddress[IF_HADDR];
	int interfaceIndex;
	char ipAddr[IP_ADDR_LEN];
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

#define PROTOCOL_NUMBER 51235
#endif