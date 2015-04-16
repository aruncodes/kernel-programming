#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/mm.h>  /* mmap related stuff */


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

#define N_PAGES 	100

#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#define DEVICE_NAME "mmap_dyn"

static unsigned long alloted_pages[N_PAGES];

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

struct vm_operations_struct mmap_vm_ops = {
	.fault =    mmap_fault,
};

int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_RESERVED;
	
	printk(KERN_INFO "VMA with size %lu, %lu pages\n",vma->vm_end - vma->vm_start,(vma->vm_end - vma->vm_start)/PAGE_SIZE);
	return 0;
}

static struct file_operations fops = {
	.open = device_open,
	.release = device_release,
	.mmap = my_mmap,
};

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

	for (i = 0; i < N_PAGES; ++i)  //Init all alloted addresses to 0
		alloted_pages[i] = 0;

	return 0;
}

static void __exit mmap_module_exit(void)
{
	int i;

	unregister_chrdev(Major, DEVICE_NAME);

	for (i = 0; i < N_PAGES; ++i) {
		if(alloted_pages[i] != 0) { //If allocated, print content and free page
			printk(KERN_INFO "Content in mmap %d : %s \n",i,(char *)alloted_pages[i]);
			free_page(alloted_pages[i]); 
		}
	}


}

module_init(mmap_module_init);
module_exit(mmap_module_exit);

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
