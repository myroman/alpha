#include "bidTable.h"
#include "debug.h"

BidEntry *headBid = NULL;
BidEntry *tailBid = NULL;

char * printIPHuman(in_addr_t ip){
    struct in_addr ipIa;
    ipIa.s_addr = ip;
    return inet_ntoa(ipIa);
}
int addBidEntry(in_addr_t sIP, uint32_t bid, int hop_count){
	int ret = 0;
	BidEntry * newEntry = (BidEntry*) malloc(sizeof(struct BidEntry));
	if(newEntry == NULL){
		ret = -1;
		return ret;
	}

	newEntry->src_ip = sIP;
	newEntry->hop_count = hop_count;
	newEntry->bid = bid;

	if(headBid == NULL){
		headBid = newEntry;
		newEntry->right = NULL;
		tailBid = newEntry;
	}
	else{
		tailBid->right = newEntry;
		tailBid = newEntry;
	}
}
BidEntry* findBidEntry(in_addr_t dIP, uint32_t bid){
	BidEntry *ptr = headBid;
	while(ptr != NULL){
		if(ptr->src_ip == dIP && ptr->bid == bid){
			debug("Match found in RREP bid table.");
			return ptr;
		}
	}
}

/*
* return -1 if entry is not in the routing table
* return  1 if they entry is hopcount is less than the hopcount in the entry.
* return  0 if they entry is hopcount >= the hopcount in the entry table
*/
int compareHopCount(in_addr_t destIP, uint32_t bid, int hopcount){
	BidEntry * analyze = findBidEntry(destIP, bid);
	if(analyze == NULL){
		return -1;
	}
	else if(analyze->hop_count > hopcount){
		debug("SEND RREQ. Found a better route");
		return 1;
	}
	else{
		return 0;
	}
}


int main(){
	addBidEntry(16777343, 1, 5);
	compareHopCount(16777343, 1, 1);

}