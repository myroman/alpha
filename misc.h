#ifndef __MISC_H
#define __MISC_H

#define DAYTIME_PORT 51234

#define SEND_MSG_TYPE 0
#define RECV_MSG_TYPE 1

//Ethernet frame positions
#define ETH_FRAME_LEN 1518

#define ETHFR_MAXDATA_LEN 1500

void rmnl(char* s);

#endif