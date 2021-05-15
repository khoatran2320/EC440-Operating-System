#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#define NUM_THREADS 8

struct thread_data{
  int thread_id;
  int sum;
  char *message;
  int *arr;
};

int ind = 0;

pthread_mutex_t mutexA[NUM_THREADS+1];

/********************/
void *PrintHello(void *threadarg)
{
  long taskid, sum;
  struct thread_data *my_data;
  char *message;
  int * arr;

  my_data = (struct thread_data *) threadarg;
  taskid = my_data->thread_id;
  sum = my_data->sum;
  message = my_data->message;
  arr = my_data->arr;

  /* printf("PrintHello thread #%ld, sum = %ld, message = %s\n",
                                       taskid, sum, message); */

  switch (taskid) {
  case 2:
    printf("thread #%ld waiting for 1 ...\n", taskid);
    pthread_mutex_lock(&mutexA[1]);
    pthread_mutex_unlock(&mutexA[1]);
    break;
  case 3:
    printf("thread #%ld waiting for 1 ...\n", taskid);
    pthread_mutex_lock(&mutexA[1]);   
    pthread_mutex_unlock(&mutexA[1]);
    break;
  case 4:
    printf("thread #%ld waiting for 2 ...\n", taskid);
    pthread_mutex_lock(&mutexA[2]);   
    pthread_mutex_unlock(&mutexA[2]);
    break;
  case 5:
    printf("thread #%ld waiting for 4 ...\n", taskid);
    pthread_mutex_lock(&mutexA[4]);
    pthread_mutex_unlock(&mutexA[4]);
    break;
  case 6:
    printf("thread #%ld waiting for 3 and 4 ...\n", taskid);
    pthread_mutex_lock(&mutexA[3]);
    pthread_mutex_unlock(&mutexA[3]);
    pthread_mutex_lock(&mutexA[4]);
    pthread_mutex_unlock(&mutexA[4]);
    break;
  case 7:
    printf("thread #%ld waiting for 5 and 6 ...\n", taskid);
    pthread_mutex_lock(&mutexA[5]);
    pthread_mutex_unlock(&mutexA[5]);
    pthread_mutex_lock(&mutexA[6]);
    pthread_mutex_unlock(&mutexA[6]);
    break;
  case 8:
    printf("thread #%ld waiting for 6 and 7 ...\n", taskid);
    pthread_mutex_lock(&mutexA[6]);
    pthread_mutex_unlock(&mutexA[6]);
    pthread_mutex_lock(&mutexA[7]);
    pthread_mutex_unlock(&mutexA[7]);
    break;
  default:
    printf("It's me, thread #%ld! I'm waiting ...\n", taskid);
    break;
  }
  printf("It's me, thread #%ld! I'm unlocking %ld...\n", taskid, taskid);

  my_data->arr[ind++] = taskid;
  if (pthread_mutex_unlock(&mutexA[taskid])) {
    printf("ERROR on unlock\n");
  }
  pthread_exit(NULL);
}

/*************************************************************************/
int main(int argc, char *argv[])
{
  pthread_t threads[NUM_THREADS+1];
  struct thread_data thread_data_array[NUM_THREADS+1];
  int arr[NUM_THREADS+1] = {0};
  int i, rc;
  long t;
  char *Messages[NUM_THREADS+1] = {"Zeroth Message",
                                 "First Message",
                                 "Second Message",
                                 "Third Message",
                                 "Fourth Message",
                                 "Fifth Message",
                                 "Sixth Message",
                                 "Seventh Message"};
  char dummy[1];

  for (i = 0; i <= NUM_THREADS; i++) {
    if (pthread_mutex_init(&mutexA[i], NULL)) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  for (i = 0; i <= NUM_THREADS; i++) {
    if (pthread_mutex_lock(&mutexA[i])) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }


  for (t = 2; t <= NUM_THREADS; t++) {
    thread_data_array[t].thread_id = t;
    thread_data_array[t].sum = t+28;
    thread_data_array[t].message = Messages[t];
    thread_data_array[t].arr = arr;
    /*  printf("In main:  creating thread %ld\n", t); */
    rc = pthread_create(&threads[t], NULL, PrintHello,
                                    (void*) &thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  sleep(1);  
  if (pthread_mutex_unlock(&mutexA[1])) {
    printf("ERROR on unlock\n");
  }
    
//   pthread_mutex_lock(&mutexA[7]);

  int sum = 0;
  for(int i = 0; i<1000000000;i++){
    sum++;
  }

  assert(arr[0] == 2 || arr[0] == 3);
  assert(arr[1] == 2 || arr[1] == 3);
  assert(arr[2] == 4);
  assert(arr[3] == 5 || arr[3] == 6);
  assert(arr[4] == 5 || arr[4] == 6);
  assert(arr[5] == 7);
  assert(arr[6] == 8);

  
  return(0);
} /* end main */