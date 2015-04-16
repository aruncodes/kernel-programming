#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/mm.h>  /* mmap related stuff */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define DEVICE_NAME "mmapchardev"

static DEFINE_MUTEX(rw_lock);

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  How manu times */
static long write_offset=0,read_offset=0;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

struct mmap_info {
	char *data;	/* the data */
	int reference;       /* how many times it is mmapped */  	
};


/* keep track of how many times it is mmapped */

void mmap_open(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference++;
}

void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference--;
}

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;

	/* the data is in vma->vm_private_data */
	info = (struct mmap_info *)vma->vm_private_data;
	if (!info->data) {
		printk("no data\n");
		return 0;	
	}

	/* get the page */
	page = virt_to_page(info->data);
	
	/* increment the reference count of this page */
	get_page(page);
	vmf->page = page;					//--changed

	printk(KERN_INFO "Page fault served!");
	/* type is the page fault type */
	return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.open =     mmap_open,
	.close =    mmap_close,
	.fault =    mmap_fault,
};

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	/* assign the file private data to the vm private data */
	vma->vm_private_data = filp->private_data;
	mmap_open(vma);
	return 0;
}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.mmap = my_mmap,
};

static int __init mmap_module_init(void)
{

	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}

	printk(KERN_INFO "Character device was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return 0;
}

static void __exit mmap_module_exit(void)
{
	unregister_chrdev(Major, DEVICE_NAME);

}

module_init(mmap_module_init);
module_exit(mmap_module_exit);

static int device_open(struct inode *inode, struct file *filp)
{
	struct mmap_info *info;

	if(Device_Open) {
		return -EBUSY;
	}
	Device_Open++;

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);

    info->data = (char *)get_zeroed_page(GFP_KERNEL); /* obtain new memory */
	
	memcpy(info->data, "hello from kernel this is file: ", 32);
	memcpy(info->data + 32, filp->f_dentry->d_name.name, strlen(filp->f_dentry->d_name.name));
	
	filp->private_data = info; /* assign this info struct to the file */
	
	write_offset = 0;
	read_offset = 0;


	printk(KERN_INFO "Device opened\n");
	try_module_get(THIS_MODULE); //Increment usage count

	return 0;//SUCCESS
}

static int device_release(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = filp->private_data;

	printk(KERN_INFO "Content in mmap : %s \n",info->data);

	free_page((unsigned long)info->data); /* obtain new memory */
	kfree(info);
	filp->private_data = NULL;

	Device_Open--;

	module_put(THIS_MODULE); // Decrement Count
	printk(KERN_INFO "Device released\n");

	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length,loff_t * offset)
{
	struct mmap_info *info;
	int ret=0;

	if(read_offset >= write_offset) return 0;

	if (mutex_lock_interruptible(&rw_lock)) // Aquire lock
		return -ERESTARTSYS;

	info = filp -> private_data;

	if(read_offset + length < PAGE_SIZE) {
		memcpy(buffer,info->data + read_offset,length);
		ret = length;
	}
	else {
		memcpy(buffer,info->data + read_offset,(read_offset + length) - PAGE_SIZE);
		ret = (read_offset + length) - PAGE_SIZE ;
	}

	read_offset = (read_offset + length) % PAGE_SIZE;

	mutex_unlock(&rw_lock); //Release lock

	return length;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	struct mmap_info *info;
	int ret=0;

	if (mutex_lock_interruptible(&rw_lock))// Aquire lock
		return -ERESTARTSYS;

	info = filp -> private_data;

	if(write_offset + len < PAGE_SIZE) {
		memcpy(info->data + write_offset,buff,len);
		ret = len;
	}
	else {
		memcpy(info->data + write_offset,buff, (write_offset + len) - PAGE_SIZE );
		ret = (write_offset + len) - PAGE_SIZE ;
	}

	write_offset = (write_offset + len) % PAGE_SIZE;

	mutex_unlock(&rw_lock); //Release lock

	return ret;
}