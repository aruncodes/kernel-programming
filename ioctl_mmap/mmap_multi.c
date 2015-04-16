#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mm.h>  /* mmap related stuff */
#include <linux/mutex.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include "ioctl_config.h"

static DEFINE_MUTEX(rcount_lock);
static DEFINE_MUTEX(w_lock);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

#define N_PAGES 	100

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define DEVICE_NAME 	"mmap_multi_ioctl"

static unsigned long alloted_pages[N_PAGES];

//---sysfs
int enable_mmap = 0;
int num_pages = 0;
static int init_sysfs(void);
static void exit_sysfs(void);
//---sysfs-end

///-----------DEV_METHODS----------------
static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  How manu times */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	pgoff_t page_id = vmf -> pgoff;

	if(alloted_pages[page_id] == 0) {
		//Not yet allocated
		alloted_pages[page_id] = get_zeroed_page(GFP_KERNEL);
		memcpy((char *)alloted_pages[page_id], "hello from kernel.. ", 20);
	}

	page = virt_to_page(alloted_pages[page_id]);
	
	get_page(page); /* increment the reference count of this page */
	vmf->page = page;

	printk(KERN_INFO "Page fault served! Missing page %lu\n",vmf->pgoff);

	return 0;
}

static int reader_count=0;
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {

	switch(cmd) {
		case READ_LOCK_MMAP:

			if (mutex_lock_interruptible(&rcount_lock)) // Aquire read count lock
				return -ERESTARTSYS;
			
			reader_count++;

			if(reader_count == 1) { //First reader entering
				if (mutex_lock_interruptible(&w_lock)) // Aquire write lock
					return -ERESTARTSYS;
			}
			
			mutex_unlock(&rcount_lock); //Release lock

			break;

		case READ_UNLOCK_MMAP:

			if (mutex_lock_interruptible(&rcount_lock)) // Aquire read count lock
				return -ERESTARTSYS;
			
			reader_count--;

			if(reader_count == 0) { ///Last person leaving
				mutex_unlock(&w_lock); //Release write lock
			}
			
			mutex_unlock(&rcount_lock); //Release lock

			break;

		case WRITE_LOCK_MMAP:
			if (mutex_lock_interruptible(&w_lock)) // Aquire write lock
				return -ERESTARTSYS;
			// if (mutex_lock_interruptible(&rcount_lock)) // Aquire read count lock
				// return -ERESTARTSYS;
			break;

		case WRITE_UNLOCK_MMAP:
			// mutex_unlock(&rcount_lock); //Release rcount lock
			mutex_unlock(&w_lock); //Release write lock
			break;

	}
	return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.fault =    mmap_fault,
};

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int n_pages;

	if(!enable_mmap) {
		printk("MMAP disabled. Please set sysfs entry\n");
		return -EAGAIN; //try again 
	}

	n_pages = (vma->vm_end - vma->vm_start)/PAGE_SIZE;
	if(num_pages < n_pages) {
		printk("Request contain too many pages in VMA. Increase limit in sysfs.\n");
		return -EAGAIN;
	}

	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	
	printk(KERN_INFO "VMA with size %lu, %lu pages\n",vma->vm_end - vma->vm_start,(vma->vm_end - vma->vm_start)/PAGE_SIZE);
	return 0;
}

static struct file_operations fops = {
	.open = device_open,
	.release = device_release,
	.mmap = my_mmap,
	.unlocked_ioctl = my_ioctl,
};

static int device_open(struct inode *inode, struct file *filp)
{
	Device_Open++;
	
	printk(KERN_INFO "Device opened\n");
	try_module_get(THIS_MODULE); //Increment usage count

	return 0;//SUCCESS
}

static int device_release(struct inode *inode, struct file *filp)
{

	Device_Open--;

	module_put(THIS_MODULE); // Decrement Count
	printk(KERN_INFO "Device released\n");

	return 0;
}
//-------------DEV_END--------------

//-------------MODULE_METHODS-----------
static int __init mmap_module_init(void)
{
	int i;

	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}

	printk(KERN_INFO "Character device was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Remove the device file and module when done.\n");

	init_sysfs(); //Enable sysfs

	for (i = 0; i < N_PAGES; ++i)  //Init all alloted addresses to 0
		alloted_pages[i] = 0;

	return 0;
}

static void __exit mmap_module_exit(void)
{
	int i;

	unregister_chrdev(Major, DEVICE_NAME);
	exit_sysfs(); //Remove sysfs

	for (i = 0; i < N_PAGES; ++i) {
		if(alloted_pages[i] != 0) { //If allocated, print content and free page
			printk(KERN_INFO "Content in mmap %d : %s \n",i,(char *)alloted_pages[i]);
			free_page(alloted_pages[i]); 
		}
	}
}

module_init(mmap_module_init);
module_exit(mmap_module_exit);
//-------------MODULE-END-------------

//-------------SYSFS-------------------
//Methods for sys fs
static ssize_t var_show(struct kobject *kobj, struct kobj_attribute *attr,
	char *buf) {
	int var;

	if (strcmp(attr->attr.name, "enable_mmap") == 0)
		var = enable_mmap;
	else
		var = num_pages;
	return sprintf(buf, "%d\n", var);
}

static ssize_t var_store(struct kobject *kobj, struct kobj_attribute *attr,
	const char *buf, size_t count) {
	int var;

	sscanf(buf, "%d", &var);
	if (strcmp(attr->attr.name, "enable_mmap") == 0)
		enable_mmap = var;
	else
		num_pages = var;
	return count;
}

static struct kobj_attribute enable_attribute = __ATTR(enable_mmap,0664, var_show, var_store);
static struct kobj_attribute pages_attribute = __ATTR(num_pages,0664, var_show, var_store);

static struct attribute *attrs[] = {
          &enable_attribute.attr,
          &pages_attribute.attr,
          NULL,   /* need to NULL terminate the list of attributes */
  };
static struct attribute_group attr_group = {
         .attrs = attrs,
 };
static struct kobject *mmap_kobj;

static int init_sysfs(void) {
        int retval;

        mmap_kobj = kobject_create_and_add("mmap_config", kernel_kobj);
        if (!mmap_kobj)
                return -ENOMEM;

        /* Create the files associated with this kobject */
        retval = sysfs_create_group(mmap_kobj, &attr_group);
        if (retval)
                kobject_put(mmap_kobj);

        return retval;
}

static void exit_sysfs(void) {
        kobject_put(mmap_kobj);
}

//-------------SYSFS END----------------