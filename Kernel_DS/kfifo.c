#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kfifo.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

DEFINE_KFIFO(queue,int,1024);

DEFINE_KFIFO(byte_queue,unsigned char,128);

struct task_struct	*task;
int pid;

static void producer(void);
static void consumer(void);
static void test_byte_queue(void);

int console_print(size_t size, const char *fmt, ...);

static int __init start(void) {
	console_print(128, "Module started\r\n");

	producer();
	consumer();

	console_print(128, "List from task_structs \r\n");
	for_each_process(task) {
		console_print(64,"%d \t", task -> pid);
	}

	test_byte_queue();

	return 0;
}

static void __exit stop(void) {
	console_print(128, "Module exit\r\n");
}

static void producer(void) {

	for_each_process(task) {

		kfifo_put(&queue, task -> pid);
	}
	console_print(128, "List populated, queue length = %d\r\n",kfifo_len(&queue));
}

static void consumer(void) {

	console_print(128, "List from consumer \r\n");
	while(kfifo_get(&queue,&pid)) {
		console_print(128, "%d \t",pid);
	}
}

static void test_byte_queue(void) {

	unsigned char a,b,c;
	unsigned char buf[20];
	int ret;

	//Insert 2 ints
	kfifo_put(&byte_queue, (unsigned char)10);
	kfifo_put(&byte_queue, (unsigned char)40);

	//Insert a string
	kfifo_in(&byte_queue, "hello world",11);

	//Insert 3 chars
	kfifo_put(&byte_queue, 'x');
	kfifo_put(&byte_queue, 'y');
	kfifo_put(&byte_queue, 'z');


	//Print all
	ret = kfifo_get(&byte_queue,&a);
	ret = kfifo_get(&byte_queue,&b);

	console_print(128, "Ints: %d, %d\r\n",a,b);

	ret = kfifo_out(&byte_queue,buf,11);
	buf[11] = '\0';
	console_print(128, "String : %s\r\n",buf);

	ret = kfifo_get(&byte_queue,&a);
	ret = kfifo_get(&byte_queue,&b);
	ret = kfifo_get(&byte_queue,&c);
	console_print(128, "Chars: %c, %c, %c\r\n",a,b,c);
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