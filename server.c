#include "unp.h"
#include <time.h>
#include "misc.h"
#include "oapi.h"
#include "debug.h"
#include "hw_addrs.h"
char* myNodeIP;
char* myNodeName;
void replyTs(int sockfd);

int main(int argc, char **argv)
{	
	//getNodeName();

	int listenfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	unlink(SRV_UNIX_PATH);
	SockAddrUn servaddr = createSockAddrUn(SRV_UNIX_PATH);
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));	
	
	replyTs(listenfd);
}
void getNodeName(){
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	struct hostent *hptr;
	struct in_addr ipv4addr;


	for (hwahead = hwa = get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {

		if(strcmp(hwa->if_name, "eth0") == 0 || strcmp(hwa->if_name, "wlan0") == 0 ){
			if ( (sa = hwa->ip_addr) != NULL)
				myNodeIP = Sock_ntop_host(sa, sizeof(*sa));
		}
	}
	myNodeIP = Sock_ntop_host(sa, sizeof(*sa));
char* myNodeName;
	//myNodeName = findNameofVM(myNodeIP);
	if(inet_aton(myNodeIP, &ipv4addr) != 0){
            if((hptr = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET)) == NULL)
                    err_quit ( "gethostbyaddress error for IP address: %s: %s" , myNodeIP, hstrerror(h_errno));
            
            printf("The server host name is %s.\n", hptr->h_name);
            myNodeName =(char*) malloc(strlen(hptr->h_name));
            strcpy(myNodeName,hptr->h_name);
            //strcpy(myNodeName, myNodeIP);//copy the string into string
    }
    debug("%s %s", myNodeName, myNodeIP);
}
char * getRequestNodeName(char * ip){
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	struct hostent *hptr;
	struct in_addr ipv4addr;
	char* myNodeName;
	if(inet_aton(myNodeIP, &ipv4addr) != 0){
            if((hptr = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET)) == NULL)
                    err_quit ( "gethostbyaddress error for IP address: %s: %s" , ip, hstrerror(h_errno));
            
            printf("The server host name is %s.\n", hptr->h_name);
            //myNodeName =(char*) malloc(strlen(hptr->h_name));
            //strcpy(myNodeName,hptr->h_name);
            return hptr->h_name;
            //strcpy(myNodeName, myNodeIP);//copy the string into string
    }

    debug("%s %s", myNodeName, myNodeIP);
}
void replyTs(int sockfd)
{
	char* msg = malloc(MAXLINE);
	char buff[MAXLINE];
	bzero(buff, MAXLINE);
	time_t ticks;
	char srcIpAddr[IP_ADDR_LEN];
	int srcPort, forceRediscovery = 0;

	for ( ; ; ) {
		//debug("Waiting for request...");
		int n = msg_recv(sockfd, msg, srcIpAddr, &srcPort);
		if (n == -1){
			//printf("Timeout msg_recv\n");
			continue;
		}
		printf("Received msg:%s from %s:%d\n", msg, srcIpAddr, srcPort);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
        //printf("server at node %s responding to request from %s\n", myNodeName, getRequestNodeName(srcIpAddr));
        printf("Sending %s, length = %d\n", buff, (int)strlen(buff));         
		msg_send(sockfd, srcIpAddr, srcPort, buff, forceRediscovery);
	}

	free(msg);
	free(srcIpAddr);
}