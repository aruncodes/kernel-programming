ASSIGNMENT #2

Estimation of RSS (Resident Set Size)

What is RSS?
	RSS or resident set size is the amount of actual physical memory that is used by the process. Only a subset of actual virtual memory alloted to the process will be in physical memory at any given point in time. It is called RSS.

How is RSS calculated?
	Kernel manages memory in units of Pages. Pages are small sized chunks of bytes. It is usually of size 4KB in a 32 bit system. Page table keeps track of all the pages alloted to a process. Each process has its own page table. 

	The procedure I used to calculate RSS is to walk through the page table entries and check if the page is present in memory or not.

	Page table had Page Table Entires (PTE). The PTE contains many 
	bits which indicate the status of the page. The bit that is of importance here is the Valid bit / Present Bit. 
	The kernel module iterates over all PTEs of a process and checks if the page is present in physical memory and counts if it is.

How to compile the module?
	1. Change directory to ./rss/modules/ 
	2. make
	3. The file rss.ko will be generated and it is the module file

How to execute ?
	1. Go to folder where rss.ko resides
	2. Find the pid of the process which you want to find the RSS using the command "ps -e"
	3. sudo insmod rss.ko pid=PID ntimes=N # where PID is you process id and N is the number of times rss should be calculated in 10 second intervals

	Eg: sudo insmod rss.ko pid=4825 ntimes=4

	4. After insertion, you can unload the module using
		sudo rmmod rss

How to get results?
	Results are printed in system log. You can access it using the command
		dmesg

How to verify results?
	There are severl ways to see the RSS of process

	1. ps -eo pid,comm,rss
	2. cat /proc/<PID>/status
	3. cat /proc/<PID>/smaps  #(detailed)