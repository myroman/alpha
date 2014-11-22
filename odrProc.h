#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "payloadHdr.h"
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
void fillInterfaces();

int odrSend(int sockfd, PayloadHdr* ph, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex);
int odrRecv(int sockfd, PayloadHdr* ph, void* buffer);
int addCurrentNodeAddressAsSource(PayloadHdr* dto);
void handlePacketAtDestinationNode(PayloadHdr* ph, int unixDomainFd);
int handleLocalDestMode(PayloadHdr* dto);

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	unsigned char macAddress[IF_HADDR];
	int interfaceIndex;
	in_addr_t ipAddr;
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

void printMac(unsigned char mac[6]);
int createOdrSocket();
#define PROTOCOL_NUMBER 51235
#endif