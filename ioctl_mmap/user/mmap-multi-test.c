#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "../ioctl_config.h"

#define PAGE_SIZE 4096

int rlock=0,wlock=0;

void read_lock(int fd) {
	if (ioctl(fd, READ_LOCK_MMAP) == -1) 
	    perror("read_lock");
}
void write_lock(int fd) {
	if (ioctl(fd, WRITE_LOCK_MMAP) == -1) 
	    perror("write_lock");
}
void read_unlock(int fd) {
	if (ioctl(fd, READ_UNLOCK_MMAP) == -1) 
	    perror("read_unlock");
}
void write_unlock(int fd) {
	if (ioctl(fd, WRITE_UNLOCK_MMAP) == -1) 
	    perror("write_unlock");
}

void read_write_menu(int fd, char *address);

int main ( int argc, char **argv )
{
	int devfd,ret;
	char buf[4092];

	devfd = open("/dev/mmap", O_RDWR);
	if(devfd < 0) {
		perror("open");
		return -1;
	}

	char * address = NULL;
	address = mmap(NULL,3*  PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, devfd, 0);
	if (address == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	read_lock(devfd);
	printf("Written by kernel : %s\n", address);
	read_unlock(devfd);
	
	write_lock(devfd);
	memcpy(address + 11 , "*arun*", 6);
	write_unlock(devfd);
	
	read_lock(devfd);
	printf("Modified by user : %s\n", address);
	read_unlock(devfd);

	write_lock(devfd);
	memcpy(address + PAGE_SIZE + 11, "page 1",6);
	memcpy(address + 2*PAGE_SIZE + 11, "page 2",6);
	write_unlock(devfd);

	read_write_menu(devfd,address);

	close(devfd);	
	return 0;
}

void read_write_menu(int fd, char *address) {

	int ch=0;
	char buf[100];

	while(1) {

		printf("MENU\n\n");
		printf("1. Aquire READ_LOCK\n");
		printf("2. READ from mmap\n");
		printf("3. Release READ_LOCK\n");
		printf("4. Aquire WRITE_LOCK\n");
		printf("5. WRITE to mmap\n");
		printf("6. Release WRITE_LOCK\n");
		printf("7. Exit\nYour choice : ");

		scanf("%d",&ch);

		switch(ch) {
			case 1:
				if( rlock ) {printf("READ_LOCK already aquired.\n"); break;}
				if( wlock ) {printf("WRITE_LOCK already aquired.\n"); break;}
				read_lock(fd);
				rlock = 1;
				printf("\t READ_LOCK \t aquired.\n");
				break;
			case 2:
				if( !rlock && !wlock) {printf("READ_LOCK or WRITE_LOCK not aquired!\n");break;}
				printf("Content: %s\n", address);
				break;
			case 3:
				if( !rlock ) {printf("Not aquired.\n"); break;}
				read_unlock(fd);
				rlock = 0;
				printf("\t READ_LOCK \t released.\n");
				break;
			case 4:
				if( rlock ) {printf("READ_LOCK already aquired.\n"); break;}
				if( wlock ) {printf("WRITE_LOCK already aquired.\n"); break;}
				write_lock(fd);
				wlock = 1;
				printf("\t WRITE_LOCK \t aquired.\n");
				break;
			case 5:
				if( !wlock ) {printf("WRITE_LOCK not aquired!\n");break;}
				printf("Enter a string:\n");
				scanf("%[^\n]",buf);
				memcpy(address,buf,100);
				break;
			case 6:
				if( !wlock ) {printf("Not aquired.\n"); break;}
				write_unlock(fd);
				wlock = 0;
				printf("\t WRITE_LOCK \t released.\n");
				break;
			default:
				if( rlock ) {printf("READ_LOCK already aquired, release first.\n"); break;}
				if( wlock ) {printf("WRITE_LOCK already aquired, release first.\n"); break;}
				return;
		}
		getchar(); // to read the enter key pressed after choice input
		getchar(); //actual pause
	}
}