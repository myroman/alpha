#ifndef __MISC_H
#define __MISC_H

#define CLIENT_MSG_TYPE 0
#define SRV_MSG_TYPE 1

//Ethernet frame positions
#define IP_ADDR_LEN 16
#define PORT_DIG_LEN 5
#define ETH_FRAME_LEN 1518

#define ETHFR_MAXDATA_LEN 1500
#define MAX_PAYLD_MSG_LEN 1482

#define ETH_MAX_MSG_LEN (ETHFR_MAXDATA_LEN - IP_ADDR_LEN - PORT_DIG_LEN)

#define ODR_UNIX_PATH "TonyRomanOdr"
#define SRV_UNIX_PATH "TonyRomanSrv"
#define SRV_PORT_NUMBER 51248
#define LOCAL_INET_IP 50462976

#define ROMAN_IP_TEST "10.0.2.15\0"

void rmnl(char* s);
char* createTmplFilename();
void printOK();
void printFailed();
char * printIPHuman(in_addr_t ip);

typedef struct sockaddr_un SockAddrUn;
typedef struct sockaddr_ll SockAddrLl;
#endif