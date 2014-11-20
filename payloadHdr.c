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

	insertType(p->msgType, msg_ptr);
	insertFD(p->forceRediscovery, msg_ptr);
	insertRREQ(p->rrepSent, msg_ptr);
	insertHopCount(p->hopCount, msg_ptr);
	insertBroadCastID(p->broadcastId, msg_ptr);
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
	
	phdr->msgType = extractType(buf);
	phdr->forceRediscovery = extractFD(buf);
	phdr->rrepSent = extractRREQ(buf);
	phdr->hopCount = extractHopCount(buf);
	phdr->broadcastId = extractBroadCast(buf);

	phdr->srcPort = extractSrcPort(buf);
	phdr->destPort = extractDestPort(buf);

	phdr->srcIp = extractSrcIp(buf);
	phdr->destIp = extractDestIp(buf);
	phdr->msg = extractMsg(buf);

	return phdr;
}

void insertType(uint32_t type, void* buf){
	uint32_t * result = (uint32_t *)malloc(4);
	//uint32_t result;
	memcpy(result, buf, 4);
	type = type << 30;
	*result = *result | type;
	memcpy(buf, result, 4);	
	free(result);
}


void insertFD(uint32_t fd, void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	fd = fd << 29;
	*result = *result | fd;
	memcpy(buf, result, 4);
	free(result);
}

void insertRREQ(uint32_t r, void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	r = r << 28;
	*result = *result | r;
	memcpy(buf, result, 4);
	free(result);
}
void insertHopCount(uint32_t hopCount , void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	hopCount = hopCount << 14;
	*result = *result | hopCount;
	memcpy(buf, result, 4);
	free(result);
}

void insertBroadCastID(uint32_t bID , void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	*result = *result | bID;
	memcpy(buf, result, 4);
	free(result);
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
	
	uint32_t* pLen = (uint32_t*)malloc(2);
	memcpy(pLen, buf + 16, 2);
	uint32_t msgLen = *pLen;
	free(pLen);
	
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
	void* ptr = malloc(2);
	memcpy(ptr, buf + 12, 2);
	uint32_t result = *((uint32_t*)ptr);
	result = result & (0xFFFF);
	free(ptr);
	return result;
}
uint32_t extractDestPort(void *buf){
	void* ptr = malloc(2);
	memcpy(ptr, buf + 14, 2);
	uint32_t result = *((uint32_t*)ptr);
	result = result & (0xFFFF);
	free(ptr);
	return result;
}
uint32_t extractType(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0xc0000000);
	result = result >> 30;
	return result;
}

uint32_t extractFD(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x20000000);
	result = result >> 29;
	return result;
}

uint32_t extractRREQ(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x10000000);
	result = result >> 28;
	return result;
}

uint32_t extractHopCount(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0xFFFC000);
	result = result >> 14;
	return result;
}

uint32_t extractBroadCast(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x3FFF);
	return result;
}
