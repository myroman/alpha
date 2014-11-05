#ifdef DEBUG
#define debug(fmt, ...) printf("DEBUG: %s:%s:%d \n" fmt, __FILE__,__FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif


#include <stdio.h>


int main () {
	printf("Hello, world!");
	int i =100;
	debug(i);
	//exit(0);
}