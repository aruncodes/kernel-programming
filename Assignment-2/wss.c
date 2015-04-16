#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <linux/highmem.h>
#include <asm/pgtable.h>
#include <linux/timer.h>

#include "addr_list.h" //Customized Kernel linked list implementation

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arun Babu");

// Input process id as argument
static int pid = -1;
module_param(pid,int,0);
MODULE_PARM_DESC(pid,"Process ID");

//No. of times wss should be calculated
static int ntimes = -1;
module_param(ntimes,int,0);
MODULE_PARM_DESC(ntimes,"Interval");

struct task_struct	*task; //Struct to process
struct vm_area_struct *vm_area,*vmstart; //VM area pointers
unsigned long rss_sum=0,vma_sum=0; //RSS and VMA
unsigned long addr; //VM address
unsigned long wss_count=0; //WSS wss_count
spinlock_t *lck; //To lock PTE
static struct timer_list wss_timer; //Timer to calculate wss in periodic intervals
int ret;

//Find name of vm area
unsigned char *name;
void get_vm_file_name(struct vm_area_struct *vm_start);

//Hook inside mm/fault.c
extern int (*pf_hook)(struct task_struct *tsk,unsigned long addr);
int pf_hook_implementation(struct task_struct *tsk,unsigned long addr);

void set_pte_bits(void);
void reset_all_ptes(void);
void wss_service(unsigned long data); 

//Function to walk and find PTE from page table
static pte_t *get_locked_pte_(struct mm_struct *mm,unsigned long addr,spinlock_t **);

static int __init start(void) {
	// Can be verifed using /proc/meminfo and /proc/vmstat. 
	//PAGE_SIZE = Mapped/nr_mapped
	printk(KERN_INFO "Module started..\nPage size : %lu", PAGE_SIZE); 

	task = pid_task(find_vpid(pid),PIDTYPE_PID);

	if(!task || pid == -1) {
		printk(KERN_INFO "Couldn't get process with that pid \n");
		return 1;
	}

	pf_hook = &pf_hook_implementation; // Enable hook

	printk(KERN_INFO "Process name with PID %d is : %s\n",pid, task -> comm);

	setup_timer( &wss_timer, wss_service, 0 ); //Setup timer
	wss_service(0); //Start first estimation
	
	return 0;
}

static void __exit stop(void) {

	reset_all_ptes(); //Reset all PTEs
	dealloc_list(); //Dealocate list
	pf_hook = NULL; //Disable hook

	printk(KERN_INFO "Module exit\n");
}

module_init(start);
module_exit(stop);

void wss_service(unsigned long data) { //Peridoically runs and estimates WSS for 10 seconds

	//Print result if ran previously
	if(rss_sum > 0) {
		printk(KERN_INFO "WSS : %lu RSS : %lu VMA : %lu\n", wss_count,rss_sum*PAGE_SIZE/1024, vma_sum/1024);
	}

	//Clear any previous PTEs
	reset_all_ptes(); //Reset all PTEs
	dealloc_list(); //Dealocate list

	if(ntimes < 0){
		printk("Completed.. Please remove module.\n");
		return;	
	} 

	wss_count = rss_sum = vma_sum = 0;
	set_pte_bits(); // Set PTE bits

	//Set timer again
	ret = mod_timer( &wss_timer, jiffies + msecs_to_jiffies(10 * 1000) ); //Timer for 10 seconds
  	if (ret) printk("Error in mod_timer\n");
  	 --ntimes;
}

//Walk through all VM areas and mark the present pages as not present and protected
void set_pte_bits() {
	vm_area = task -> mm -> mmap; //Get VM start from mm_struct

	vmstart = vm_area;
	while (1) { // Iterate over all VMAs
		vma_sum += (vmstart->vm_end-vmstart->vm_start); //Sum to find out VM use

		//Iterate over VMA with PAGESIZE step
		for (addr = vmstart->vm_start; addr < vmstart->vm_end; addr += PAGE_SIZE) {

			pte_t *pte = get_locked_pte_(task->mm,addr,&lck),tmp_pte; //Get PTE by walking page table

			 //Only wss_count present addresses with no protection:
			if(pte && (pte_val(*pte) & _PAGE_PRESENT) && !((pte_val(*pte) & _PAGE_PROTNONE))) {
					rss_sum++;

					tmp_pte = *pte; 
					tmp_pte = pte_set_flags(tmp_pte, _PAGE_PROTNONE);	
					tmp_pte = pte_clear_flags(tmp_pte, _PAGE_PRESENT);

					set_pte(pte , tmp_pte);
					
					add_addr(addr,addr + PAGE_SIZE);
			}

			if(pte) pte_unmap_unlock(pte,lck); //unlock accessed page
		}

		vmstart = vmstart ->vm_next; //Next VMA in list
		if(vmstart == NULL || vmstart == vm_area) {
			break;
		}
	}

}

//Hook function which will be called from kernel when page fault happens
int pf_hook_implementation(struct task_struct *tsk,unsigned long addr) {
	pte_t *pte,tmp_pte;
	struct node *addr_node;

	
	if(!task || !task -> mm) {

		printk(KERN_INFO "Task struct null :- Process exited before module.. ");

		dealloc_list(); //Dealocate list
		pf_hook = NULL; //Disable hook

		printk(KERN_INFO "WSS : %lu RSS : %lu VMA : %lu\n", wss_count,rss_sum*PAGE_SIZE/1024, vma_sum/1024);
		
		return 0;
	}

	if(tsk -> pid == pid) {
		pte = get_locked_pte_(tsk->mm,addr,&lck); //Get PTE by walking page table

		if(!pte) {
			printk(KERN_INFO"Cant get PTE!\n");
			return 1;
		}

		//Check if actually set by our module
		if(pte && (pte_val(*pte) & _PAGE_PROTNONE) && !(pte_val(*pte) & _PAGE_PRESENT)) {
		
			list_for_each_entry(addr_node,&addr_head,list) {
				if( addr_node-> addr <= addr && addr < addr_node->end_addr) {
					// printk(KERN_INFO "Page Fault at : %lx \n",addr);

					//Change the bits to normal
					tmp_pte = *pte; 
					tmp_pte = pte_set_flags(tmp_pte, _PAGE_PRESENT);
					tmp_pte = pte_clear_flags(tmp_pte, _PAGE_PROTNONE);	
					set_pte(pte , tmp_pte);

					wss_count++;
					break;
				}
			}
		}
		pte_unmap_unlock(pte,lck); //unmap accessed page
	}
	return 0;
}

//Change back all PTEs to normal while cleaning up
void reset_all_ptes() {
	pte_t *pte,tmp_pte;
	struct node *addr_node;

	if(task == NULL || task -> mm == NULL)
		return;
	
	list_for_each_entry(addr_node,&addr_head,list) {
			pte = get_locked_pte_(task->mm,addr_node->addr,&lck);

			//Check if actually set by our module
			if(pte && (pte_val(*pte) & _PAGE_PROTNONE) && !(pte_val(*pte) & _PAGE_PRESENT)) {

				tmp_pte = *pte; 
				tmp_pte = pte_set_flags(tmp_pte, _PAGE_PRESENT);
				tmp_pte = pte_clear_flags(tmp_pte, _PAGE_PROTNONE);	
				set_pte(pte , tmp_pte);

			}
			if(pte) pte_unmap_unlock(pte,lck); //unmap accessed page
	}
	// printk(KERN_INFO "Reset all PTEs");
}

//Return the locked PTE. Similar to kernel implementation of get_locked_pte() in memory.c but it is not exported
static pte_t *get_locked_pte_(struct mm_struct *mm,unsigned long addr,spinlock_t **lck)
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

    pte = pte_offset_map_lock(mm,pmd, addr,lck); //Page table entry
   	
   	return pte;
}

