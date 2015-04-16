#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/rbtree.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

static int pid = -1;

module_param(pid,int,0);
MODULE_PARM_DESC(pid,"PID of process");

struct task_struct	*task;

struct rb_node *node;
struct rb_root *rbroot;
struct vm_area_struct *vma;

int console_print(size_t size, const char *fmt, ...);

static int __init start(void) {
	//Get task_struct of pid
	task = pid_task(find_vpid(pid),PIDTYPE_PID);

	if(!task || pid == -1) {
		console_print(128, "Couldn't get  task_struct \r\n");
		return 1;
	}

	rbroot = &(task -> mm -> mm_rb);
	//iterate through tree
	for (node = rb_first(rbroot); node; node = rb_next(node) ) {
		vma = rb_entry(node,struct vm_area_struct,vm_rb);
		console_print(128, " %lx - %lx \r\n", vma -> vm_start, vma -> vm_end);
	}

	return 0;
}

static void __exit stop(void) {
	console_print(128, "Module exit\r\n");
}

module_init(start);
module_exit(stop);

static void printString(char *string) {
    struct tty_struct *tty;
    tty = get_current_tty();

    if(tty != NULL) { 
        (tty->driver->ops->write) (tty, string, strlen(string));
    } else
        printk("tty equals to zero");
}

int console_print(size_t size, const char *fmt, ...){
    va_list args;
    int i;
    char buf[size];

    va_start(args, fmt);
    i = vsnprintf(buf, size, fmt, args);
    va_end(args);

    printString(buf);

    return i;
}