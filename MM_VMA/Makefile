obj-m += ts_extract.o
obj-m += vm_map.o
obj-m += all_process.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean