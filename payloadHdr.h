#ifndef __payloadhdr_h
#define __payloadhdr_h

#include "oapi.h"
typedef struct PayloadHdr PayloadHdr;
struct PayloadHdr {
	uint32_t msgType;
	uint32_t forceRediscovery;
	uint32_t rrepSent;
	uint32_t hopCount;
	uint32_t broadcastId;

	in_addr_t srcIp;
	in_addr_t destIp;

	uint32_t srcPort;
	uint32_t  destPort;

	char* msg;
};

void* packPayload(PayloadHdr* p );
PayloadHdr* unpackPayload(void * buf);

void insertSrcPort(uint32_t sPort, void* buf);
void insertDestPort(uint32_t dPort, void* buf);
void insertSrcIp(in_addr_t ipAddr, void* buf);
void insertDestIp(in_addr_t ipAddr, void* buf);
void insertMsgOrFluff(char* msg, void* buf);

in_addr_t extractSrcIp(void* buf);
in_addr_t extractDestIp(void* buf);
uint32_t extractSrcPort(void *buf);
uint32_t extractDestPort(void *buf);
char* extractMsg(void* buf);

char * printIPHuman(in_addr_t ip);

#endif