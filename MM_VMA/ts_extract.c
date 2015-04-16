/*
	Extract task struct from pid
*/


#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/pid.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

static int pid = 1;

module_param(pid,int,0);
MODULE_PARM_DESC(pid,"PID of process");

struct task_struct	*task, *parent;

static int __init start(void) {
	int i=0;
	// printk(KERN_INFO "Hello World!, I am Arun speaking from inside kernel.\n");

	task = pid_task(find_vpid(pid),PIDTYPE_PID);

	if(!task) {
		printk(KERN_INFO "Couldn't get process with that pid \n");
		return 1;
	}

	printk(KERN_INFO "Process name with PID %d is : %s\n",pid, task -> comm);

	//Print some info from task struct
	printk(KERN_INFO "Details of process: %s\n---------------\n",task->comm);
	printk(KERN_INFO "State of process :");
	switch ( task -> state) {
		case -1 : printk(KERN_INFO "Unrunnable \n"); break;
		case 0 : printk(KERN_INFO "Runnable \n"); break;
		default : printk(KERN_INFO "Stopped \n"); break;
	}

	printk(KERN_INFO "Page Fault count : Major: %ld Minor %ld \n",task->maj_flt,task->min_flt);
	printk(KERN_INFO "Number of context switches : %ld\n",task->nvcsw);
	printk(KERN_INFO "Start time (nanosecs) : %ld \n",task->start_time);
	printk(KERN_INFO "CPU : %d",task -> on_cpu);

	//Find process hierarchy

	printk(KERN_INFO "%s (%d)\n",task -> comm,task->pid);
	while ( i < 50) { // For not going into infinite loop just in case something goes wrong
		
		parent = task -> parent;

		printk(KERN_INFO " |\n V\n");
		printk(KERN_INFO "%s (%d)\n",parent->comm,parent->pid);

		//exit if init is reached
		if(parent->pid == 1 || parent->pid == 0) {
			break;
		}

		task = parent;
		i++;
	}

	return 0;
}

static void __exit stop(void) {
	printk(KERN_INFO "Module exiting..\n");
}

module_init(start);
module_exit(stop);