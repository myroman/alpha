#include "unp.h"
#include "misc.h"

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
		printf("API: callbackFd should be positive\n");
		return -1;
	}
	if (destIpAddr == NULL) {
		printf("API: destIpAddr can't be NULL\n");
		return -1;
	}
	if (destPort <= 0) {
		printf("API: destPort should be positive\n");
		return -1;
	}
	if (msg == NULL) {
		printf("API: msg can't be NULL\n");
		return -1;
	}

	//Serialization stage

	char* serialized = malloc(MAXLINE);
	char* ptrPaste = serialized;
	char* tmp2 = itostr(destPort);	
	char* tmp3 = itostr(forceRediscovery);	
	char* msgType = itostr(CLIENT_MSG_TYPE);/* ODR should know not only these args, but also type of request {send,recv} */
	char* cbfd = itostr(callbackFd);

	ptrPaste = cpyAndMovePtr(ptrPaste, msgType);
	ptrPaste = addDlm(ptrPaste);


	printf("len of ip:%d\n",strlen(destIpAddr));
	ptrPaste = cpyAndMovePtr(ptrPaste, destIpAddr);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, tmp2);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, msg);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, tmp3);
	ptrPaste = addDlm(ptrPaste);
	ptrPaste = cpyAndMovePtr(ptrPaste, cbfd);
	ptrPaste = addDlm(ptrPaste);

	SockAddrUn mysa;
	socklen_t l = sizeof(mysa);
	bzero(&mysa, l);
	Getsockname(callbackFd, (SockAddrUn *)&mysa, &l);
	printf("Filepath bound to fd %d is %s\n", callbackFd, mysa.sun_path);	
	
	ptrPaste = cpyAndMovePtr(ptrPaste, mysa.sun_path);

	ptrPaste = cpyAndMovePtr(ptrPaste, "\0");
	
	free(cbfd);
	free(tmp2);
	free(tmp3);
	free(msgType);

	printf("Serialized into byte array: %s\n", serialized);
	
	SockAddrUn addr = createSockAddrUn(ODR_UNIX_PATH);
	printf("Send to %d len %d\n", callbackFd, strlen(serialized));	
	sendto(callbackFd, serialized, strlen(serialized), 0, (SA *)&addr, sizeof(addr));
	return 0;
}

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort) {
	void* buffer = (void*)malloc(ETH_FRAME_LEN); /*Buffer for ethernet frame*/
	int length = 0; /*length of the received frame*/ 
	
	length = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
	if (length == -1) { 
		printf("%s\n", "Length=-1");
	}

	printf("Got message\n");
}