#ifdef DEBUG
	#define debug(fmt, ...) do{printf("DEBUG: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__); printf("\n");}while(0)
	#define info(fmt, ...) do{printf("INFO: %s:%s:%d " fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__);printf("\n");}while(0)
#else
	#define debug(fmt, ...)
	#define info(fmt, ...) //do{printf("INFO: \n" fmt,##__VA_ARGS__); printf("\n");}while(0)
#endif



#include <stdio.h>


int main () {
	printf("Hello, world!\n");
	int i =100;
	debug("%d", i);
	info("Tesing info msg");
	//exit(0);
}