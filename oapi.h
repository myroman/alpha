#define ETH_FRAME_LEN 1518

int msg_send(int sockfd, char* destIpAddr, int destPort, const char* msg, int forceRediscovery);

int msg_recv(int sockfd, char* msg, char* srcIpAddr, int* srcPort);