#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "debug.h"
#include <string.h>
#include "payloadHdr.h"


char * printIPHuman(in_addr_t ip){
	struct in_addr ipIa;
	ipIa.s_addr = ip;
	return inet_ntoa(ipIa);
}
void printPayloadContents(PayloadHdr *p){
	printf(">> Source IP is %s %u\n",printIPHuman(p->srcIp), p->srcIp);
	printf(">> Destination IP is %s %u\n",printIPHuman(p->destIp), p->destIp);
	printf(">> MsgType: %u, forceRediscovery %u, RREP Sent: %u, HopCount: %u, broadcastId %u, source Port: %u, destPort: %u",
	 p->msgType, p->forceRediscovery, p->rrepSent, p->hopCount, p->broadcastId, p->srcPort, p->destPort);
	if (p->msg != NULL){
		printf(", msgLength: %u, msg:%s\n", (uint32_t)strlen(p->msg), p->msg);
	} else{
		printf("\n");
	}
}

// Basically it's strlen(msg) + 1 for storing '\0'
int resolveCorrectMsgSize(char* msg) {
	if (msg == NULL) {
		return 28;
	}
	int msgSize = strlen(msg) + 1;	
	if (msgSize < 28) {
		msgSize = 28;
	} else if (msgSize > 1482) {
		msgSize = 1482;
	}
	return msgSize;
}

void* packPayload(PayloadHdr* p, uint32_t* bufLen){

	int headerLength = 18 + resolveCorrectMsgSize(p->msg);
	void * msg_ptr = malloc(headerLength);
	bzero(msg_ptr, headerLength);
	
	// Create compact storage in 4 bytes.	
	int tmp = 0;
	tmp = (p->msgType << 30);	
	tmp |= (p->forceRediscovery << 29);
	tmp |= (p->rrepSent << 28);
	tmp |= (p->hopCount << 14);
	tmp |= p->broadcastId;
	//copy info to the head of buffer
	memcpy(msg_ptr, &tmp, 4);	
	insertSrcIp(p->srcIp, msg_ptr);
	insertDestIp(p->destIp, msg_ptr);
	insertSrcPort(p->srcPort, msg_ptr);
	insertDestPort(p->destPort, msg_ptr);
	insertMsgOrFluff(p->msg, msg_ptr);
	debug("Created buffer size:%d", headerLength);

	*bufLen = headerLength;
	return msg_ptr;
}

void unpackPayload(void * buf, PayloadHdr* phdr){
	bzero(phdr, sizeof(PayloadHdr));

	int tmp = 0;
	memcpy(&tmp, buf, 4);
	phdr->msgType = (tmp & 0xc0000000) >> 30;
	phdr->forceRediscovery = (tmp & 0x20000000) >> 29;
	phdr->rrepSent = (tmp & 0x10000000) >> 28;
	phdr->hopCount = (tmp & 0xFFFC000) >> 14;
	phdr->broadcastId = tmp & 0x3FFF;

	phdr->srcPort = extractSrcPort(buf);
	phdr->destPort = extractDestPort(buf);

	phdr->srcIp = extractSrcIp(buf);
	phdr->destIp = extractDestIp(buf);

	uint32_t msgLen = 0;
	memcpy(&msgLen, buf + 16, 2);
	if (msgLen <= 0) {
		return;
	}
	bzero(phdr->msg, msgLen + 1);
	memcpy(phdr->msg, buf + 18, msgLen);
}

void insertSrcIp(in_addr_t ipAddr, void* buf) {
	void* ptr = buf + 4;
	*((in_addr_t*)ptr) = ipAddr;
}

void insertDestIp(in_addr_t ipAddr, void* buf) {
	void* ptr = buf + 8;
	*((in_addr_t*)ptr) = ipAddr;
}

void insertSrcPort(uint32_t sPort, void* buf){	
	uint32_t* ptr = (uint32_t*)(buf + 12);	
	bzero((void*)ptr, 2);
	*ptr = *ptr | (sPort & 0xFFFF);
}

void insertDestPort(uint32_t dPort, void* buf){
	uint32_t* ptr = (uint32_t*)(buf + 14);
	bzero((void*)ptr, 2);
	dPort = dPort & 0xFFFF;	
	*ptr = *ptr | dPort;
}

// should be NULL-term. string!
void insertMsgOrFluff(char* msg, void* buf) {	
	uint32_t* ptr = (uint32_t*)(buf + 16);
	char* msgPtr = (char*)(buf + 18);

	uint32_t strSize;
	if (msg == NULL) {
		strSize = 0;
	} else {
		// +1 byte for '\0'
		strSize = strlen(msg) + 1;
	}
	if (strSize > 1482) {
		strSize = 1482;
	}

	bzero(msgPtr, strSize);
	memcpy(msgPtr, msg, strSize - 1);

	// filling message length (without '\0')
	bzero((void*)ptr, 2);
	*ptr = *ptr | ((strSize-1) & 0xFFFF);
}

in_addr_t extractSrcIp(void* buf) {
	void* ptr;

	ptr = buf + 4;
	return *((in_addr_t*) ptr);
}

in_addr_t extractDestIp(void* buf) {
	void* ptr;

	ptr = buf + 8;
	return *((in_addr_t*) ptr);
}

uint32_t extractSrcPort(void *buf){
	uint32_t tmp;
	memcpy(&tmp, buf + 12, 2);
	tmp = tmp & (0xFFFF);
	return (uint32_t)tmp;
}
uint32_t extractDestPort(void *buf){
	uint32_t tmp;
	memcpy(&tmp, buf + 14, 2);
	tmp = tmp & (0xFFFF);
	return (uint32_t)tmp;
}