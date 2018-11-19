#include "uthread.h"

#include <stdlib.h>
#include <stdio.h>

#define MEM 1048567

static int num_threads = 1;
static ucontext_t *parent_ctx;
static ucontext_t *sched_ctx;
ucontext_t tmpctx;

void insert(node *new_node){
	if(head == NULL){
		head = new_node;
		tail = new_node;
		//printf("head tid: %u\n", head->t->tid);
	} else {
		tail->next = new_node;
		tail = new_node;
	}
	tail->next = head;
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



node *create_node(uthread_t *new_thread){
	node *tmp = (node *) malloc(sizeof(node));
	tmp->next = NULL;
	tmp->t = new_thread;
	insert(tmp);

	return tmp;
}

void die(){
	node *old_n = get_thread_node(T_ACTIVE);
	if(old_n->t->blocking_tid!=0){
		uthread_t *new_th = get_thread(old_n->t->blocking_tid);
		old_n->t->state = T_DEAD;
		new_th->state = T_ACTIVE;
		swapcontext(old_n->t->ctx,new_th->ctx);
	}else{
		node *new_n = get_thread_node(T_READY);
		old_n->t->state = T_DEAD;
		new_n->t->state = T_ACTIVE;
		swapcontext(old_n->t->ctx,new_n->t->ctx);
	}
	
}

ucontext_t *gen_die_ctx(){
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
	ucontext_t *new_ctx = malloc(sizeof(ucontext_t));
	getcontext(new_ctx);
 	new_ctx->uc_link=gen_die_ctx();
 	new_ctx->uc_stack.ss_sp=malloc(MEM);
 	new_ctx->uc_stack.ss_size=MEM;
 	new_ctx->uc_stack.ss_flags=0;
 	makecontext(new_ctx, func, sizeof(argp), argp);
 	//creating a new thread
 	uthread_t *new_th = malloc(sizeof(uthread_t));
 	new_th->tid = num_threads;
 	// incement the counter so that next threads are inexed properly
 	num_threads++;
 	new_th->ctx = new_ctx;
 	new_th->blocking_tid = NULL;
 	new_th->state = 1;
 	node *new_n = create_node(new_th);

 	// swap context with thread that taht was just created
 	node *old_n = get_thread_node(T_ACTIVE);
 	old_n->t->state = T_READY;
 	new_th->state = T_ACTIVE;
 	swapcontext(old_n->t->ctx,new_th->ctx);

 	return new_th;
}

void uthread_init(void){
	ucontext_t *main_th_ctx = malloc(sizeof(ucontext_t));
	getcontext(main_th_ctx);
	main_th_ctx->uc_link=gen_die_ctx();
 	main_th_ctx->uc_stack.ss_sp=malloc(MEM);
 	main_th_ctx->uc_stack.ss_size=MEM;
 	main_th_ctx->uc_stack.ss_flags=0;
	uthread_t *new_th = malloc(sizeof(uthread_t));
	new_th->tid = num_threads;
	// incement the counter so that next threads are inexed properly
	num_threads++;
 	new_th->ctx = main_th_ctx;
 	new_th->state = 0;
 	new_th->blocking_tid = NULL;
 	node *new_n = create_node(new_th);
}

int uthread_get_id(void){
	node *n = get_thread_node(T_ACTIVE);
	return n->t->tid;
}


int uthread_yield(void){
	node *n = get_thread_node(T_ACTIVE);
	int s_tid = n->t->blocking_tid;
	if(s_tid!=0){
		// this thread is blocking some other thread, so need to switch to the thread that is waiting for this thread to return
		uthread_t *th = get_thread(s_tid);
		n->t->state = T_READY;
		th->state = T_ACTIVE;
		swapcontext(n->t->ctx,th->ctx);
		
	} else{
		// scheduling the next ready thread to run since there is no thread that is blocked by this thread
		node *new_n = get_next_ready(n);
		new_n->t->state = T_ACTIVE;
		n->t->state = T_READY;
		swapcontext(n->t->ctx,new_n->t->ctx);
	}
	return NULL;
}


int uthread_join (uthread_t thread){

	int s_tid = thread.tid;
	uthread_t *th = get_thread(s_tid);
	node *n = get_thread_node(T_ACTIVE);
	// setting the blocking tid for the new thread that is about to be activated
	// so when the newly activated thread finishes its execution, it can switch to the original thread that is blocked and waiting
	th->blocking_tid = n->t->tid;
	n->t->state = T_BLOCKED;
	th->state = T_ACTIVE;
	swapcontext(n->t->ctx, th->ctx);

	return 0;
}
