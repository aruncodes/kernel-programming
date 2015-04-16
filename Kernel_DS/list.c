#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

struct node {
	int pid;
	char *name;
	int cpu;
	char *parent;
	int priority;

	struct list_head list;
};

LIST_HEAD(process_head);

struct task_struct *task;

int console_print(size_t size, const char *fmt, ...);

static int __init start(void) {
	// console_print(128,"Hello World!, I am Arun speaking from inside kernel.\r\n");
	struct node *process_node,*tmp;
	int flag = 0;

	for_each_process(task) {

		process_node = kmalloc(sizeof(struct node),GFP_KERNEL);
		process_node -> pid = task -> pid;
		process_node -> name = task -> comm;
		process_node -> cpu = task -> on_cpu;
		process_node -> parent = task -> parent -> comm;
		process_node -> priority = task -> prio;

		list_add(&process_node -> list, &process_head);
	}

	console_print(128,"Populated task list.\r\n");

	console_print(128,"Process List from task_struct list\r\n") ;
	console_print(128,"PID \t Name \t Parent \t CPU \t Priority \r\n");
	for_each_process(task) {

		console_print(128,"%d \t %s \t %s \t %d \t %d \r\n", 
										task -> pid,
										task -> comm,
										task -> parent -> comm,
										task -> on_cpu,
										task -> prio );	
	}

	console_print(128,"Process List from custom list\r\n") ;
	console_print(128,"PID \t Name \t Parent \t CPU \t Priority \r\n");
	list_for_each_entry_safe(process_node,tmp,&process_head,list) {


		console_print(128,"%d \t %s \t %s \t %d \t %d \r\n", 
										process_node -> pid,
										process_node -> name,
										process_node -> parent,
										process_node -> cpu,
										process_node -> priority);
		if(flag) {
			flag=0;
			list_del(&process_node->list);
			kfree(process_node);
			continue;
		} else {
			flag=1;
		} //Skip alternative entry
	}

	console_print(128,"Process List from custom list (alternative entry)\r\n") ;
	console_print(128,"PID \t Name \t Parent \t CPU \t Priority \r\n");
	list_for_each_entry(process_node,&process_head,list) {


		console_print(128,"%d \t %s \t %s \t %d \t %d \r\n", 
										process_node -> pid,
										process_node -> name,
										process_node -> parent,
										process_node -> cpu,
										process_node -> priority);
	}

	return 0;
}

static void __exit stop(void) {
	struct node *process_node,*tmp;

	list_for_each_entry_safe(process_node,tmp,&process_head,list) {
		list_del(&process_node->list);
		kfree(process_node);
	}
	console_print(128,"Module exit\r\n");
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