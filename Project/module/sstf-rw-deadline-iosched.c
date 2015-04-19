/*
 * elevator sstf with read and write deadlines
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>

#include "sysfs-entry.h"

struct sstf_data {
	struct list_head queue;
};

//Store the last dispatched sector
sector_t last_dispatch_sector = 0;

static void sstf_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int sstf_dispatch(struct request_queue *q, int force)
{
	struct sstf_data *nd = q->elevator->elevator_data;
	sector_t sector,min=0;
	long abs;
/*
	if (!list_empty(&nd->queue)) {
		struct request *rq;
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
*/
	if (!list_empty(&nd->queue)) {
		struct request *rq, *next_rq = NULL;

		//Iterate over request queue and find the shortest		
		list_for_each_entry(rq,&nd->queue,queuelist) {

			//Get sector
			sector = blk_rq_pos(rq);

			//Check if deadline is past, if so, then dispatch immediately
			if(time_before(rq->deadline,jiffies)) {
				printk(KERN_INFO "Deadline reached!");

				next_rq = rq;
				last_dispatch_sector = sector; 
				break;
			}

			//Init vars
			if(min == 0) {
				min = sector;

				//Just an optimization to check last dispatch inside this
				if(last_dispatch_sector == 0) 
					last_dispatch_sector = sector;
			} 

			//Find absolute diff between last dispatch
			abs = last_dispatch_sector - sector; 
			abs = abs < 0 ? -abs : abs;

			//Update min distance 
			if(min >= abs) {
				min = abs;
				next_rq = rq;
			}

			printk(KERN_INFO "Queue Item : %lu Diff: %ld \n", (unsigned long )sector, abs );
		}

		//Ensure next_rq will not be uninitialized
		if(!next_rq) next_rq = list_entry(nd->queue.next, struct request, queuelist);

		//Update global var
		last_dispatch_sector = blk_rq_pos(next_rq);

		//Print debug
		printk(KERN_INFO "Dispatched : %lu\n", (unsigned long )last_dispatch_sector);

		//Empty queue list and re initialize before dispatching
		list_del_init(&next_rq->queuelist);

		//Dispatch
		elv_dispatch_sort(q,next_rq);

		return 1;
	}
	
	return 0;
}

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *nd = q->elevator->elevator_data;
	unsigned long deadline;

	//Read flag of comming request to see if its a read or write
	int write_request = rq->cmd_flags & REQ_WRITE;

	if(write_request) {
		//Write request
		// deadline = (WRITE_DEADLINE * HZ) / 1000;
		deadline = msecs_to_jiffies(WRITE_DEADLINE);
	} else {
		//Read request
		// deadline = (READ_DEADLINE* HZ) / 1000;
		deadline = msecs_to_jiffies(READ_DEADLINE);
	}

	//Update deadline value
	rq -> deadline = jiffies + deadline;

	list_add_tail(&rq->queuelist, &nd->queue);
}

static struct request *
sstf_former_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
sstf_latter_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct sstf_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *e)
{
	struct sstf_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_noop = {
	.ops = {
		.elevator_merge_req_fn		= sstf_merged_requests,
		.elevator_dispatch_fn		= sstf_dispatch,
		.elevator_add_req_fn		= sstf_add_request,
		.elevator_former_req_fn		= sstf_former_request,
		.elevator_latter_req_fn		= sstf_latter_request,
		.elevator_init_fn		= sstf_init_queue,
		.elevator_exit_fn		= sstf_exit_queue,
	},
	.elevator_name = "sstf-rw-dl",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
	init_sysfs();
	return elv_register(&elevator_noop);
}

static void __exit sstf_exit(void)
{
	exit_sysfs();
	elv_unregister(&elevator_noop);
}

module_init(sstf_init);
module_exit(sstf_exit);


MODULE_AUTHOR("Arun Babu");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SSTF with Read-Write Deadline IO scheduler");
