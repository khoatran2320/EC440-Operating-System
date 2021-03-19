#include "ec440threads.h"
#include "threads.h"
#include <assert.h>

void schedule(int signal)
{
	// printf("Thread %ld calling schedule \n", curr->thread_id);
	lock();
	//save the current thread's environment
	if(!setjmp(curr->env)){
		//first time 
		switch (curr->t_status)
		{
			case TS_RUNNING:
				curr->t_status = TS_READY;
				for(int i = 0; i < MAX_THREADS + 1; i++){
					curr = curr->next_thread;
					if(curr == NULL){
						break;
					}
					if(curr->t_status == TS_READY){
						//run the next ready thread
						curr->t_status = TS_RUNNING;
						longjmp(curr->env,1);
					}
				}	
				break;
			case TS_EXITED:
				free(curr->stack);
				struct thread_control_block *prev = curr->prev_thread;
				struct thread_control_block *next = curr->next_thread;
				free(curr);
				prev->next_thread = next;
				next->prev_thread = prev;
				curr = prev;
			case TS_BLOCKED:
				for(int i = 0; i < MAX_THREADS + 1; i++){
					curr = curr->next_thread;
					if(curr == NULL){
						break;
					}
					if(curr->t_status == TS_READY){
						// printf("Next ready thread is %ld\n", curr->thread_id);
						//run the next ready thread
						curr->t_status = TS_RUNNING;
						longjmp(curr->env,1);
					}
				}	
				break;
			default:
				break;
		}
	}else{
		//thread called longjmp and is now running
		curr->t_status = TS_RUNNING;
	}
	// printf("Scheduled thread %ld\n", curr->thread_id);
	unlock();
	return;
}

int scheduler_init()
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
		new_tcb->next_thread = curr;
		curr->prev_thread = new_tcb;
		curr->next_thread = new_tcb;
		new_tcb->prev_thread = curr;
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

	// printf("Created thread %ld with return code %d\n", *thread, return_code);
	return return_code;
}

void pthread_exit(void *value_ptr)
{
	lock();
	curr->t_status = TS_EXITED;
	// printf("Thread %ld exiting\n", curr->thread_id);
	num_curr_threads--;
	// printf("\nnumber of threads: %d\n", num_curr_threads);
	if(num_curr_threads > 0){
		unlock();
		schedule(SIGALRM);
	}
	unlock();
	exit(0);
}
pthread_t pthread_self(void)
{
	return curr->thread_id;
}

void lock(){
	sigemptyset (&sig_mask);
	sigaddset(&sig_mask, SIGALRM);

	if(sigprocmask(SIG_BLOCK, &sig_mask, NULL) < 0){
		perror("Failed to mask signal");
        exit(1);
	}
	return;
}

void unlock(){
	if(sigprocmask(SIG_UNBLOCK, &sig_mask, NULL) < 0){
		perror("Failed unmask signal");
        exit(1);
	}
	return;
}

struct mutex_info * get_mutex(pthread_mutex_t *mutex){
	struct mutex_info *m;
	unsigned long p;
	if(memcpy(&p, &mutex->__size, sizeof(m)) == NULL){
		return NULL;
	};
	m = (struct mutex_info *)p;
	return m;
}

struct barrier_info * get_barrier(pthread_barrier_t *barrier){
	struct barrier_info *b;
	unsigned long p;
	if(memcpy(&p, &barrier->__size, sizeof(b)) == NULL){
		return NULL;
	};
	b = (struct barrier_info *)p;
	return b;
}


int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
	lock();
	struct mutex_info *m = (struct mutex_info *)malloc(sizeof(struct mutex_info));
	if(!m){
		unlock();
		return 0;
	}
	m->init = true;
	m->locked = false;
	m->mutex_id = mutex_counter++;
	m->waiters = NULL;
	// printf("Initializing mutex %d\n", m->mutex_id);

	unsigned long p = (unsigned long)m;
	memcpy(&mutex->__size, &p, sizeof(m));
	unlock();
	return 0;
}


int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	lock();
	struct mutex_info *m = get_mutex(mutex);
	if(!m){
		unlock();
		return -1;
	}
	// printf("Destroying mutex %d\n", m->mutex_id);
	while(m->waiters){
		struct queue * temp = m->waiters;
		m->waiters = m->waiters->next;
		free(temp);
	}
	free(m);
	unlock();
	return 0;
}


int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	lock();
	struct mutex_info *m = get_mutex(mutex);
	if(!m){
		unlock();
		return -1;
	}
	if(m->init){
		if(m->locked){
			curr->t_status = TS_BLOCKED;
			// printf("Calling thread %ld is blocked\n", curr->thread_id + 1);

			struct queue *q = (struct queue *)malloc(sizeof(struct queue));
			q->tcb = curr;
			q->next = NULL;

			if(m->waiters == NULL){
				m->waiters = q;
			}else{
				q->next = m->waiters;
				m->waiters = q;
			}
			unlock();
			schedule(SIGALRM);
		}
		else{
			m->locked = true;
			// printf("Locking lock %d\n", m->mutex_id);
		}
	}
	unlock();
	return 0;
}


int pthread_mutex_unlock(pthread_mutex_t *mutex){
	lock();
	struct mutex_info *m = get_mutex(mutex);
	if(!m){
		unlock();
		return -1;
	}
	if(m->init){
		if(m->locked){
			m->locked = false;
			// printf("Thread %ld unlocking mutex %d\n", curr->thread_id+1, m->mutex_id);
			if(m->waiters){
				m->locked = true;
				struct queue *temp = m->waiters;
				temp->tcb->t_status = TS_READY;
				// printf("Waking up thread %ld\n",temp->tcb->thread_id + 1);
				m->waiters = m->waiters->next;
				free(temp);
			}
		}
		else{
			unlock();
			return -1;
		}
	}
	unlock();
	return 0;
}


int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned count){
	if(count == 0){
		return EINVAL;
	}
	lock();
	struct barrier_info *b = (struct barrier_info *)malloc(sizeof(struct barrier_info));
	if(!b){
		unlock();
		return -1;
	}
	b->init = true;
	b->count = count;
	b->barrier_id = barrier_counter++;
	b->num_threads_entered = 0;
	b->waiters = NULL;
	// printf("Initializing barrier %d\n", b->barrier_id);

	unsigned long p = (unsigned long)b;
	if(memcpy(&barrier->__size, &p, sizeof(b)) == NULL){
		unlock();
		return -1;
	}
	unlock();
	return 0;
}
int pthread_barrier_wait(pthread_barrier_t *barrier){
	lock();
	struct barrier_info *b = get_barrier(barrier);
	if(!b){
		unlock();
		return 0;
	}
	if(b->init){
		++b->num_threads_entered;
		if(b->num_threads_entered < b->count){
			curr->t_status = TS_BLOCKED;
			// printf("Calling thread %ld is blocked\n", curr->thread_id);

			struct queue *q = (struct queue *)malloc(sizeof(struct queue));
			q->tcb = curr;
			q->next = NULL;

			if(b->waiters == NULL){
				b->waiters = q;
			}else{
				q->next = b->waiters;
				b->waiters = q;
			}
			unlock();
			schedule(SIGALRM);
		}
		else if (b->num_threads_entered == b->count){
			// printf("All threads in barrier\n");
			while(b->waiters != NULL){
				b->waiters->tcb->t_status = TS_READY;
				struct queue *temp = b->waiters;
				// printf("Waking up thread %ld\n",temp->tcb->thread_id );
				b->waiters = b->waiters->next;
				free(temp);
				temp = NULL;
			}
			b->num_threads_entered = 0;
			b->waiters = NULL;
			unlock();
			return PTHREAD_BARRIER_SERIAL_THREAD;
		}
	}
	unlock();
	return 0;
}
int pthread_barrier_destroy(pthread_barrier_t *barrier){
	lock();
	struct barrier_info *b = get_barrier(barrier);
	if(!b){
		unlock();
		return -1;
	}
	b->init = false;
	// printf("Destroying barrier %d\n", b->barrier_id);
	// while(b->waiters){
	// 	struct queue * temp = b->waiters;
	// 	b->waiters = b->waiters->next;
	// 	free(temp);
	// }
	// free(b);
	unlock();
	return 0;
}