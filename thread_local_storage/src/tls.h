#ifndef TLS_H_
#define TLS_H_
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_THREAD_COUNT 128

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, const char *buffer);
int tls_clone(pthread_t tid);

///// data structures /////

typedef struct thread_local_storage
{
    pthread_t tid;
    unsigned int size;
    unsigned int num_pages;
    struct page **pages;
} TLS;

struct page
{
    unsigned long address;
    int ref_count;
};

typedef struct tid_tls_pair
{
    pthread_t tid;
    TLS *tls;
} tid_tls;

///// Global Variables /////
tid_tls *tid_tls_arr[MAX_THREAD_COUNT];
int page_size;
pthread_mutex_t mutex_read;
pthread_mutex_t mutex_write;
pthread_mutex_t mutex_copy;
#endif /* TLS_H_ */
