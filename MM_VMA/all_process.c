/*
	Extract info of all processes
*/


#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

struct task_struct	*task;

static int __init start(void) {

	//Find all processes 
	for_each_process(task) {
		printk(KERN_INFO "%s (PID:%d)(PF:%ld)(CS:%ld)(CPU:%d)\n",
			task->comm,task->pid, task->maj_flt+task->min_flt,task->nvcsw,task -> on_cpu);
	}

	return 0;
}

static void __exit stop(void) {
	printk(KERN_INFO "Module exiting..\n");
}

module_init(start);
module_exit(stop);



	// printk(KERN_INFO "Hello World!, I am Arun speaking from inside kernel.\n");

	// task = pid_task(find_vpid(1),PIDTYPE_PID);

	// if(!task) {
	// 	printk(KERN_INFO "Couldn't get process with pid 1\n");
	// 	return 1;
	// }

	// printk(KERN_INFO "Process name with PID %d is : %s\n",pid, task -> comm);

	// //Print some info from task struct
	// printk(KERN_INFO "Details of process: %s\n---------------\n",task->comm);
	// printk(KERN_INFO "State of process :");
	// switch ( task -> state) {
	// 	case -1 : printk(KERN_INFO "Unrunnable \n"); break;
	// 	case 0 : printk(KERN_INFO "Runnable \n"); break;
	// 	default : printk(KERN_INFO "Stopped \n"); break;
	// }

	// printk(KERN_INFO "Page Fault count : Major: %ld Minor %ld \n",task->maj_flt,task->min_flt);
	// printk(KERN_INFO "Number of context switches : %ld\n",task->nvcsw);
	// printk(KERN_INFO "Start time (nanosecs) : %ld \n",task->start_time);
	// printk(KERN_INFO "CPU : %d",task -> on_cpu);
