#ifndef __payloadhdr_h
#define __payloadhdr_h

#include "misc.h"
#include "oapi.h"

#define MT_RREQ 0
#define MT_RREP 1
#define MT_PLD 2

typedef struct PayloadHdr PayloadHdr;
struct PayloadHdr {
	uint32_t msgType; // takes values of MT_ defines.
	uint32_t forceRediscovery;
	uint32_t rrepSent;
	uint32_t hopCount;
	uint32_t broadcastId;

	in_addr_t srcIp;
	in_addr_t destIp;

	uint32_t srcPort;
	uint32_t  destPort;

	char msg[MAX_PAYLD_MSG_LEN];
};

// Interface of this file
void* packPayload(PayloadHdr* p, uint32_t* bufLen);
void unpackPayload(void* buf, PayloadHdr* ph);
void printPayloadContents(PayloadHdr *ph);

// for inner use
void insertSrcPort(uint32_t sPort, void* buf);
void insertDestPort(uint32_t dPort, void* buf);
void insertSrcIp(in_addr_t ipAddr, void* buf);
void insertDestIp(in_addr_t ipAddr, void* buf);
// msg should be NULL-terminated
void insertMsgOrFluff(char* msg, void* buf);

in_addr_t extractSrcIp(void* buf);
in_addr_t extractDestIp(void* buf);
uint32_t extractSrcPort(void *buf);
uint32_t extractDestPort(void *buf);
//return NULL-terminated msg
char* extractMsg(void* buf);


PayloadHdr convertToResponse(PayloadHdr req);
#endif