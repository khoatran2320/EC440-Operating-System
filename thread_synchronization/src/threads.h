#ifndef THREADS_H
#define THREADS_H

#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)
#define ADDRESS_SIZE 8

#define MUTEX_NO_OWNER 22222

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
	TS_READY, 
	TS_BLOCKED
};
sigset_t sig_mask;


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

struct queue{
	struct thread_control_block *tcb;
	struct queue *next;
};
struct mutex_info{
	bool init;
	bool locked;
	int mutex_id;
	struct queue * waiters;
};

struct barrier_info{
	bool init;
	int num_threads_entered;
	int count;
	int barrier_id;
	struct queue * waiters;
};

struct thread_control_block *head;
struct thread_control_block *tail;
struct thread_control_block *curr;

static volatile pthread_t thread_id_counter = 0;
static volatile int num_curr_threads = 0;
static volatile int mutex_counter = 0;
static volatile int barrier_counter = 0;

void schedule(int signal);
void pthread_exit(void *value_ptr);
int scheduler_init();
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_barrier_init(pthread_barrier_t *restrict barrier, const pthread_barrierattr_t *restrict attr, unsigned count);
int pthread_barrier_wait(pthread_barrier_t *barrier);
int pthread_barrier_destroy(pthread_barrier_t *barrier);
int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg);
pthread_t pthread_self(void);
struct mutex_info * get_mutex(pthread_mutex_t *mutex);
void lock();
void unlock();


#endif