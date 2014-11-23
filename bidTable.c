#include "bidTable.h"
#include "debug.h"


/*
* This is the function to call. It will return 1 or 0. It will insert into table if not present. 
* It will update table if its more efficient route is found. Call only this function from ODR.
* Returns 1 if RREP needs to be sent
* Returns 0 if RREP doesn't need to be sent
*/
int addBidEntry(in_addr_t sIP, uint32_t bid, int hop_count, BidEntry ** headBid, BidEntry ** tailBid){
	int ret = 0;
	//BidEntry* isPresent = findBidEntry(sIP, bid, *headBid);
	int retVal = compareHopCount(sIP, bid, hop_count, *headBid);
	if(retVal != -1){
		//debug("Entry already present.");
		if(retVal == 1){
			debug("Send RREP. More Efficient Route.");
			return 1;
		}
		else if(retVal == 0){
			debug("Dont send RREP. The one that was already sent is more efficient.");
			return 0;
		}
		else{
			return 0;
		}
	}
	BidEntry * newEntry = (BidEntry*) malloc(sizeof(struct BidEntry));
	if(newEntry == NULL){
		ret = -1;
		debug("Malloc Error");
		return ret;
	}
	newEntry->src_ip = sIP;
	newEntry->hop_count = hop_count;
	newEntry->bid = bid;
	if(*headBid == NULL){
		*headBid = newEntry;
		newEntry->right = NULL;
		*tailBid = newEntry;
	}
	else{
		newEntry->right = NULL;
		(*tailBid)->right = newEntry;
		*tailBid = newEntry;
	}
	ret = 1;
	debug("New entry inserted. Generating RREP");
	return ret;
}
BidEntry* findBidEntry(in_addr_t IP, uint32_t bid, BidEntry * headBid){
	BidEntry *ptr = headBid;
	while(ptr != NULL){
		if(ptr->src_ip == IP && ptr->bid == bid){
			//debug("Match found in RREP bid table.");
			return ptr;
		}
		ptr = ptr->right;
	}
	return NULL;
}

/*
* return -1 if entry is not in the routing table
* return  1 if they entry is hopcount is less than the hopcount in the entry.
* return  0 if they entry is hopcount >= the hopcount in the entry table
*/
int compareHopCount(in_addr_t destIP, uint32_t bid, int hopcount, BidEntry * headBid){
	BidEntry * analyze = findBidEntry(destIP, bid, headBid);
	if(analyze == NULL){
		return -1;
	}
	else if(analyze->hop_count > hopcount){
		//debug("SEND RREQ. Found a better route");
		analyze->hop_count = hopcount;//update table with efficient 
		return 1;
	}
	else{
		//debug("DONT SEND RREQ. DUPLICATE RREP TP RREQ.");
		return 0;
	}
}


void printBidTable(BidEntry * headBid){
	
	BidEntry * ptr = headBid;
	int index = 0;
	printf("\n*** BID TABLE ****\n");
	while(ptr != NULL){
        //char * destIP = inet_ntoa(ptr->dest_ip);
		printf("Source IP: %s, Broadcast id: %u , Hop Count : %u\n", printIPHuman(ptr->src_ip), ptr->bid,ptr->hop_count);
		index++;
		ptr = ptr->right;
	}
	printf("\n");
}

/*int main(){
	BidEntry *headBid = NULL;
	BidEntry *tailBid = NULL;

	addBidEntry(16777343, 1, 5, &headBid, &tailBid);
	addBidEntry(16777343, 1, 5, &headBid, &tailBid);
	printBidTable(headBid);

	addBidEntry(16777343, 1, 1, &headBid, &tailBid);
	addBidEntry(16777343, 1, 5, &headBid, &tailBid);
	addBidEntry(16777343, 2, 5, &headBid, &tailBid);
	//compareHopCount(16777343, 1, 1, headBid);
	printBidTable(headBid);

}*/