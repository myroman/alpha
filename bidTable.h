#ifndef __bidTable_h_
#define __bidTable_h_
#include "unp.h"
#include "misc.h"

typedef struct BidEntry BidEntry;
struct BidEntry{
	in_addr_t src_ip;
	int hop_count;
	uint32_t bid;
	BidEntry *right;
};
/*
* This is the function to call. It will return 1 or 0. It will insert into table if not present. 
* It will update table if its more efficient route is found. Call only this function from ODR.
* Returns 1 if RREP needs to be sent
* Returns 0 if RREP doesn't need to be sent
*/
int addBidEntry(in_addr_t sIP, uint32_t bid, int hop_count, BidEntry ** headBid, BidEntry ** tailBid);
BidEntry* findBidEntry(in_addr_t IP, uint32_t bid, BidEntry * headBid);
int compareHopCount(in_addr_t destIP, uint32_t bid, int hopcount, BidEntry * headBid);
void printBidTable(BidEntry * headBid);
#endif