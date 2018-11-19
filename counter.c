/* counter.c */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "uthread.h"


static unsigned long long counter = 0;
static unsigned long long max_counter = 25;

static pthread_mutex_t mutex;

void *incr(void *arg){
	unsigned int tid = uthread_get_id();

	while(1){
		pthread_mutex_lock(&mutex);
		if(counter >= max_counter){
			pthread_mutex_unlock(&mutex);
			break;
		}
		sleep(1);
		counter = counter + 1;
		printf("Thread number %-2u (I was the %d thread created).  Incrementing counter to %llu\n", tid, (int)arg, counter);
		pthread_mutex_unlock(&mutex);
		uthread_yield();
	}
	return NULL;
}

int main(void){
	printf("Main thread starting now!\n");

	pthread_mutex_init(&mutex, NULL);
	uthread_init();
	int num_threads = 3;

	uthread_t *list = malloc(sizeof(uthread_t) * num_threads);
	int i = 0;
	for(i = 0; i < num_threads; i++){
		list[i] = *uthread_create(incr, (void *)((long)i));
		//printf("Main, thread_list[%d].tid: %u\n", i, list[i].tid);
	}

	
	for(i = 0; i < num_threads; i++){
		printf("Waiting on %u to finish...\n", list[i].tid);
		uthread_join(list[i]);
	}
	

	printf("Final value: %llu\n", counter);

	return 0;
}
