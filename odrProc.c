#include "unp.h"
#include "odrProc.h"
int main(int argc, char **argv) {

	//00:0c:29:49:3f:5b vm1
	//00:0c:29:d9:08:ec vm2

	//TODO: use prhwaddrs to use arbitrary MAC	
	unsigned char src_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*our MAC address*/	
	unsigned char dest_mac[6] = {0x08, 0x00, 0x27, 0x8a, 0x83, 0x53};/*other host MAC address*/

	/* Listen to UNIX domain sockets for messages from client/server 
When received - deserialize the data into arguments which they sent: for msg_send or msg_recv.
Then call a function from odrImpl.c
When you get a response, serialize it and send back to that socket
	*/
}