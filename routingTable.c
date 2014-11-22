#include "unp.h"
#include <sys/time.h>
#include "debug.h"
#include "routingTable.h"
#include "misc.h"
RouteEntry *headEntry = NULL;
RouteEntry *tailEntry = NULL;
int insertOrUpdateRouteEntry(in_addr_t sIP, char * nh, int hc, int intFIndex){
    removeRoutingEntry();//removes any stale entries in the routing table
    //Then we try to go and find the entry in our rooting table
    int ret;
    RouteEntry * ptr =  findRouteEntry(sIP);
    if(ptr == NULL){
        //Route to IP was not found we insert it into the table
        ret = addRouteEntry(sIP, nh, hc, intFIndex);
    }
    else{
        if(ptr->hop_count == hc){
            //
            if(strcmp(nh,ptr->next_hop) == 0){
                //update the time
                debug("Update the entry time");
                gettimeofday(&(ptr->entryTime), NULL);
                ptr->interfaceInd = intFIndex;
            }
            else{
                memcpy(ptr->next_hop, nh, IF_HADDR);
                gettimeofday(&(ptr->entryTime), NULL);
                ptr->interfaceInd = intFIndex;
                debug("Update the next hop MAC and entry time");
            }
        }
        else if(ptr->hop_count > hc){
            memcpy(ptr->next_hop, nh, IF_HADDR); //set the memory of the next hop
            gettimeofday(&(ptr->entryTime), NULL); //set the timeestmap
            ptr->hop_count = hc;
            ptr->interfaceInd = intFIndex;  
            debug("Found a better Route in the routing table. Updating entry."); 
        }
        else{
            debug("New route takes longer to get to dest IP. Disreguarding Msg.");
        }
    }
}
int addRouteEntry(in_addr_t dIP, char * nh, int hc, int intFIndex){
	int ret = 0;
	//Malloc the space for the new struct
    RouteEntry *newRoute = (RouteEntry *) malloc(sizeof( struct RouteEntry ));
    if(newRoute == NULL){
    	ret = -1;
    	return ret;
    }
    //struct timeval entryTime;

    gettimeofday(&(newRoute->entryTime), NULL);

    //memcpy(newRoute->dest_ip, dIP, IF_NAME);
    newRoute->dest_ip = dIP;
    memcpy(newRoute->next_hop, nh, IF_HADDR);
    newRoute->hop_count = hc;
    newRoute->interfaceInd = intFIndex;
    if(headEntry == NULL){
    	headEntry = newRoute;
    	newRoute->left = NULL;
    	newRoute->right = NULL;
    	tailEntry = newRoute;
    }
    else{
    	tailEntry->right = newRoute;
    	newRoute->left = tailEntry;
    	tailEntry = newRoute;
    }
    return 1;
}

int checkTime(struct timeval * inspect){
    struct timeval curTime;
    gettimeofday(&(curTime), NULL);
    if( (curTime.tv_sec - inspect->tv_sec) > ROUTING_ENTRY_STALE){
        return -1;
    }
    else{
        return 0;
    }

}

void removeRoutingEntry(){
    RouteEntry *ptr = headEntry;
    while(ptr != NULL){
        if(checkTime(&(ptr->entryTime)) == -1){
            printf("Removed Routing entry because it was stale\n");
            if(ptr->left == NULL && ptr->right != NULL){
                //HEAD
                debug("HEAD Removed");
                headEntry = ptr->right;
                headEntry->left = NULL;
            }
            else if(ptr->left != NULL && ptr->right == NULL){
                //TAIL
                debug("TAIL Removed\n");
                tailEntry = ptr->left;
                tailEntry->right = NULL;
            }
            else if(ptr->left == NULL && ptr->right == NULL){
                //HEAD_TAIL
                debug("HEAD_TAIL Removed\n");
                headEntry = NULL;
                tailEntry = NULL;
            }
            else{
                debug("MIDDLE Removed\n");
                ptr->left->right = ptr->right;
                ptr->right->left = ptr->left;
            }
            free(ptr);
        }
        ptr=ptr->right;
    }
}

RouteEntry* findRouteEntry(in_addr_t destIP){
    RouteEntry * ptr = headEntry;
    ptr = headEntry;
    removeRoutingEntry();
    while(ptr != NULL){
        //if(strcmp(ptr->dest_ip, destIP) ==0)
        if(destIP == ptr->dest_ip)
            return ptr;
        ptr=ptr->right;
    }
    return NULL;
}


void printRoutingTable(){
	RouteEntry * ptr = headEntry;
	int index = 0;
	while(ptr != NULL){
        //char * destIP = inet_ntoa(ptr->dest_ip);
		printf("%d: destadsfIP: %s, Next Hop MAC: %s, Hop Count : %u, Interface Index: %u, Entry Time: <%ld.%06ld>\n", index, printIPHuman(ptr->dest_ip), ptr->next_hop, ptr->hop_count, ptr->interfaceInd,(long) ptr->entryTime.tv_sec, (long) ptr->entryTime.tv_usec);
		index++;
		ptr = ptr->right;
	}
}

/*int main (){
    //addRouteEntry("129.49.233.217", "bc:77:37:27:94:03", 0);
    //addRouteEntry("129.49.233.218", "bc:77:37:27:94:03", 0);
    insertOrUpdateRouteEntry(16777343, "bc:77:37:27:94:03", 10, 1);
    insertOrUpdateRouteEntry(16909567, "bc:77:37:27:94:03", 0, 2);
    debug("here");
    printRoutingTable();

    insertOrUpdateRouteEntry(16777343, "bc:77:37:27:94:03", 1, 1);
    insertOrUpdateRouteEntry(16777343, "bc:77:37:27:94:03", 2, 1);
    insertOrUpdateRouteEntry(16777343, "bc:77:37:27:94:03", 1, 1);
    sleep(5);


    RouteEntry * p = findRouteEntry(16909567);

    if(p != NULL){
        printf("DESTIP %s, MAC %s, HopCount %d\n", printIPHuman(p->dest_ip), p->next_hop, p->hop_count);
    }
    else{
        printf("Not found\n");
    }

    printRoutingTable();
}*/
