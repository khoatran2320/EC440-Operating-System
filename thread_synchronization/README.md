# User-Space Threading Library
This is a user-space threading library that recreated the primary thread functions from pthreads: 
- pthread_create 
- pthread_exit
- pthread_self
- pthread_mutex_init
- pthread_mutex_lock
- pthread_mutex_unlock
- pthread_mutex_destroy
- pthread_barrier_init
- pthread_barrier_wait
- pthread_barrier_destroy

## Scheduler and Internal Mechanism
The library implements a doubly, circular linked list to keep track of all the TCBs. It uses a 50ms preemptive round-robin scheduler to schedule all the threads. When pthread_create is first called, the state of the main thread is saved by a setjmp procedure. During this step, the alarm is set for the scheduler. In subsequent pthread_create calls, the procedure will set up the TCB for the new thread. This includes setting up the stack, manipulate the environment to call start_routine immediately, and set the return address so that the thread will call pthread_exit after returning from start_routine. 

## pthread_create
Initializes a new TCB and sets up the environment for the new thread. It directly manipulates the setjmp environment to change the stack pointer to a manually allocated stack, changing the PC to point to start_routine, and pushing the pthread_exit procedure onto the stack so it may be called after the return from start_routine. The newly created TCB is then inserted behind the current thread in the circular linked list to be scheduled. 

## pthread_exit
marks the calling thread as exited so it may be removed. The scheduler is then called to remove the thread and schedule a new thread.

## pthread_self
returns the current thread's ID

## pthread_mutex_init
initializes the mutex by allocating memory for a custom mutex structure that has information for if the mutex has been initialized, whether it's locked, and the threads that are blocked by the mutex. The address of this custom structure is copied to the pthread_mutex_t structure. 

## pthread_mutex_lock
locks the mutex if it is unlocked, else block the calling thread, add it to the waiting queue in the mutex, and schedule another thread.

## pthread_mutex_unlock
unlocks the mutex and wake up another thread in the mutex waiting queue if there are any. 

## pthread_mutex_destroy
frees all memory associated with the custom mutex structure

## pthread_barrier_init
initializes the barrier by allocation memory for a custom barrier structure that has information for if the barrier is initialized, count for the required number of threads to enter the barrier, the current number of threads in the barrier, and a queue of all blocked threads in the barrier. 

## pthread_barrier_wait
If the addition of calling thread does not meet the required number of threads needed to release the threads in the barrier, the thread will become blocked and will be added to the waiting queue. In this case, the thread count of the barrier will increment and a thread thread will be scheduled. If the calling thread satisfies the thread count requirement of the barrier, all other threads in the barrier will be waken up and the barrier is reset. 

## pthread_barrier_destroy
frees all memory associated with the custom mutex structure


## Credits
- stackoverflow for bugs
- Man page for references 
- Lecture notes for implementation and theory