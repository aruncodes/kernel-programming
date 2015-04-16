ASSIGNMENT #2

Estimation of WSS (Working Set Size)

What is WSS?
	WSS or working set size is the amount of actual physical memory that is used by the process for a specific amount time. Only a subset of actual physical memory alloted to the process will be used to accomplish a task. It is called WSS.

Components
	1. Kernel Hook in page fault handler which calls the function in our kernel module when it is inserted
	2. Kernel module which run periodically to estimate WSS. It depends on kernel hook
	
How to apply patch?
	The patch is to be applied to fault.c file. Assuming you are inside the kernel source root, it can be applied as

	patch arch/x86/mm/fault.c < fault.c.patch

	Then you need to compile and install kernel.

How to compile the module?
	1. Change directory to ./wss/modules/ 
	2. make
	3. The file wss.ko will be generated and it is the module file

How to execute ?
	1. Go to folder where wss.ko resides
	2. Find the pid of the process which you want to find the WSS using the command "ps -e"
	3. sudo insmod wss.ko pid=PID ntimes=N # where PID is you process id and N is the number of times rss should be calculated in 10 second intervals
	Eg: sudo insmod wss.ko pid=4825 ntimes=4

	4. After insertion, you can unload the module using
		sudo rmmod wss

How to get results?
	Results are printed in system log. You can access it using the command
		dmesg

How to compile and execute userspace program?
	1. gcc mem_jump.c -o jumper
	2. chmod +x jumper
	3. ./jumper