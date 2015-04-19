obj-m += sstf-iosched.o
obj-m += sstf-rw-deadline-iosched.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean