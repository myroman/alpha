#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
//#include <strings.h>
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
	printf("MsgType: %u, forceRediscovery %u, RREP Sent: %u, HopCount: %u, broadcastId %u, source Port: %u, destPort: %u\n",
	 p->msgType, p->forceRediscovery, p->rrepSent, p->hopCount, p->broadcastId, p->srcPort, p->destPort);
}

void* createNewMsg(PayloadHdr* p ){
	void * msg_ptr = malloc(17);
	insertType(p->msgType, msg_ptr);
	insertFD(p->forceRediscovery, msg_ptr);
	insertRREQ(p->rrepSent, msg_ptr);
	insertHopCount(p->hopCount, msg_ptr);
	insertBroadCastID(p->broadcastId, msg_ptr);
	
	insertSrcIp(p->srcIp, msg_ptr);
	insertDestIp(p->destIp, msg_ptr);

	insertSrcPort(p->srcPort, msg_ptr);
	insertDestPort(p->destPort, msg_ptr);
	return msg_ptr;
}

PayloadHdr * extractMsgContents(void * buf){
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

	return phdr;
}


/*
int main (){
	void * msg_ptr = malloc(17);
	//insertType(2,msg_ptr );
	
	//PayloadHdr* p = (PayloadHdr*) malloc(17);
	//p->msgType = 2;
	//insertType(p->msgType, msg_ptr);
	//extractType(msg_ptr);

	PayloadHdr* p = (PayloadHdr*) malloc(sizeof(PayloadHdr));
	p->msgType = 2;
	p->forceRediscovery = 1;
	p->rrepSent = 0;
	p->hopCount = 10;
	p->broadcastId = 20;
	p->srcPort = 50001;
	p->destPort = 12456;
	p->srcIp = inet_addr("127.0.0.1");
	p->destIp = inet_addr("255.4.2.1");

	void * msgPayload = createNewMsg(p);
	debug("Here");	
	debug("Asdf");

	PayloadHdr* extractPhdr = extractMsgContents(msgPayload);
	printPayloadContents(extractPhdr);



	/*void* buf = malloc(17);
	void* firstPart = buf;
	//void* ports = buf + 12;
	//first 4 bytes
	uint32_t result;
	insertType(2, buf);
	insertFD(1, buf);
	insertRREQ(1, buf);
	insertHopCount(10, buf);
	insertBroadCastID(20, buf);
	((uint32_t*)firstPart) = result;
	void* testPart1 = malloc(4);
	printf("What I put into buf is %u. The value before is %u\n", (uint32_t)*((uint32_t*)testPart1), result);
	extractType(buf);
	extractFD(buf);
	extractRREQ(buf);
	extractHopCount(buf);
	extractBroadCast(buf);


	//Sourc IP
	in_addr_t ip = inet_addr("127.0.0.1");
	insertSrcIp(ip, buf);
	struct in_addr srcIpIa;
	srcIpIa.s_addr = extractSrcIp(buf);
	printf("Src IP I put into buf is %s\n", inet_ntoa(srcIpIa));

	//Dest IP
	in_addr_t ipDest = inet_addr("255.255.128.192");
	insertDestIp(ipDest, buf);
	struct in_addr destIpIa;
	destIpIa.s_addr = extractDestIp(buf);
	printf("Dest IP I put into buf is %s\n", inet_ntoa(destIpIa));

	//the fourth 4 byte pair
	insertSrcPort(50001, buf);
	insertDestPort(61234, buf);
	extractSrcPort(buf);
	extractDestPort(buf);
    */
	
//}


void insertType(uint32_t type, void* buf){
	uint32_t * result = (uint32_t *)malloc(4);
	//uint32_t result;
	memcpy(result, buf, 4);
	type = type << 30;
	printf("type shifted to the left by 30: %u\n", type);
	*result = *result | type;
	printf("result = %u, buf=%u\n", *result, *((uint32_t*)buf));
	memcpy(buf, result, 4);
	debug("here");
	free(result);
	debug("here2");
}


void insertFD(uint32_t fd, void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	fd = fd << 29;
	printf("type shifted to the left by 29: %u\n", fd);
	*result = *result | fd;
	printf("resutlt = %u\n", *result);
	memcpy(buf, result, 4);
	free(result);
}

void insertRREQ(uint32_t r, void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	r = r << 28;
	printf("type shifted to the left by 28: %u\n", r);
	*result = *result | r;
	printf("resutlt = %u\n", *result);
	memcpy(buf, result, 4);
	free(result);
}
void insertHopCount(uint32_t hopCount , void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	hopCount = hopCount << 14;
	printf("type shifted to the left by 14: %u\n", hopCount);
	*result = *result | hopCount;
	printf("resutlt = %u\n", *result);
	memcpy(buf, result, 4);
	free(result);
}

void insertBroadCastID(uint32_t bID , void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	//printf("type shifted to the left by 10: %u\n", bID);
	*result = *result | bID;
	printf("resutlt = %u\n", *result);
	memcpy(buf, result, 4);
	free(result);
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

void insertSrcIp(in_addr_t ipAddr, void* buf) {
	void* ptr = buf + 4;
	*((in_addr_t*)ptr) = ipAddr;
}

in_addr_t extractSrcIp(void* buf) {
	void* ptr;

	ptr = buf + 4;
	return *((in_addr_t*) ptr);
}

void insertDestIp(in_addr_t ipAddr, void* buf) {
	void* ptr = buf + 8;
	*((in_addr_t*)ptr) = ipAddr;
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
	printf("The source port is %u\n", result);
	free(ptr);
	return result;
}
uint32_t extractDestPort(void *buf){
	void* ptr = malloc(2);
	memcpy(ptr, buf + 14, 2);
	uint32_t result = *((uint32_t*)ptr);
	result = result & (0xFFFF);
	printf("The dest port is %u\n", result);
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
	printf("the type field is %u\n", result);
	return result;
}

uint32_t extractFD(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x20000000);
	result = result >> 29;
	printf("the FD field is %u\n", result);
	return result;
}

uint32_t extractRREQ(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x10000000);
	result = result >> 28;
	printf("the RREQ field is %u\n", result);
	return result;
}

uint32_t extractHopCount(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0xFFFC000);
	result = result >> 14;
	printf("the hop count field is %u\n", result);
	return result;
}

uint32_t extractBroadCast(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x3FFF);
	printf("the broadcast id field is %u\n", result);
	return result;
}
