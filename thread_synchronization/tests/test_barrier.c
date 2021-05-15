#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#define COUNTER_FACTOR 0xFFFFFF
#define NUM_THREADS 5

struct thread_data{
  int thread_id;
  int sum;
  char *message;
};

/* Barrier variable */
pthread_barrier_t barrier1;
pthread_barrier_t barrier2;

void wasteTime(int a){
	for(long int i = 0; i < a*COUNTER_FACTOR; i++);
}

/********************/
void *PrintHello(void *threadarg)
{
  long taskid, i;
  struct thread_data *my_data;
  int rc;

  my_data = (struct thread_data *) threadarg;
  taskid = my_data->thread_id;

  /* print some thread-specific stuff, then wait for everybody else */
  for(i = 0;i<3; i++){
    wasteTime(taskid<<i);
    printf("Thread %ld printing before barrier 0 %ld of 3\n", taskid+1, i+1);
  }
  printf("Thread %ld entering barrier 0\n", taskid+1);

  /* wait at barrier until all NUM_THREAD threads arrive */
  rc = pthread_barrier_wait(&barrier1);
  if (rc != 0 && rc != -1) {
    printf("Could not wait on barrier 0(return code %d)\n", rc);
    exit(-1);
  }

  /* now print some more thread-specific stuff */
  for (i = 0; i < 2; i++) {
    printf("Thread %ld after barrier 0(print %ld of 2)\n", taskid+1, i+1);
  }

  pthread_exit(NULL);
}

/********************/
void *PrintHello1(void *threadarg)
{
  long taskid, i;
  struct thread_data *my_data;
  int rc;

  my_data = (struct thread_data *) threadarg;
  taskid = my_data->thread_id;

  /* print some thread-specific stuff, then wait for everybody else */
  for(i = 0;i<3; i++){
    wasteTime(taskid<<i);
    printf("Thread %ld printing before barrier 1 %ld of 3\n", taskid+1, i+1);
  }
  printf("Thread %ld entering barrier 1\n", taskid+1);

  /* wait at barrier until all NUM_THREAD threads arrive */
  rc = pthread_barrier_wait(&barrier2);
  if (rc != 0 && rc != -1) {
    printf("Could not wait on barrier 1 (return code %d)\n", rc);
    exit(-1);
  }

  /* now print some more thread-specific stuff */
  for (i = 0; i < 2; i++) {
    printf("Thread %ld after barrier 1 (print %ld of 2)\n", taskid+1, i+1);
  }

  pthread_exit(NULL);
}

/*************************************************************************/
int main(int argc, char *argv[])
{
  pthread_t threads[2*NUM_THREADS];
  struct thread_data thread_data_array[2*NUM_THREADS];
  int rc;
  long t;
  char *Messages[NUM_THREADS] = {"First Message",
				 "Second Message",
				 "Third Message",
				 "Fourth Message",
				 "Fifth Message"};

  printf("Hello test_barrier.c\n");

  /* Barrier initialization -- spawned threads will wait until all arrive */
  if (pthread_barrier_init(&barrier1, NULL, NUM_THREADS)) {
    printf("Could not create a barrier\n");
    return -1;
  } 
   if (pthread_barrier_init(&barrier2, NULL, NUM_THREADS)) {
    printf("Could not create a barrier\n");
    return -1;
  } 
  /* create threads */
  for (t = 0; t < NUM_THREADS; t++) {
    thread_data_array[t].thread_id = t;
    thread_data_array[t].sum = t+28;
    thread_data_array[t].message = Messages[t];
    printf("In main:  creating thread %ld\n", t);
    rc = pthread_create(&threads[t], NULL, PrintHello,
                                             (void*) &thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

   /* create threads */
  for (t = 0; t < NUM_THREADS; t++) {
    thread_data_array[NUM_THREADS+t].thread_id = NUM_THREADS+t;
    thread_data_array[NUM_THREADS+t].sum = t+28;
    thread_data_array[NUM_THREADS+t].message = Messages[t];
    printf("In main:  creating thread %ld\n", NUM_THREADS + t);
    rc = pthread_create(&threads[NUM_THREADS+t], NULL, PrintHello1,
                                             (void*) &thread_data_array[NUM_THREADS+t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }


  printf("After creating the threads; my id is %lx\n", pthread_self());

  int sum = 0;
  for(int i = 0; i<1000000000;i++){
    sum++;
  }
  printf("After joining\n");

  return(0);

} /* end main */
