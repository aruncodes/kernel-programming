#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>

#define PAGE_SIZE 4096

int main ( int argc, char **argv )
{
	int configfd,ret;
	char buf[4092];

	configfd = open("/dev/mmapchardev", O_RDWR);
	if(configfd < 0) {
		perror("open");
		return -1;
	}

	char * address = NULL;
	address = mmap(NULL,3*  PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, configfd, 0);
	if (address == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	printf("Written by kernel : %s\n", address);
	memcpy(address + 11 , "*arun*", 6);
	printf("Modified by user : %s\n", address);

	memcpy(address + PAGE_SIZE + 11, "page 1",6);
	memcpy(address + 2*PAGE_SIZE + 11, "page 2",6);
	
	close(configfd);	
	return 0;
}