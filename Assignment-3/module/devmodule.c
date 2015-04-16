#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include "circular_buffer.h"
#include "chardev.h"
#include "procfile.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

#define CIRCULAR_QUEUE_SIZE	512

static int __init start(void) {

	//Init circular queue for character device 
	//(my own implementation in circular_buffer.h)
	if( !init_buffer(CIRCULAR_QUEUE_SIZE) ) 
		printk("Buffer could not be initalized."); 

	//Register character device
	register_char_device(); // See chardev.h

	//Create a proc file entry
	create_proc_file(); // See procfile.h

	return 0;
}

static void __exit stop(void) {
	free_buffer(); //remove memory allocated
	unregister_char_device();
	remove_proc_file();
	
	printk(KERN_INFO "Module exit\n");
}

module_init(start);
module_exit(stop);
