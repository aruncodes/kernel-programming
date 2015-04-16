#ifndef IOCTL_CONFIG
#define IOCTL_CONFIG 

#include <linux/ioctl.h>

#define UNIQUE_NO 	63

#define READ_LOCK_MMAP 		_IO(UNIQUE_NO,0)
#define WRITE_LOCK_MMAP 		_IO(UNIQUE_NO,1)
#define READ_UNLOCK_MMAP 	_IO(UNIQUE_NO,2)
#define WRITE_UNLOCK_MMAP 	_IO(UNIQUE_NO,3)

#endif