#include <stdio.h>
#include <fcntl.h>

#define DEVICE_NAME "/dev/chardev"

int devfd;

int main() {
	int i,ret;
	char buf[512],ch;

	devfd = open(DEVICE_NAME,O_RDWR,0777);

	if (devfd < 0) {
		perror("Cannot open device : ");
		return 1;
	}

	/*Write to device file*/
	ret = write(devfd, "Hello from user space..\n",25);
	printf("Wrote %d bytes to device\n", ret);

	/*Read from device*/
	ret = read( devfd ,buf, 25);
	printf("Read %d bytes from device : %s\n",ret, buf);


	printf("Writing until queue becomes full..\n");
	for(i=0; ; i++) {
		ret = write(devfd,"a",1);

		if(ret <= 0) {
			printf("Queue full.. Capacity : %d\n",i);
			break;
		}
	}

	close(devfd);
	return 0;
}