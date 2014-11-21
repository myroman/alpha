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

// Argument msg is not a null-terminated string
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

	//Building structure
	PayloadHdr ph;

	bzero(&ph, sizeof(ph));
	if (destPort == SRV_PORT_NUMBER) {
		ph.msgType = CLIENT_MSG_TYPE;
	} else {
		ph.msgType = SRV_MSG_TYPE;
	}
	ph.forceRediscovery = forceRediscovery;		
	if (strcmp(destIpAddr, "loc") == 0) {
		// for ODR this message means "Destination IP = local"
		ph.destIp = LOCAL_INET_IP; // inet_addr("0.1.2.3"); 
	} else {
		ph.destIp = inet_addr(destIpAddr);
	}	
	ph.destPort = destPort;
	//Initially
	int msgSpace = strlen(msg) + 1;
	bzero(ph.msg, msgSpace);
	strcpy(ph.msg, msg);

	printPayloadContents(&ph);

	// Packing and sending
	uint32_t bufLen = 0;
	void* packedBuf = packPayload(&ph, &bufLen);	
	
	SockAddrUn addr = createSockAddrUn(ODR_UNIX_PATH);
	printf("Sending packed buffer, length=%u...", bufLen);	
	int n;
	if ((n = sendto(callbackFd, packedBuf, bufLen, 0, (SA *)&addr, sizeof(addr))) == -1) {
		printFailed();
	} else{
		printOK();
		PayloadHdr pp;
		unpackPayload(packedBuf, &pp);
		printPayloadContents(&pp);
	}
	free(packedBuf);
	return 0;
}

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort) {
	
	fd_set set;
	int maxfd;
	struct timeval tv;
	PayloadHdr ph;
	char* buf = malloc(ETHFR_MAXDATA_LEN);
	for(;;){
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		maxfd = sockfd+1;
		select(maxfd, &set, NULL, NULL, &tv);
		if(FD_ISSET(sockfd, &set)){
			// let's choose some smaller value than 1500 bytes.
			bzero(buf, ETHFR_MAXDATA_LEN);
			printf("Waiting for request...");
			int length = recvfrom(sockfd, buf, ETHFR_MAXDATA_LEN, 0, NULL, NULL);
			if (length == -1) { 
				printFailed();
				free(buf);
				return length;
			}
			printOK();
			debug("Got packed payload, length = %d", length);
			
			unpackPayload(buf, &ph);
			strcpy(srcIpAddr, printIPHuman(ph.srcIp));
			*srcPort = ph.srcPort;
			strcpy(msg, ph.msg);

			free(buf);
			return length;
		}
		free(buf);
		debug("Nothing read. Timeout.");
		return -1;//timeout
	}
}
