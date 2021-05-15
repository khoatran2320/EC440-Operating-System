#include <pthread.h>
#include <stdlib.h>
#include<stdio.h>
#include<unistd.h>		

#define THREAD_CNT 3
#define COUNTER_FACTOR 0xFFFFFF

pthread_barrier_t barrier;
pthread_t threads[THREAD_CNT];
int criticalData;
#define INITIAL_DATA 0

int sum;

void wasteTime(int a){
	for(long int i = 0; i < a*COUNTER_FACTOR; i++);
}

 void* barrierTest(void *arg) {
	 printf("thread %ld start -- before barrier\n",pthread_self());
	 if(criticalData != INITIAL_DATA){
		 printf("Error, barrier has modified the criticalData before it should\n");
	 }
	wasteTime(10);
	if(criticalData != INITIAL_DATA){
		 printf("Error, barrier has modified the criticalData before it should\n");
	 }
	sum += pthread_barrier_wait(&barrier);
	criticalData++;
	printf("thread %ld has passed barrier, criticalData is now %i\n",pthread_self(),criticalData);

	return NULL;
 }
 
 void* notBarrier(void *arg){		
	wasteTime(5);
	printf("thread %ld finish -- not part of barrier\n",pthread_self());
	return NULL;
 }

int main(int argc, char **argv) {
	sum = 0;
	criticalData = INITIAL_DATA;
	pthread_barrier_init(&barrier,NULL,THREAD_CNT);
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
		pthread_create(&threads[i], NULL, &notBarrier, (void *)(intptr_t)i);
	}
	
	wasteTime(20);	//makes sure checks arent done while threads running
	
	if(sum != PTHREAD_BARRIER_SERIAL_THREAD){
		printf("Error, not exactly one PTHREAD_BARRIER_SERIAL_THREAD %d\n", sum);
	}
	
	printf("\nFIRST SET OF THREADS DONE\n\n");
	//The second set of threads is to test that the barrier resets once it lets the threads through
	sum = 0;
	criticalData = INITIAL_DATA;
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
	}
	
	wasteTime(20);
	
	printf("\nSECOND SET OF THREADS DONE\n\n");

	//Delete then re-initialize threads
	sum = 0;
	criticalData = INITIAL_DATA;
	pthread_barrier_destroy(&barrier);
	pthread_barrier_init(&barrier,NULL,THREAD_CNT);
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
	}
	
	wasteTime(20);
	if(sum != PTHREAD_BARRIER_SERIAL_THREAD){
		printf("Error, not exactly one PTHREAD_BARRIER_SERIAL_THREAD %d\n", sum);
	}
	return 0;
}