#include <pthread.h>
#include <stdlib.h>
#include<stdio.h>
#include<unistd.h>		
#include <assert.h>

#define THREAD_CNT 6
#define COUNTER_FACTOR 0xFFFFFF

pthread_barrier_t barrier_G, barrier_R;
pthread_t threads[THREAD_CNT];
int criticalData;
int teamR;
int teamG;
#define INITIAL_DATA 0

int sum_G, sum_R;

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
    int t = pthread_self()%2 == 0;
    if(t){
        sum_G += pthread_barrier_wait(&barrier_G);
    }
    else{
        sum_R += pthread_barrier_wait(&barrier_R);
    }
	criticalData++;
    teamG = t ? teamG + 1 : teamG;
    teamR = !t ? teamR + 1 : teamR;
	printf("thread %ld has passed barrier, criticalData is now %i\n",pthread_self(),criticalData);
    printf("thread %ld has passed barrier, teamG is now %i teamR is now %i\n",pthread_self(),teamG, teamR);
	return NULL;
 }
 
 void* notBarrier(void *arg){		
	wasteTime(5);
	printf("thread %ld finish -- not part of barrier\n",pthread_self());
	return NULL;
 }

int main(int argc, char **argv) {
	sum_G = 0;
    sum_R = 0;
	criticalData = INITIAL_DATA;
    teamG = INITIAL_DATA;
    teamR = INITIAL_DATA;
	pthread_barrier_init(&barrier_G,NULL,THREAD_CNT/2);
    pthread_barrier_init(&barrier_R,NULL,THREAD_CNT/2);
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
	}
	
	wasteTime(20);	//makes sure checks arent done while threads running
	assert(teamR == 3);
    assert(teamG == 3);
	if((sum_G + sum_R) != -2){
		printf("Error, not exactly one PTHREAD_BARRIER_SERIAL_THREAD %d\n", sum_G + sum_R);
	}
	

	printf("\nFIRST SET OF THREADS DONE\n\n");
	//The second set of threads is to test that the barrier resets once it lets the threads through
	sum_R = 0;
    sum_G = 0;
	criticalData = INITIAL_DATA;
    teamG = INITIAL_DATA;
    teamR = INITIAL_DATA;
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
	}
	
	wasteTime(20);
	assert(teamR == 3);
    assert(teamG == 3);
	printf("\nSECOND SET OF THREADS DONE\n\n");

	//Delete then re-initialize threads
	sum_R = 0;
    sum_G = 0;
	criticalData = INITIAL_DATA;
    teamG = INITIAL_DATA;
    teamR = INITIAL_DATA;
	pthread_barrier_destroy(&barrier_G);
    pthread_barrier_destroy(&barrier_R);
	pthread_barrier_init(&barrier_G,NULL,THREAD_CNT/2);
    pthread_barrier_init(&barrier_R,NULL,THREAD_CNT/2);
	for (int i = 0; i < THREAD_CNT; i++) {
		pthread_create(&threads[i], NULL, &barrierTest,(void *)(intptr_t)i);
	}
	
	wasteTime(20);
    assert(teamR == 3);
    assert(teamG == 3);
	if((sum_G + sum_R) != -2){
		printf("Error, not exactly one PTHREAD_BARRIER_SERIAL_THREAD %d\n", sum_G + sum_R);
	}
	return 0;
}