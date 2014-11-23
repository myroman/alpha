#include "portPath.h"
#include "debug.h"


int addPortPath(char * fpath, int port, int fd ,int w, PortPath ** headEntryPort, PortPath ** tailEntryPort){
	int ret = 0;
	//Malloc the space for the new struct
    PortPath *newEntry = (PortPath *) malloc(sizeof( struct PortPath ));
    if(newEntry == NULL){
    	ret = -1;
    	return ret;
    }

    //struct timeval entryTime;

    gettimeofday(&(newEntry->entryTime), NULL);

    memcpy(newEntry->file_path, fpath, PATH_LENGTH);
    newEntry->port_number = port;
    newEntry->well_known = w;
    newEntry->fd = fd;
    if(*headEntryPort == NULL){
    	*headEntryPort = newEntry;
    	newEntry->left = NULL;
    	newEntry->right = NULL;
    	*tailEntryPort = newEntry;
    }
    else{
    	(*tailEntryPort)->right = newEntry;
    	newEntry->left = *tailEntryPort;
    	*tailEntryPort = newEntry;
    }
}
int checkTime2(struct timeval * inspect){
	struct timeval curTime;
    gettimeofday(&(curTime), NULL);
    //debug("currentTime %u, inspect time %u", curTime.tv_sec, inspect->tv_sec);
    
    //printf("\n\n\nSTALENESS = %d \n\n\n", STALENESS);

    if( (curTime.tv_sec - inspect->tv_sec) > STALENESS){
        debug("check time = -1 %u", (curTime.tv_sec - inspect->tv_sec));
        return -1;
    }
    else{
        return 0;
    }
}
void removePortEntry(PortPath ** headEntryPort, PortPath ** tailEntryPort){
	PortPath *ptr = *headEntryPort;
    while(ptr != NULL){
        if(ptr->well_known == 1){
        	ptr= ptr->right;
        	continue;
        }

        if(checkTime2(&(ptr->entryTime)) == -1){
            printf("Removed file path and port entry because it was stale\n");
            if(ptr->left == NULL && ptr->right != NULL){
                //HEAD
                debug("HEAD Removed");
                *headEntryPort = ptr->right;
                (*headEntryPort)->left = NULL;
            }
            else if(ptr->left != NULL && ptr->right == NULL){
                //TAIL
                debug("TAIL Removed\n");
                *tailEntryPort = ptr->left;
                (*tailEntryPort)->right = NULL;
            }
            else if(ptr->left == NULL && ptr->right == NULL){
                //HEAD_TAIL
                debug("HEAD_TAIL Removed\n");
                *headEntryPort = NULL;
                *tailEntryPort = NULL;
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
PortPath* findAndUpdatePath(char * fp,PortPath ** headEntryPort, PortPath ** tailEntryPort){
    removePortEntry(headEntryPort, tailEntryPort);
    PortPath * ptr = *headEntryPort;
    while(ptr != NULL){
        if(strcmp(ptr->file_path, fp) ==0){
            gettimeofday(&(ptr->entryTime), NULL);
            return ptr;
        }
        ptr=ptr->right;
    }

    return NULL;
}
PortPath* findAndUpdatePort(int port, PortPath ** headEntryPort, PortPath ** tailEntryPort){
	removePortEntry(headEntryPort, tailEntryPort);
    PortPath * ptr = *headEntryPort;
    
    while(ptr != NULL){
        if(ptr->port_number==port){
            gettimeofday(&(ptr->entryTime), NULL);
            return ptr;
        }
        ptr=ptr->right;
    }

    return NULL;
}
void printPortTable(PortPath ** headEntryPort, PortPath ** tailEntryPort){
	PortPath * ptr = *headEntryPort;
	int index = 0;
	printf("\n*** Port Path Table ***\n ");
    while(ptr != NULL){

		printf("%d: Sun Path filename: %s, Port Number: %d, File Descriptor: %u, Well Know: %d\n", index, ptr->file_path, ptr->port_number, ptr->fd ,ptr->well_known);
		index++;
		ptr = ptr->right;
	}
    printf("\n");
}
/*
int main(){
	
    PortPath *headEntryPort = NULL;
    PortPath *tailEntryPort = NULL;
    addPortPath("test.cpp", 1025, 11,1, &headEntryPort, &tailEntryPort);
    addPortPath("test.c", 1024, 10,0, &headEntryPort, &tailEntryPort);
	

	printPortTable(&headEntryPort, &tailEntryPort);
	sleep(3);

	PortPath *p = findAndUpdatePort(1025, &headEntryPort, &tailEntryPort);
	PortPath *p2 = findAndUpdatePath("test.c", &headEntryPort, &tailEntryPort);
    debug("here");
	if(p!= NULL)
		printf("%s, %u, %u\n", p->file_path, p->port_number, p->fd);
	if(p2!=NULL) 
		printf("%s, %u, %u\n", p2->file_path, p2->port_number, p2->fd);
    debug("here");
	printPortTable(&headEntryPort, &tailEntryPort);
}*/
