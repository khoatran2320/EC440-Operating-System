#include "ec440threads.h"
// #include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)
#define ADDRESS_SIZE 8

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY
};

static void schedule(int signal);
void pthread_exit(void *value_ptr);
static int scheduler_init();
int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg);
pthread_t pthread_self(void);
/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	pthread_t thread_id;
	char *stack;
	enum thread_status t_status;
	jmp_buf env;
	struct thread_control_block *next_thread;
	struct thread_control_block *prev_thread;
};

static struct thread_control_block *head;
static struct thread_control_block *tail;
static struct thread_control_block *curr;

volatile static pthread_t thread_id_counter = 0;
volatile static int num_curr_threads = 0;

static void schedule(int signal)
{
	// printf("calling thead: %ld\n", pthread_self());
	// if(num_curr_threads < 2){
	// 	return;
	// }

	//save the current thread's environment
	if(!setjmp(curr->env)){
		//first time 
		if(curr->t_status == TS_RUNNING){
			curr->t_status = TS_READY;
		}
		//now we can ignore the previous thread and find the next ready thread
		for(int i = 0; i < MAX_THREADS + 1; i++){
			curr = curr->next_thread;
			if(curr == NULL){
				break;
			}
			if(curr->t_status == TS_READY){
				//run the next ready thread
				curr->t_status = TS_RUNNING;
				longjmp(curr->env,1);
				break;
			}
		}
	}else{
		//thread called longjmp and is now running
		curr->t_status = TS_RUNNING;
	}
	return;
}

static int scheduler_init()
{
	int return_code = 0;
	//set global thread count and thread ID counter
	thread_id_counter = 0;
	num_curr_threads = 0;

	//initialize main thread
	curr = (struct thread_control_block *)malloc(sizeof(struct thread_control_block));
	if(curr == NULL){
		return_code = -1;
		return return_code;
	}
	setjmp(curr->env);
	curr->stack = NULL;
	curr->next_thread = NULL;
	curr->prev_thread = NULL;
	curr->thread_id = thread_id_counter++;
	curr->t_status = TS_RUNNING;
	tail = curr;
	head = curr;
	num_curr_threads++;
	//set up signal handler 
	struct sigaction act;
	if(sigemptyset(&act.sa_mask) == -1){
		return_code = -1;
		return return_code;
	};
	act.sa_flags = SA_NODEFER;
	act.sa_handler = schedule;
	if(sigaction(SIGALRM, &act, NULL) == -1){
		return_code = -1;
		return return_code;
	}

	//set up alarm
	ualarm(SCHEDULER_INTERVAL_USECS,SCHEDULER_INTERVAL_USECS);
	return return_code;


}

int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	// Create the timer and handler for the scheduler. Create thread 0.
	static int is_first_call = true;

	bool return_code = 0;
	
	if (is_first_call)
	{
		is_first_call = false;
		return_code = scheduler_init();
	}
	/*-----create TCB for new thread-----*/
	struct thread_control_block *new_tcb = (struct thread_control_block *)malloc(sizeof(struct thread_control_block));
	if(new_tcb == NULL){
		return_code = -1;
		return return_code;
	}
	//allocate stack
	new_tcb->stack = (char*)malloc(THREAD_STACK_SIZE);
	if(new_tcb->stack == NULL){
		return_code = -1;
		return return_code;
	}
	unsigned long int exit_addr = (unsigned long int)pthread_exit;
	//push pthread_exit on top of the stack
	memcpy(new_tcb->stack + THREAD_STACK_SIZE - ADDRESS_SIZE, &exit_addr, ADDRESS_SIZE);

	//set the env
	setjmp(new_tcb->env);
	
	//set the stack pointer of the env to be top of the stack
	new_tcb->env->__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)(new_tcb->stack + THREAD_STACK_SIZE - ADDRESS_SIZE));

	//move arg to R13 bc jmp_buf doesn't have RDI
	new_tcb->env->__jmpbuf[JB_R13] = (unsigned long int)(unsigned long int *)arg;

	//move start routine to R12 in prep for start_thunk
	new_tcb->env->__jmpbuf[JB_R12] = (unsigned long int)(unsigned long int *)start_routine;

	//change PC to start_routine(arg)
	new_tcb->env->__jmpbuf[JB_PC]  = ptr_mangle((unsigned long int)start_thunk);

	/*-----handling logistics with the new TCB-----*/
	//link the new TCB in the circular linked list

	if(num_curr_threads == 1){
		new_tcb->next_thread = head;
		head->prev_thread = new_tcb;
		head->next_thread = new_tcb;
		new_tcb->prev_thread = head;
	}
	else{
		head = curr->prev_thread;
		head->next_thread = new_tcb;
		new_tcb->prev_thread = head;
		new_tcb->next_thread = curr;
		curr->prev_thread = new_tcb;
	}
	//ready
	new_tcb->t_status = TS_READY;
	//increment the current number of running threads
	num_curr_threads++;
	//set thread id to the next thread id counter
	new_tcb->thread_id = thread_id_counter++;
	*thread = new_tcb->thread_id;
	return return_code;
}

void pthread_exit(void *value_ptr)
{
	curr->t_status = TS_EXITED;
	num_curr_threads--;
	// printf("\nnumber of threads: %d, thread ID: 0x%lx\n", num_curr_threads, thread_id_counter);
	if(num_curr_threads > 0){
		schedule(SIGALRM);
	}
	exit(0);
}
pthread_t pthread_self(void)
{
	return curr->thread_id;
}
