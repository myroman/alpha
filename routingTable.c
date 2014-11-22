#include "unp.h"
#include <sys/time.h>
#include "debug.h"
#include "routingTable.h"
#include "misc.h"
RouteEntry *headEntry = NULL;
RouteEntry *tailEntry = NULL;

int addRouteEntry(in_addr_t dIP, char * nh, int hc){
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

RouteEntry* findAndUpdateRouteEntry(in_addr_t destIP){
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
		printf("%d: destIP: %s, Next Hop MAC: %s, Hop Count : %d, Entry Time: <%ld.%06ld>\n", index, printIPHuman(ptr->dest_ip), ptr->next_hop, ptr->hop_count, (long) ptr->entryTime.tv_sec, (long) ptr->entryTime.tv_usec);
		index++;
		ptr = ptr->right;
	}
}
/*
int main (){
    //addRouteEntry("129.49.233.217", "bc:77:37:27:94:03", 0);
    //addRouteEntry("129.49.233.218", "bc:77:37:27:94:03", 0);
    addRouteEntry(16777343, "bc:77:37:27:94:03", 0);
    addRouteEntry(16909567, "bc:77:37:27:94:03", 0);
    debug("here");
    printRoutingTable();

    sleep(1);
    debug("here");
    RouteEntry * p = findAndUpdateRouteEntry(16909567);
    debug("here");
    if(p != NULL){
        printf("DESTIP %s, MAC %s, HopCount %d\n", printIPHuman(p->dest_ip), p->next_hop, p->hop_count);
    }
    else{
        printf("Not found\n");
    }

    printRoutingTable();
}
*/