#ifndef SYSFS_ENTRY_H
#define SYSFS_ENTRY_H 

#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "proc-deadline.h"

unsigned long READ_DEADLINE = 200; //Microseconds
unsigned long WRITE_DEADLINE = 300; //Microseconds

/* Proc deadline show and store */
static ssize_t proc_deadline_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	int ret = 0,i;
	ret += sprintf(buf,"%4s %10s\n","PID","Deadline(usec)");

	for (i = 0; i < NPROC; ++i) {
		if(proc_array[i].pid > 0) { /*Neither Deleted nor Not used*/ 
			ret += sprintf(buf + ret,"%4d %8luus\n",proc_array[i].pid,proc_array[i].deadline );
		}
	}
	if(ret < 20)ret += sprintf(buf+ret,"\nNo deadline entries added!"); 

	ret += sprintf(buf+ret,"\n\nTo add/change deadline,\n\techo 'pid deadline_in_us' > proc_deadline");
	ret += sprintf(buf+ret,"\nTo remove entry, \n\techo 'pid 0' > proc_deadline\n");

	return ret;
}
static ssize_t proc_deadline_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	int pid;
	unsigned long deadline;
	if(sscanf(buf,"%d %ld",&pid,&deadline)) {
		if(deadline > 0) {
			add_update_proc(pid,deadline);
			printk(KERN_INFO"Added %d %lu\n",pid,deadline);
		} else {
			invalidate(pid);
			printk(KERN_INFO"Removed %d\n",pid);
		}

		return count;
	} 
	return 0;
}

/*
 * The "read_deadline" file .
 */
static ssize_t read_deadline_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%ld usec\n",READ_DEADLINE);
}

static ssize_t read_deadline_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	if(sscanf(buf,"%ld",&READ_DEADLINE)) return count;

	return 0;
}

/*
 * The "write_deadline" file .
 */
static ssize_t write_deadline_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%ld usec\n",WRITE_DEADLINE);
}

static ssize_t write_deadline_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count)
{
	if(sscanf(buf,"%ld",&WRITE_DEADLINE)) return count;

	return 0;
}

/* Sysfs attributes cannot be world-writable. */
static struct kobj_attribute read_deadline_attribute =
	__ATTR(read_deadline, 0664, read_deadline_show, read_deadline_store);

static struct kobj_attribute write_deadline_attribute =
	__ATTR(write_deadline, 0664, write_deadline_show, write_deadline_store);

static struct kobj_attribute proc_deadline_attribute =
	__ATTR(proc_deadline, 0664, proc_deadline_show, proc_deadline_store);


/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *attrs[] = {
	&read_deadline_attribute.attr,
	&write_deadline_attribute.attr,
	&proc_deadline_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *sstf_rw_deadline_kobj;

static int init_sysfs(void)
{
	int retval;	

	sstf_rw_deadline_kobj = kobject_create_and_add("sstf_rw_deadline", NULL);
	if (!sstf_rw_deadline_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(sstf_rw_deadline_kobj, &attr_group);
	if (retval)
		kobject_put(sstf_rw_deadline_kobj);

	return retval;
}

static void exit_sysfs(void)
{
	kobject_put(sstf_rw_deadline_kobj);
}


#endif