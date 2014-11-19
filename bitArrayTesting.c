#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <strings.h>

in_addr_t extractSrcIp(void* buf);
in_addr_t extractDestIp(void* buf);
int main (){
	void* buf = malloc(17);
	void* firstPart = buf;
	//void* ports = buf + 12;
	//first 4 bytes
	uint32_t result;
	insertType(2, buf);
	insertFD(0, buf);
	insertRREQ(1, buf);
	insertHopCount(10, buf);
	insertBroadCastID(20, buf);
	*((uint32_t*)firstPart) = result;
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

	
}

void insertType(uint32_t type, void* buf){
	uint32_t* result = malloc(4);
	memcpy(result, buf, 4);
	
	type = type << 30;
	printf("type shifted to the left by 30: %u\n", type);
	*result = *result | type;
	printf("result = %u, buf=%u\n", *result, *((uint32_t*)buf));
	memcpy(buf, result, 4);
	free(result);
}

int extractType(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0xc0000000);
	result = result >> 30;
	printf("the type field is %u\n", result);
	return result;
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

int extractSrcPort(void *buf){
	void* ptr = malloc(2);
	memcpy(ptr, buf + 12, 2);
	uint32_t result = *((uint32_t*)ptr);
	result = result & (0xFFFF);
	printf("The source port is %u\n", result);
	free(ptr);
}
int extractDestPort(void *buf){
	void* ptr = malloc(2);
	memcpy(ptr, buf + 14, 2);
	uint32_t result = *((uint32_t*)ptr);
	result = result & (0xFFFF);
	printf("The dest port is %u\n", result);
	free(ptr);
}

int extractFD(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x20000000);
	result = result >> 29;
	printf("the FD field is %u\n", result);
	return result;
}

int extractRREQ(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x10000000);
	result = result >> 28;
	printf("the RREQ field is %u\n", result);
	return result;
}

int extractHopCount(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0xFFFC000);
	result = result >> 14;
	printf("the hop count field is %u\n", result);
	return result;
}

int extractBroadCast(void* buf){
	uint32_t* ptr = malloc(4);
	memcpy(ptr, buf, 4);
	uint32_t result = *ptr;
	free(ptr);
	result = result & (0x3FFF);
	printf("the broadcast id field is %u\n", result);
	return result;
}
