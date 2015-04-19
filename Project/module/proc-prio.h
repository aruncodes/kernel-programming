#ifndef PROC_PRIO_H
#define PROC_PRIO_H 

#define NPROC 		50

#define DELETED 	-2
#define NOT_USED	-1

struct proc_info {
	int pid;
	unsigned long deadline;
};

struct proc_info proc_array[NPROC];
int i;

unsigned long get_prio(int pid) {
	for (i = 0; i < NPROC; ++i) {
		if(proc_array[i].pid == NOT_USED) break; 

		if(proc_array[i].pid == pid) {
			return proc_array[i].deadline;
		}
	}
	return 0;
}
void init_prio(void) {
	for (i = 0; i < NPROC; ++i)	{
		proc_array[i].pid = NOT_USED;
	}
}

void add_update_proc(int pid,unsigned long deadline) {

	for (i = 0; i < NPROC; ++i)
	{
		if(proc_array[i].pid < 0 /*Deleted or Not used*/ 
			|| proc_array[i].pid == pid) {
			proc_array[i].pid = pid;
			proc_array[i].deadline = deadline;
			break;
		}
	}
}

void invalidate(int pid) {
	for (i = 0; i < NPROC; ++i) {
		if(proc_array[i].pid == pid) {
			proc_array[i].pid = DELETED;
			break;
		}
	}
}

#endif