#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include "threads.c"
#include <stdio.h>
#define NUM_THREADS 5


void *work(void *i)
{
  int * sum = (int*)i;
  *sum = 0;
  printf("Hello World! from child thread %lx\n", (long)pthread_self());

  for(int i = 0; i<10000000;i++){
    (*sum)++;
    if((*sum) % 100000 ==0){
      printf("thread %ld counted %d of 10000000\n",pthread_self(), *sum);
    }
  }
  pthread_exit(NULL);
}

/****************************************************************/
int main(int argc, char *argv[])
{
  int arg,i,j,k,m;                  /* Local variables. */
  pthread_t *id = (pthread_t*)malloc(NUM_THREADS*sizeof(pthread_t));
  int arr[NUM_THREADS];
  int sum = 0;
  printf("Hello test\n");

  for (i = 0; i < NUM_THREADS; ++i) {
    if (pthread_create(id+i, NULL, work, &arr[i])) {
      printf("ERROR creating the thread\n");
      exit(19);
    }
  }
  for(int i = 0; i<40000000;i++){
    (sum)++;
    if((sum) % 100000 ==0){
      printf("thread %ld counted %d of 40000000\n",pthread_self(), sum);
    }
  }
  for(i = 0; i< NUM_THREADS; i++){
    printf("%d\n",arr[i]);
  }
  printf("main() after creating the thread.  My id is %lx\n",
                                              (long) pthread_self());
    free(id);
  return(0);
}