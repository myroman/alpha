#include "unp.h"
#include "misc.h"
#include "debug.h"

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

	//Serialization stage

	char* serialized = malloc(MAXLINE);
	char* ptrPaste = serialized;
	char* destPortS = itostr(destPort);	
	char* forceRed = itostr(forceRediscovery);	
	char* msgType;
	if (destPort == SRV_PORT_NUMBER) {
		msgType = itostr(CLIENT_MSG_TYPE);
	} else {
		msgType = itostr(SRV_MSG_TYPE);
	}
	char* cbfd = itostr(callbackFd);
	
	ptrPaste = cpyAndMovePtr(ptrPaste, msgType);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, destIpAddr);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, destPortS);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, msg);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, forceRed);

	ptrPaste = cpyAndMovePtr(ptrPaste, "\0");
	free(cbfd);
	free(destPortS);
	free(forceRed);
	free(msgType);
	debug("Serialized into byte array: %s", serialized);
	
	SockAddrUn addr = createSockAddrUn(ODR_UNIX_PATH);
	sendto(callbackFd, serialized, strlen(serialized), 0, (SA *)&addr, sizeof(addr));
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
			char* buf = malloc(ETH_MAX_MSG_LEN);
			int length = recvfrom(sockfd, buf, ETH_MAX_MSG_LEN, 0, NULL, NULL);
			if (length == -1) { 
				debug("%s\n", "Length=-1");
				return length;
			}
			strcpy(msg, buf);

			debug("Got message %s", msg);
			return length;
		}
		debug("Nothing read. timeout.");
		return -1;//timeout
	}
	
}