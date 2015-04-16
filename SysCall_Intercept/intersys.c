#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/syscalls.h>

#include <linux/sched.h>

MODULE_LICENSE("GPL");

unsigned long original_cr0;
unsigned long **sys_call_table = (unsigned long**) 0xc1750200;

asmlinkage long (*aruncall_ptr)(const char * str);

asmlinkage long intercepted_aruncall(const char* str) {

	printk(KERN_INFO "Arun call is intercepted..");
	printk(KERN_INFO "Parameter passed is : %s", str);

	return aruncall_ptr(str);
}

static int __init start(void) {
	
	printk(KERN_ALERT "Going to intercept system call");

	//Disable Write protection using cr0 register
	original_cr0 = read_cr0();
	write_cr0(original_cr0 & ~0x00010000);

	aruncall_ptr = (void *)sys_call_table[__NR_aruncall];
	sys_call_table[__NR_aruncall] = (unsigned long *)intercepted_aruncall;

	//Re-enable Write protection using cr0 register
	write_cr0(original_cr0);

	return 0;
}

static void __exit stop(void) {

	//Disable Write protection using cr0 register
	write_cr0(original_cr0 & ~0x00010000);

	sys_call_table[__NR_aruncall] = (unsigned long *)aruncall_ptr;

	//Re-enable Write protection using cr0 register
	write_cr0(original_cr0);
}

module_init(start);
module_exit(stop);