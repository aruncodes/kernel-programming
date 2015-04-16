/*
	This program will create array of PAGES elements each of size PAGESIZE. 
	It will access random number of array elements every 10 seconds.
*/

#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"

#define	PAGES		50
#define	PAGESIZE	4 * 1024


//Each page takes PAGESIZE bytes
struct page{
	char data[ PAGESIZE ];
};

//A map with some pages
struct page map[PAGES];

int main()
{
	int i,n;

	while(1) {

		n = rand() % PAGES;

		printf("Accessing %d elements in array\n",n);
		for (i = 0; i < n; ++i){
			map[i].data[0] = '1';
			map[i].data[1] = '2';
			map[i].data[2] = '3';
			map[i].data[PAGESIZE -1] = 'N';
		}

		sleep(10);
	}

	return 0;
}