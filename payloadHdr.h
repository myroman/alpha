#include "oapi.h"

struct payloadHdr {
	int msgType;
	int forceRediscovery;
	int rrepSent;
	int hopCount;
	int broadcastId;

	in_addr_t srcIp;
	in_addr_t destIP;

	char srcPort;
	char destPort;
};