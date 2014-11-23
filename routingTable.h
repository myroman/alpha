#ifndef __routingTable_h_
#define __routingTable_h_
#include "unp.h"
#include <sys/time.h>
#include <linux/if_ether.h>

#define	IF_NAME		16	/* same as IFNAMSIZ    in <net/if.h> */
//#define	ETH_ALEN	 30	/* sa	me as IFHWADDRLEN in <net/if.h> */
//#define ROUTING_ENTRY_STALE 2
extern int STALENESS;
typedef struct RouteEntry RouteEntry;//typeDef for the Clinet Info object
struct RouteEntry{
	//char dest_ip [IF_NAME];
	in_addr_t dest_ip;
	char next_hop [ETH_ALEN];	/* hardware address */
	int hop_count;
	int interfaceInd;
	struct timeval entryTime;
    RouteEntry *right;
    RouteEntry *left;
};


//int addRouteEntry(char * dIP, char * nh, int hc);
int addRouteEntry(in_addr_t dIP, char * nh, int hc, int intFIndex, RouteEntry **headEntry, RouteEntry **tailEntry);
int insertOrUpdateRouteEntry(in_addr_t sIP, char * nh, int hc, int intFIndex, RouteEntry **headEntry, RouteEntry **tailEntry);
int checkTime(struct timeval * inspect);
void removeRoutingEntry(RouteEntry **headEntry, RouteEntry **tailEntry);
//RouteEntry* findAndUpdateRouteEntry(char * destIP);

RouteEntry* findRouteEntry(in_addr_t destIP, RouteEntry **headEntry, RouteEntry **tailEntry);
void printRoutingTable(RouteEntry *headEntry, RouteEntry *tailEntry);

#endif