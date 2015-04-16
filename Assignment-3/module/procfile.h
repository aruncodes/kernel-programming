#ifndef PROCFILE_H
#define PROCFILE_H 

#include <linux/proc_fs.h>

#include "circular_buffer.h"

#define	PROCNAME	"chardev"

static ssize_t procfile_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos) {
	int ret;
	static int read_once=0;
	char content[buffer_capacity() + 1];

	get_content(content); //Get queue content as single line string
	
	ret = sprintf(buf,
	"CHAR DEVICE STATS\n\n\
	Items   \t:- %d \n\
	Capacity\t:- %d \n\
	Reads   \t:- %d \n\
	Writes  \t:- %d \n\
	Front   \t:- %d \n\
	Rear    \t:- %d \n\
	Content \t:- '%s' \n",
	buffer_size(),buffer_capacity(),get_reads(),
	get_writes(),get_front(),get_rear(),content);
	
	//Only allow alternate read, else infinite reads while cating
	return read_once ? (read_once = 0, 0) : (read_once = 1, ret); 
}

static ssize_t procfile_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
	return -EPERM;
}

static const struct file_operations proc_fops = {
	.owner		= THIS_MODULE,
	.read		= procfile_read,
	.write		= procfile_write,
};

int create_proc_file(void) {
	if(proc_create(PROCNAME,0,NULL,&proc_fops)==NULL) {
		return -ENOMEM;
	}

	return 0;
}

void remove_proc_file(void) {
	remove_proc_entry(PROCNAME,NULL);
}

#endif