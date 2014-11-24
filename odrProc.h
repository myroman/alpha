#ifndef __odr_h_
#define __odr_h_

#include "oapi.h"
#include "misc.h"
#include "payloadHdr.h"
#include "hw_addrs.h"
#include "routingTable.h"
#include <linux/if_ether.h>

#define PROTOCOL_NUMBER 0x2457
	
struct frameUserData {
	char ipAddr[IP_ADDR_LEN];
	int portNumber;
	char* msg;
	char srcIpAddr[IP_ADDR_LEN];
	int srcPortNumber;
};
typedef struct frameUserData FrameUserData;

typedef struct networkInterface NetworkInterface;
struct networkInterface {
	SockAddrLl* sockAddr;
	unsigned char macAddress[ETH_ALEN];
	int interfaceIndex;
	in_addr_t ipAddr;
	int isEth0;
	int isLo;
	NetworkInterface* next;
	int sd;

};

// odrProc.c
void* respondToHostRequestsRoutine (void *arg);
//void respondToHostRequestsRoutine (int unixDomainFd, int odrSockFd);
void* respondToNetworkRequestsRoutine (void *arg);
//void respondToNetworkRequestsRoutine (int unixDomainFd, int odrSockFd);
char* ut();
char* nt();
void fillInterfaces();

void rrepBack(int rawSockFd, PayloadHdr respH, unsigned char currentMac[ETH_ALEN], SockAddrLl recvAddr);
void sendToRoute(int rawSockFd, PayloadHdr ph, unsigned char currentMac[ETH_ALEN], RouteEntry destEntry);
void flood(int rawSockFd, PayloadHdr ph, unsigned char currentMac[ETH_ALEN], SockAddrLl senderAddr);
int odrSend(int sockfd, PayloadHdr ph, unsigned char srcMac[6], unsigned char destMac[6], int destInterfaceIndex);
int addCurrentNodeAddressAsSource(PayloadHdr* dto);
void handlePacketAtDestinationNode(int unixDomainFd, PayloadHdr* ph);
int handleLocalDestMode(PayloadHdr* dto);
void handleIncomingPacket(int rawSockFd, int unixDomainFd, PayloadHdr ph, NetworkInterface* currentNode, SockAddrLl sndAddr);
NetworkInterface* getCurrentNodeInterface();

// odrImpl.c
void serialFrameUdata(FrameUserData dto, unsigned char* out);
void deserialFrameUdata(char* src, FrameUserData* out);
char* cpyAndMovePtr2(unsigned char* destPtr, const char* src);
char* addDlm2(unsigned char* destPtr);
char* itostr2(int val);

void printMac(unsigned char mac[6]);
int createOdrSocket();

#endif