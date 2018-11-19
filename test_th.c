#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

#define MEM 1048567 


ucontext_t t1, t2, t3;
ucontext_t mctxt;
int joined = 0;

void f1(){
	joined=1;
	printf("doing stuff and then switching\n");
	setcontext(&mctxt);
}

void join(){
	printf("joining\n");
	joined = 1;
	
	getcontext(&t1);
 	t1.uc_link=0;
 	t1.uc_stack.ss_sp=malloc(MEM);
 	t1.uc_stack.ss_size=MEM;
 	t1.uc_stack.ss_flags=0;
 	makecontext(&t1, (void*)&f1, 0);
	swapcontext(&mctxt,&t1);
	printf("continuing the exec\n");
}





int main(){
	printf("starting execution\n");
	getcontext(&mctxt);
	printf("going to join\n");
	if(joined!=1){
		join();
	}






	printf("done");
	exit(0);
}
