/* thread-hello.c */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "uthread.h"

void *foo2(void *argp){
	printf("T2 I am another thread!  Here is my input: %d. \n", (int)argp);
	return NULL;
}

void *hello_printer(void *argp){
	int tid = uthread_get_id();
	printf("T1 Hello World! from thread: %d.\n", tid);
	return NULL;
}

int main(void){
	printf("starting\n");

	uthread_init();

	uthread_t *t1 = uthread_create(hello_printer, NULL);
	printf("the TID is: %u.\n", t1->tid);

	// comment out this line to test creating only one thread
	uthread_create(foo2, (void *)3);

	sleep(2); // lazy way to wait for threads to finish 
	printf("MAIN at the very end now.  Goodbye!\n");
	return 0;
}
