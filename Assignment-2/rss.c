#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <linux/highmem.h>
#include <asm/pgtable.h>

#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

// Input process id as argument
static int pid = -1;
module_param(pid,int,0);
MODULE_PARM_DESC(pid,"Process ID");

//No. of times rss should be calculated
static int ntimes = -1;
module_param(ntimes,int,0);
MODULE_PARM_DESC(ntimes,"Interval");

struct task_struct	*task; //Struct to process
struct vm_area_struct *vm_area,*vmstart; //VM area pointers
unsigned long rss_sum=0,vma_sum=0; //RSS and VMA
unsigned long addr; //VM address
static struct timer_list rss_timer; //Timer to calculate rss in periodic intervals
int ret;

static void calculate_rss(unsigned long data); //Calculate RSS

static pte_t *walk_page_table(struct mm_struct *mm,unsigned long addr); //Function to walk and find PTE from page table

static int __init start(void) {
	// Can be verifed using /proc/meminfo and /proc/vmstat. 
	//PAGE_SIZE = Mapped/nr_mapped

	printk(KERN_INFO "Page size : %lu", PAGE_SIZE); 

	task = pid_task(find_vpid(pid),PIDTYPE_PID);

	if(!task || pid == -1) {
		printk(KERN_INFO "Couldn't get process with that pid \n");
		return 1;
	}

	printk(KERN_INFO "Process name with PID %d is : %s\n",pid, task -> comm);

	setup_timer( &rss_timer, calculate_rss, 0 ); //Setup timer

	calculate_rss(0); // Calculate RSS and set timer

	return 0;
}

static void __exit stop(void) {
	printk(KERN_INFO "Module exit\n");
}

module_init(start);
module_exit(stop);

static void calculate_rss(unsigned long data) {

	vm_area = task -> mm -> mmap; //Get VM start from mm_struct

	vmstart = vm_area;
	while (1) { // Iterate over all VMAs

		//Print range of VMA
		// printk(KERN_INFO "%lx - %lx - %-luKB \n",
			// vmstart->vm_start,vmstart->vm_end,(-(vmstart->vm_start-vmstart->vm_end))/1024);

		vma_sum += (vmstart->vm_end-vmstart->vm_start); //Sum to find out VM use

		//Iterate over VMA with PAGESIZE step
		for (addr = vmstart->vm_start; addr < vmstart->vm_end; addr += PAGE_SIZE) {

			pte_t *pte = walk_page_table(task->mm,addr); //Get PTE by walking page table

			 //Only count present addresses:
			if(pte && (pte_val(*pte) & _PAGE_PRESENT)) {
				rss_sum++;
			}
			if(pte) pte_unmap(pte); //unmap accessed page
		}

		vmstart = vmstart ->vm_next; //Next VMA in list
		if(vmstart == NULL || vmstart == vm_area) {
			break;
		}
	}
	printk(KERN_INFO "RSS : %lu VMA : %lu \n", rss_sum*PAGE_SIZE/1024, vma_sum/1024);
	
	//Set timer again
	if( --ntimes > 0) {
		rss_sum = vma_sum = 0;
		ret = mod_timer( &rss_timer, jiffies + msecs_to_jiffies(10 * 1000) ); //Timer for 10 seconds
	  	if (ret) printk("Error in mod_timer\n");
	}
}


static pte_t *walk_page_table(struct mm_struct *mm,unsigned long addr)
{
    pgd_t *pgd;
    pte_t *pte;
    pud_t *pud;
    pmd_t *pmd;

    pgd = pgd_offset(mm, addr); //Get page global directory offset
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return NULL;

    pud = pud_offset(pgd, addr); //Page Upper Directory
    if (pud_none(*pud) || pud_bad(*pud))
        return NULL;

    pmd = pmd_offset(pud, addr); //Page Mid Directory
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return NULL;

    pte = pte_offset_map(pmd, addr); //Page table entry
   	
   	return pte;
}