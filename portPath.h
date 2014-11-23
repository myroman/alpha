#ifndef __routingTable_h_
#define __routingTable_h_
#include "unp.h"
#include <sys/time.h>

#define PATH_LENGTH 104
#define STALENESS 5

typedef struct PortPath PortPath;//typeDef for the Clinet Info object
struct PortPath{
	char file_path [PATH_LENGTH];
	int port_number;
	int fd;
	int well_known;
	struct timeval entryTime;
	
    PortPath *right;
    PortPath *left;
};


int addPortPath(char * fpath, int port, int fd,int w, PortPath ** headEntryPort, PortPath ** tailEntryPort);
int checkTime2(struct timeval * inspect);
void removePortEntry(PortPath ** headEntryPort, PortPath ** tailEntryPort);
PortPath* findAndUpdatePath(char * fp, PortPath ** headEntryPort, PortPath ** tailEntryPort);
PortPath* findAndUpdatePort(int port, PortPath ** headEntryPort, PortPath ** tailEntryPort);
void printPortTable(PortPath ** headEntryPort, PortPath ** tailEntryPort);

#endif