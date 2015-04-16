#ifndef _ADDR_LIST_H
#define _ADDR_LIST_H 

#include <linux/list.h>
#include <linux/slab.h>

struct node {
	unsigned long addr,end_addr;

	struct list_head list; /* kernel's list structure */
};

LIST_HEAD(addr_head);

void add_addr(unsigned long addr,unsigned long end_addr) {

	struct node *addr_node;

	addr_node = kmalloc(sizeof(struct node),GFP_KERNEL);
	addr_node -> addr = addr;
	addr_node -> end_addr = end_addr;

	list_add(&(addr_node->list),&addr_head);
}

void dealloc_list(void) {

	struct node *addr_node,*tmp;

	list_for_each_entry_safe(addr_node,tmp,&addr_head,list) {
		list_del(&addr_node->list);
		kfree(addr_node);
	}
}

#endif