#ifndef __MISC_H
#define __MISC_H

#define CLIENT_MSG_TYPE 0
#define RECV_MSG_TYPE 1

//Ethernet frame positions
#define ETH_FRAME_LEN 1518

#define ETHFR_MAXDATA_LEN 1500

#define ODR_UNIX_PATH "TonyRomanOdr"
#define SRV_UNIX_PATH "TonyRomanSrv"
#define SRV_PORT_NUMBER 51248

void rmnl(char* s);
char* createTmplFilename();

typedef struct sockaddr_un SockAddrUn;
typedef struct sockaddr_ll SockAddrLl;
#endif