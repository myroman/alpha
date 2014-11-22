#include "bidTable.h"
#include "debug.h"

BidEntry *headBid = NULL;
BidEntry *tailBid = NULL;

int addBidEntry(in_addr_t sIP, uint32_t bid, int hop_count){
	int ret = 0;
	BidEntry * newEntry = (BidEntry*) malloc(sizeof(struct BidEntry));
	if(newEntry == NULL){
		ret = -1;
		return ret;
	}

	BidEntry* isPresent = findBidEntry(sIP, bid);
	if(isPresent != NULL){
		debug("Entry already present.");
		return 0;
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
	ret = 1;
	return ret;
}
BidEntry* findBidEntry(in_addr_t IP, uint32_t bid){
	BidEntry *ptr = headBid;
	while(ptr != NULL){
		if(ptr->src_ip == IP && ptr->bid == bid){
			debug("Match found in RREP bid table.");
			return ptr;
		}
	}
	return NULL;
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
		analyze->hop_count = hopcount;//update table with efficient 
		return 1;
	}
	else{
		debug("DONT SEND RREQ. DUPLICATE RREP TP RREQ.");
		return 0;
	}
}


void printBidTable(){
	
	BidEntry * ptr = headBid;
	int index = 0;
	while(ptr != NULL){
        //char * destIP = inet_ntoa(ptr->dest_ip);
		printf("Source IP: %s, Broadcast id: %u , Hop Count : %u\n", printIPHuman(ptr->src_ip), ptr->bid,ptr->hop_count);
		index++;
		ptr = ptr->right;
	}
}

int main(){
	addBidEntry(16777343, 1, 5);
	addBidEntry(16777343, 1, 5);
	printBidTable();
	compareHopCount(16777343, 1, 1);
	printBidTable();

}