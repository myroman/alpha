#include "unp.h"
#include "misc.h"
#include "debug.h"
#include "oapi.h"
#include "payloadHdr.h"

SockAddrUn createSockAddrUn(const char* filename){
	SockAddrUn addr;
	bzero(&addr, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, filename);
	return addr;
}

char* itostr(int val){
	char* res = malloc(10);
	sprintf(res, "%d\0", val);
	return res;
}

char* cpyAndMovePtr(char* destPtr, const char* src) {
	strcpy(destPtr, src);
	destPtr += strlen(src);
	return destPtr;
}
char* addDlm(char* destPtr) {
	char* delimiter = "|\0";
	return cpyAndMovePtr(destPtr, delimiter);
}

// SERIALIZED MSG_SEND STRING: MSG_TYPE|DESTIP|DESTPORT|MSG|FORCEREDISCOVERY|CALLBACKFD
int msg_send(int callbackFd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery) {
	if (callbackFd <= 0) {
		debug("API: callbackFd should be positive");
		return -1;
	}
	if (destIpAddr == NULL) {
		debug("API: destIpAddr can't be NULL");
		return -1;
	}
	if (destPort <= 0) {
		debug("API: destPort should be positive");
		return -1;
	}
	if (msg == NULL) {
		debug("API: msg can't be NULL");
		return -1;
	}

	//Packing stage
	PayloadHdr ph;

	bzero(&ph, sizeof(ph));
	if (destPort == SRV_PORT_NUMBER) {
		ph.msgType = CLIENT_MSG_TYPE;
	} else {
		ph.msgType = SRV_MSG_TYPE;
	}
	ph.forceRediscovery = forceRediscovery;	
	ph.srcIp = inet_addr("127.1.1.1");

	ph.destIp = inet_addr("0.1.2.3");
	ph.destPort = destPort;
	ph.msg = malloc(strlen(msg));	
	strcpy(ph.msg, msg);	

	uint32_t bufLen = 0;
	void* packedBuf = packPayload(&ph, &bufLen);	
	
	SockAddrUn addr = createSockAddrUn(ODR_UNIX_PATH);
	debug("Sending packed buffer, length=%u...", bufLen);	
	int n = sendto(callbackFd, packedBuf, bufLen, 0, (SA *)&addr, sizeof(addr));
	debug("Sent it, result=%d", n);
	free(packedBuf);
	return 0;
}

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort) {
	
	fd_set set;
	int maxfd;
	struct timeval tv;
	for(;;){
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		maxfd = sockfd+1;
		select(maxfd, &set, NULL, NULL, &tv);
		if(FD_ISSET(sockfd, &set)){
			// let's choose some smaller value than 1500 bytes.
			char* buf = malloc(ETHFR_MAXDATA_LEN);
			int length = recvfrom(sockfd, buf, ETHFR_MAXDATA_LEN, 0, NULL, NULL);
			if (length == -1) { 
				debug("%s\n", "Length=-1");
				free(buf);
				return length;
			}			

			debug("Got packed payload, length = %d", length);
			PayloadHdr* ph = unpackPayload(buf);
			strcpy(srcIpAddr, ph->srcIp);
			*srcPort = ph->srcPort;
			strcpy(msg, ph->msg);

			free(buf);
			free(ph);
			return length;
		}
		debug("Nothing read. timeout.");
		return -1;//timeout
	}
}
