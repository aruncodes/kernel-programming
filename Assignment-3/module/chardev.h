#ifndef CHARDEV_H
#define CHARDEV_H 

#include <linux/mutex.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "chardev"

/* lock for read write access */
static DEFINE_MUTEX(rw_lock);

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);


static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  How manu times */

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

int register_char_device(void) {
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

void unregister_char_device(void) {
	unregister_chrdev(Major, DEVICE_NAME);
}


static int device_open(struct inode *inode, struct file *file)
{
	// if (Device_Open)
		// return -EBUSY;

	Device_Open++;

	printk(KERN_INFO "Device opened\n");
	try_module_get(THIS_MODULE); //Increment usage count

	return 0;//SUCCESS
}

static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;

	module_put(THIS_MODULE); // Decrement Count
	printk(KERN_INFO "Device released\n");

	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length,loff_t * offset)
{
	int bytes_read = 0;

	if (mutex_lock_interruptible(&rw_lock)) // Aquire lock
		return -ERESTARTSYS;

	while (length && !is_buffer_empty() ) { //Read until empty
		put_user(read_buffer(), buffer++);

		length--;
		bytes_read++;
	}

	mutex_unlock(&rw_lock); //Release lock

	return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	int bytes_wrote = 0;
	unsigned char ch;

	if (mutex_lock_interruptible(&rw_lock))// Aquire lock
		return -ERESTARTSYS;

	while( len && !is_buffer_full() ) { //Write until full
		get_user(ch, buff++);

		write_buffer(ch);
		len--;
		bytes_wrote++;
	}

	mutex_unlock(&rw_lock); //Release lock

	return bytes_wrote ? bytes_wrote : -ENOMEM; //Return no memory if nothing can be written
}

#endif