#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H 

#include <linux/slab.h>

unsigned char *buffer;
int capacity;

int front=0,rear=0;
int nelems=0,reads=0,writes=0;

int init_buffer(int size) {

	buffer = kmalloc(sizeof(unsigned char) * size, GFP_KERNEL);
	capacity = size;

	if ( !buffer )
		return 0;
	return 1;
}

int write_buffer(unsigned char c) {

	if(nelems == capacity) { //Full
		return 0; //false
	}

	buffer[rear] = c;
	rear = (rear + 1) % capacity;
	nelems++;
	writes++;

	return 1; //true
}

unsigned char read_buffer(void) {

	int index = front;

	if( nelems == 0 ) { //Empty
		return 0;
	}

	front = (front + 1) % capacity;

	nelems--;
	reads++;

	return buffer[index];
}

void free_buffer(void) {
	kfree(buffer);
}

int buffer_size(void) {
	return nelems;
}

int buffer_capacity(void) {
	return capacity;
}

int is_buffer_full(void) {
	return nelems == capacity;
}

int is_buffer_empty(void) {
	return nelems == 0;
}

int get_reads(void) {
	return reads;
}

int get_writes(void) {
	return writes;
}

int get_front(void) {
	return front;
}

int get_rear(void) {
	return rear;
}

void get_content(char *buf) {
	int i=front,index=0;

	if(is_buffer_empty()) {*buf = '\0'; return;}

	i = front;
	do {
		//Skip common special chars and make single line string
		if( buffer[i] =='\0' || buffer[i] == '\n' || buffer[i] == '\r')
			buf[index++] = ' ';
		else
			buf[index++] = buffer[i];
		i = (i+1)% capacity;
		
	} while(i != rear);
	buf[index] = '\0';

}

#endif