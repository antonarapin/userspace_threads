#include "uthread.h"

#include <stdlib.h> // malloc and other things
#include <stdio.h> // some debug printing


// Number of bytes in a Mebibyte, used for thread stack allocation
#define MEM 1048567


// The threads are managed with a double-linked list
static int num_threads = 1; // This is also used to count the number of linked list nodes and give thread_ids
static ucontext_t *parent_ctx;
static ucontext_t *sched_ctx;
ucontext_t tmpctx;


// Inserts a node (at the back) into the circular queue
void insert(node *new_node){
	if(head == NULL){
		head = new_node;
		tail = new_node;
		//printf("head tid: %u\n", head->t->tid);
	} else {
		tail->next = new_node;
		tail = new_node;
	}
	tail->next = head; // circular
}


void print_queue(){
	int counter = 0;
	printf("-- Linked List of Threads --\n");
	node *cur = head;
	while(counter < num_threads-1){
		printf("Node %d with tid %u  state: %d\n", counter, cur->t->tid, cur->t->state);
		cur->visited = 1;
		cur = cur->next;
		counter++;
	}
}

node *get_thread_node(int state){
	node *cur = head;
	int counter = 0;
	while(cur->t->state != state){
		//printf("thread tid: %u state %d\n", cur->t->tid, cur->t->state);
		cur = cur->next;
		counter++;
		if(counter > num_threads){
			return NULL;
		}
	}
	return cur;
}

uthread_t *get_thread(unsigned int tid){
	//printf("Looking for thread with TID %u\n", tid);
	node *cur = head;
	int counter = 0;
	while(cur->t->tid != tid){
		cur = cur->next;
		counter++;
		if(counter > num_threads){
			return NULL;
		}
	}
	return cur->t;
}

node *get_next_ready(node *cur){
	cur = cur->next;
	int counter = 0;
	while(cur->t->state != T_READY){
		cur = cur->next;
		counter++;
		if(counter > num_threads){
			return NULL;
		}
	}
	return cur;
}



// Creates a circular queue node
node *create_node(uthread_t *new_thread){
	node *tmp = (node *) malloc(sizeof(node));
	tmp->next = NULL;
	
	// There is now one more thread (e.g. there was 0, now there is 1)
	// But, the TID of this thread is 0 (not one).  So, increment first, 
	// then subtract 1 to get TID.  This way, there value of num_threads is never wrong

	tmp->t = new_thread;

	insert(tmp); // Insert node into linked list for scheduler

	return tmp;
}

void die(){
	node *new_n = get_thread_node(T_READY);
	node *old_n = get_thread_node(T_ACTIVE);
	old_n->t->state = T_DEAD;
	new_n->t->state = T_ACTIVE;
	swapcontext(old_n->t->ctx,new_n->t->ctx);
}

ucontext_t *gen_die_ctx(){
	// creates a die context by calling die() method that finishes execution by switching to next ready or to blocked waiting to join thread
	// in die(): search for thread that is active, get its blocking tid, make current active dead, make the one that blocking tid points to active
	ucontext_t *new_ctx = malloc(sizeof(ucontext_t));
	getcontext(new_ctx);
 	new_ctx->uc_link=0;
 	new_ctx->uc_stack.ss_sp=malloc(MEM);
 	new_ctx->uc_stack.ss_size=MEM;
 	new_ctx->uc_stack.ss_flags=0;
 	makecontext(new_ctx, die, 0);
 	return new_ctx;
}

uthread_t *uthread_create(void *(*func)(void *), void *argp){
	// Create a new node, with a new thread, and a new context
	// creating context
	ucontext_t *new_ctx = malloc(sizeof(ucontext_t));
	getcontext(new_ctx);
 	new_ctx->uc_link=gen_die_ctx();
 	new_ctx->uc_stack.ss_sp=malloc(MEM);
 	new_ctx->uc_stack.ss_size=MEM;
 	new_ctx->uc_stack.ss_flags=0;
 	makecontext(new_ctx, func, sizeof(argp), argp);
 	//creating thread
 	uthread_t *new_th = malloc(sizeof(uthread_t));
 	new_th->tid = num_threads;
 	num_threads++;
 	new_th->ctx = new_ctx;
 	new_th->blocking_tid = NULL;
 	new_th->state = 1;
 	
 	node *new_n = create_node(new_th);

 	// swap context with thread that created you
 	node *old_n = get_thread_node(T_ACTIVE);
 	old_n->t->state = T_READY;
 	new_th->state = T_ACTIVE;
 	swapcontext(old_n->t->ctx,new_th->ctx);


 	return new_th;


}

void uthread_init(void){
	// Implement the necessary structs and state so that the first call to create above is successful
	// Hint, you can assume that this method is called by the program's main / original thread.
	ucontext_t *main_th_ctx = malloc(sizeof(ucontext_t));
	getcontext(main_th_ctx);
	main_th_ctx->uc_link=gen_die_ctx();
 	main_th_ctx->uc_stack.ss_sp=malloc(MEM);
 	main_th_ctx->uc_stack.ss_size=MEM;
 	main_th_ctx->uc_stack.ss_flags=0;
	uthread_t *new_th = malloc(sizeof(uthread_t));
	new_th->tid = num_threads;
	num_threads++;
 	new_th->ctx = main_th_ctx;
 	new_th->state = 0;
 	new_th->blocking_tid = NULL;
 	node *new_n = create_node(new_th);

}

int uthread_get_id(void){
	// Search through queue for the active thread and return it's tid
	node *n = get_thread_node(T_ACTIVE);
	return n->t->tid;

}


int uthread_yield(void){
	// Just schedule some other thread!
	node *n = get_thread_node(T_ACTIVE);
	int s_tid = n->t->blocking_tid;
	printf("\n\ntid:   \n\n",s_tid);
	if(s_tid!=0){
		uthread_t *th = get_thread(s_tid);
		n->t->state = T_READY;
		th->state = T_ACTIVE;
		printf("\n\nis tid\n\n");
		swapcontext(n->t->ctx,th->ctx);
		
	} else{
		printf("\n\nnull tid\n\n");
		swapcontext(n->t->ctx,n->t->ctx->uc_link);
	}
	printf("\n\nhere?\n\n");
	return NULL;
}


int uthread_join (uthread_t thread){
	// Go find the given thread in the queue, tell it that it is blocking this (active) thread
	// Mark the active thread as blocked
	// Mark the given thread (from the queue) as active
	// swap the context to start the given thread (from the queue)

	int s_tid = thread.tid;
	uthread_t *th = get_thread(s_tid);
	node *n = get_thread_node(T_ACTIVE);
	th->blocking_tid = n->t->tid;
	n->t->state = T_BLOCKED;
	th->state = T_ACTIVE;
	print_queue();
	swapcontext(n->t->ctx, th->ctx);

	return 0;
}

static unsigned long long counter = 0;
static unsigned long long max_counter = 25;

void *incr(void *arg){
	unsigned int tid = uthread_get_id();
	while(1){
		//pthread_mutex_lock(&mutex);
		if(counter >= max_counter){
			//pthread_mutex_unlock(&mutex);
			break;
		}
		sleep(1);
		counter = counter + 1;
		printf("Thread number %-2u (I was the %d thread created).  Incrementing counter to %llu\n", tid, (int)arg, counter);
		uthread_yield();
		//pthread_mutex_unlock(&mutex);
		//uthread_yield(); //pthread_yield on linux
	}
	return NULL;
}

int main(){
	uthread_init();
	int i =0;
	int num_th = 4;
	/*uthread_t thr = *uthread_create(incr, (void *)((long)i));
	uthread_join(thr);*/
	uthread_t *list = malloc(sizeof(uthread_t) * num_th);
	for(i = 0; i < num_th; i++){
		list[i] = *uthread_create(incr, (void *)((long)i));
		//printf("Main, thread_list[%d].tid: %u\n", i, list[i].tid);
	}
	printf("\n\nbefore join\n\n");
	
	uthread_t *th = get_thread(T_ACTIVE);
	node *n = get_thread_node(2);
	swapcontext(th->ctx,n->t->ctx);



	/*
	for(i = 0; i < num_th; i++){
		printf("Waiting on %u to finish...\n", list[i].tid);
		uthread_join(list[i]);
		printf("\nout\n");
	}
	print_queue();*/
	printf("done\n");
	return 0;




}
