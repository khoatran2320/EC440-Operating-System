# Thread-Local Storage Library 
#### By Khoa Tran 
## EC440: Introduction to Operating Systems Spring 2021

## Introduction
This library provides protected memory regions for that is local to each thread. Threads can share memory, but one thread can write to the memory area without affecting other threads through the copy-on-write mechanism. This library works on top of the pthread library. 
## Supported Functions
-   ___int tls_create(unsigned int `size`)___: creates a thread-local storage area of size `size`. Returns 0 on success and -1 on failure. 
-   ___int tls_destroy()___: frees the current thread's local storage area. Return 0 on success and -1 on failure. 
-   ___int tls_read(unsigned int `offset`, unsigned int `length`, char *`buffer`)___: reads from current thread's local storage area `length` bytes starting at `offset` into `buffer`. Returns 0 on success and -1 on failure. 
-   ___int tls_write(unsigned int `offset`, unsigned int `length`, const char *`buffer`)___: writes `length` bytes to current thread's local storage area starting at `offset` with data from `buffer`. Returns 0 on success and -1 on failure. 
-   ___int tls_clone(pthread_t `tid`)___: creates a thread-local storage area by cloning thread `tid`'s local storage area. Returns 0 on success and -1 on failure. 

## Credits

-   Linux man pages for general thread-related functions references. 
-   Lecture notes (especially the homework discussion lecture).
-   Class Piazza posts. 

