/*
	Extract vm areas from pid
*/


#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/dcache.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

static int pid = 1;

module_param(pid,int,0);
MODULE_PARM_DESC(pid,"PID of process");

struct task_struct	*task;
struct vm_area_struct *vm_area,*vm_start;
char flag[5] = "----";
struct file * vmfile;
struct path fpath;
unsigned char *name;

static int __init start(void) {
	//int i=0;
	// printk(KERN_INFO "Hello World!, I am Arun speaking from inside kernel.\n");

	task = pid_task(find_vpid(pid),PIDTYPE_PID);

	if(!task) {
		printk(KERN_INFO "Couldn't get process with that pid \n");
		return 1;
	}

	printk(KERN_INFO "Process name with PID %d is : %s\n",pid, task -> comm);

	vm_area = task -> mm -> mmap;

	vm_start = vm_area;
	while (1) {
		if(vm_start->vm_flags & VM_READ) flag[0] = 'r';
		if(vm_start->vm_flags & VM_WRITE) flag[1] = 'w';
		if(vm_start->vm_flags & VM_EXEC) flag[2] = 'x';
		if(vm_start->vm_flags & VM_SHARED) flag[3] = 's';

		if (vm_start->vm_file != NULL){
			vmfile = vm_start->vm_file;
			fpath = vmfile->f_path;
			name = fpath.dentry->d_iname;
		}
		else
			name = "________";

		printk(KERN_INFO "%lx - %lx - %-luKB - \t%s - \t%s\n",
			vm_start->vm_start,vm_start->vm_end,(-(vm_start->vm_start-vm_start->vm_end))/1024,flag,name);
		vm_start = vm_start ->vm_next;

		flag[0] =flag[1] =flag[2] =flag[3] ='-';

		if(vm_start == NULL || vm_start == vm_area) {
			break;
		}
	}
	
	return 0;
}

static void __exit stop(void) {
	printk(KERN_INFO "Module exiting..\n");
}

module_init(start);
module_exit(stop);