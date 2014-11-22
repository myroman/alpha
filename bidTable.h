#ifndef __bidTable_h_
#define __bidTable_h_
#include "unp.h"

typedef struct BidEntry BidEntry;
struct BidEntry{
	in_addr_t src_ip;
	int hop_count;
	uint32_t bid;
	BidEntry *right;
};

int addBidEntry(in_addr_t sIP, uint32_t bid, int hop_count);
BidEntry* findBidEntry(in_addr_t dIP, uint32_t bid);
#endif