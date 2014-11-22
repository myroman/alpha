#ifndef __routingTable_h_
#define __routingTable_h_
#include "unp.h"
#include <sys/time.h>

#define	IF_NAME		16	/* same as IFNAMSIZ    in <net/if.h> */
#define	IF_HADDR	 30	/* same as IFHWADDRLEN in <net/if.h> */
#define ROUTING_ENTRY_STALE 2

typedef struct RouteEntry RouteEntry;//typeDef for the Clinet Info object
struct RouteEntry{
	//char dest_ip [IF_NAME];
	in_addr_t dest_ip;
	char next_hop [IF_HADDR];	/* hardware address */
	int hop_count;
	int interfaceInd;
	struct timeval entryTime;
    RouteEntry *right;
    RouteEntry *left;
};


//int addRouteEntry(char * dIP, char * nh, int hc);
int addRouteEntry(in_addr_t dIP, char * nh, int hc, int intFIndex);
int insertOrUpdateRouteEntry(in_addr_t sIP, char * nh, int hc, int intFIndex);
int checkTime(struct timeval * inspect);
void removeRoutingEntry();
//RouteEntry* findAndUpdateRouteEntry(char * destIP);
RouteEntry* findRouteEntry(in_addr_t destIP);
void printRoutingTable();

#endif