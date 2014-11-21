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
	printf("Source IP is %s %u\n",printIPHuman(p->srcIp), p->srcIp);
	printf("Destination IP is %s %u\n",printIPHuman(p->destIp), p->destIp);
	printf("MsgType: %u, forceRediscovery %u, RREP Sent: %u, HopCount: %u, broadcastId %u, source Port: %u, destPort: %u",
	 p->msgType, p->forceRediscovery, p->rrepSent, p->hopCount, p->broadcastId, p->srcPort, p->destPort);
	if (p->msg != NULL){
		printf(", msgLength: %u, msg:%s\n", (uint32_t)strlen(p->msg), p->msg);
	} else{
		printf("\n");
	}
}

void* createPayloadBuf(PayloadHdr* p, int* bufLen){
	int msgLen = 28;
	if (p->msg != NULL) {
		msgLen = strlen(p->msg);	
		if (msgLen < 28) {
			msgLen = 28;
		} else if (msgLen > 1482) {
			msgLen = 1482;
		}
	}
	int headerLength = 18 + msgLen;
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
	return msg_ptr;
}

PayloadHdr * extractPayloadContents(void * buf){
	PayloadHdr *phdr = (PayloadHdr *) malloc(sizeof(PayloadHdr)); 	

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
	phdr->msg = extractMsg(buf);

	return phdr;
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
	*ptr = *ptr | (dPort & 0xFFFF);
}

// should be NULL-term. string!
void insertMsgOrFluff(char* msg, void* buf) {	
	uint32_t* ptr = (uint32_t*)(buf + 16);
	uint32_t* msgPtr = (uint32_t*)(buf + 18);
	bzero((void*)ptr, 2);

	uint32_t msgLength;
	if (msg == NULL) {
		msgLength = 0;
	} else {
		msgLength = strlen(msg);
	}	
	debug("l of msg:%d", (int)msgLength);
	if (msgLength <= 28) {
		//copy message
		uint32_t padSize = 28 - msgLength;
		memcpy(msgPtr, msg, msgLength);
		
		//pad it
		msgPtr = msgPtr + msgLength;
		memset(msgPtr, 0xFF, padSize);
	} 
	else if (msgLength > 1482) {
		msgLength = 1482;
		memcpy(msgPtr, msg, msgLength);
	} else {
		memcpy(msgPtr, msg, msgLength);
	}
	*ptr = *ptr | (msgLength & 0xFFFF);
}

char* extractMsg(void* buf) {
	uint32_t* ptr = (uint32_t*)(buf + 16),
		*msgPtr = (uint32_t*)(buf + 18);	
	
	uint32_t msgLen = 0;
	memcpy(&msgLen, buf + 16, 2);
	
	if (msgLen <= 0) {
		return NULL;
	}

	char* msg = (char*)malloc(msgLen);
	memcpy(msg, buf + 18, msgLen);
	return msg;
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
	int16_t tmp;
	memcpy(&tmp, buf + 12, 2);
	tmp = tmp & (0xFFFF);
	
	return (uint32_t)tmp;
}
uint32_t extractDestPort(void *buf){
	int16_t tmp;
	memcpy(&tmp, buf + 14, 2);
	tmp = tmp & (0xFFFF);
	return (uint32_t)tmp;
}