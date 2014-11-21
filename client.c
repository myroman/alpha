#include "unp.h"
#include "misc.h"
#include "oapi.h"
#include "hw_addrs.h"
#include "debug.h"

char* listenFn;
char* myNodeName;
char* myNodeIP;
typedef struct VM_IP VM_IP;

struct VM_IP{
	char vm_name[5];
	char vm_ip[20];
};

VM_IP*  vmInfo;
void sig_int(int signo) {
	unlink(listenFn);
	printf("I'm cleaned up %s before termination\n", listenFn);
	exit(0);
}

int populateVmInfo(){
	vmInfo = malloc(sizeof(VM_IP) * 11);
    if(vmInfo == NULL)
    	err_sys("malloc error on allocating vmInfo structs");
    int i =0;
    for(i =0; i < 11; i++){
    	switch (i){
    		case 0:
    			strcpy(vmInfo[i].vm_name,"vm1");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.21");
    			break;
    		case 1:
    			strcpy(vmInfo[i].vm_name, "vm2");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.22");
    			break;
    		case 2:
    			strcpy(vmInfo[i].vm_name, "vm3");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.23");
    			break;
    		case 3:
    			strcpy(vmInfo[i].vm_name, "vm4");
    			strcpy(vmInfo[i].vm_ip,"130.245.156.24");
    			break;
    		case 4:
    			strcpy(vmInfo[i].vm_name, "vm5");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.25");
    			break;
    		case 5:
    			strcpy(vmInfo[i].vm_name,"vm6");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.26");
    			break;
    		case 6:
    			strcpy(vmInfo[i].vm_name, "vm7");
    			strcpy(vmInfo[i].vm_ip,"130.245.156.27");
    			break;
    		case 7:
    			strcpy(vmInfo[i].vm_name, "vm8");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.28");
    			break;
    		case 8:
    			strcpy(vmInfo[i].vm_name, "vm9");
    			strcpy(vmInfo[i].vm_ip,"130.245.156.29");
    			break;
    		case 9:
    			strcpy(vmInfo[i].vm_name, "vm10");
    			strcpy(vmInfo[i].vm_ip, "130.245.156.20");
    			break;
			case 10:
				strcpy(vmInfo[i].vm_name, "loc");
    			strcpy(vmInfo[i].vm_ip, "loc");
    			break;
    		default:
    			break;
    	}
    }
}
char * findIPofVM(char * ptr){
	debug("Gonna find %s", ptr);
	int i =0;
	for(i = 0; i < 11; i++){
		if(strcmp(ptr, vmInfo[i].vm_name) == 0){
			return vmInfo[i].vm_ip;
		}
	}
	return NULL;
}
char * findNameofVM(char * ip){
	int i =0;

	for(i = 0; i < 11; i++){
		if(strcmp(ip, vmInfo[i].vm_ip) == 0){
			return vmInfo[i].vm_name;
		}
	}

	return NULL;
}

int callServer(char* serverIP, char *serverVM, int lstFd){
	// Setup listening filename
	char* s;
	int forceRediscovery = 0, res,n, retransmit = 0;

	//res = msg_send(lstFd, "10.0.2.15\0", SRV_PORT_NUMBER, s, forceRediscovery);
  resend: 
	
	s = malloc(ETHFR_MAXDATA_LEN);
	strcpy(s, "Hi!\n");
	rmnl(s);
	debug("P4");	

	printf("Client at node %s sending to server at %s\n", myNodeName, serverVM);
	//"10.255.14.128\0"
	res = msg_send(lstFd, serverIP, SRV_PORT_NUMBER, s, forceRediscovery);
	debug("P5");
	free(s);
	
	debug("Requested time...");
	char* timestamp = malloc(ETH_MAX_MSG_LEN);
	int serverPort;
	char serverIpAddress[IP_ADDR_LEN];
	while ((n = msg_recv(lstFd, timestamp, serverIpAddress, &serverPort))) {
		if(n>0){
			printf("Clinet at node %s received from %s\n", myNodeName, serverVM);
			printf("Timestamp: %s\n", timestamp);
			free(timestamp);
			return;
		}
		break;
	}
	free(timestamp);
	if(n == -1){
		printf("Client at node %s: timeout on response from %s\n", myNodeName, serverVM);
		retransmit++;
		if(retransmit <=1){
			printf("Timeout: Force rediscovery flag set to true and going to retransmit.\n");
			forceRediscovery = 1;
			goto resend;
		}
		else{
			printf("We already retransmited once. Returing as per specs.\n");
		}
	}
}

void getNodeName(){
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;

	for (hwahead = hwa = get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {

		if( (strcmp(hwa->if_name, "eth0") == 0) ){
			if ( (sa = hwa->ip_addr) != NULL)
				myNodeIP = Sock_ntop_host(sa, sizeof(*sa));
		}
	}

	if(myNodeIP == NULL){
		myNodeIP = "130.245.156.21\0";
	}

	myNodeName = findNameofVM(myNodeIP);

}
int main(int argc, char **argv)
{
	int	lstFd, n;
	socklen_t len;
	
	if ( (lstFd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
		err_sys("socket error");
	Signal(SIGINT, sig_int);

	populateVmInfo();
	getNodeName();
	printf("%s , %s\n", vmInfo[2].vm_name, vmInfo[2].vm_ip);

	listenFn = malloc(10);
	char * fn =  createTmplFilename();
	strncpy(listenFn, fn, 10);
	free(fn);
	debug("listenFn=%s", listenFn);
	int z = mkstemp(listenFn);
	unlink(listenFn);
	debug("listenFn=%s,z=%d", listenFn, z);
	unlink(listenFn); //don't remove it. It helps to resolve filename later.

	SockAddrUn addr = createSockAddrUn(listenFn);
	bind(lstFd, (SA *)&addr, sizeof(addr));
	
	printf("**** Welcome to the Time Client. ****\n");

	while(1){
		printf("Choose one of the server nodes from vm1, vm2, vm3, vm4, vm5, vm6, vm7, vm8, vm9, and vm10.\n(To exit type exit.)\n: ");
		char choice[5];
		if(scanf("%s", choice) == 1){
			if(strcmp(choice, "exit") == 0){
				unlink(listenFn);
				printf("Thank you for using time client.\n");
				exit(0);
			}
			char *serverIP = findIPofVM(choice);
			if(serverIP != NULL){
				callServer(serverIP, choice, lstFd);
				continue;
			}
			else{
				printf("Not a valid option. Type vm# where '#' is replaced by a number from [1,10]\n");				
			}

		}
		else{
			printf("Error in selection. Plese type vm1 or vm2 etc...\n");
		}
	}
	
	if (n < 0)
		err_sys("read error");
	exit(0);
}
