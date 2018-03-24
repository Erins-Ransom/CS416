// name: Eric Knorr, Daniel Parks, Eric Fiore
// netID: erk58, dap291, ejf96

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define USE_MY_PTHREAD 1;

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif


#define STACK_ALLOCATION 0
#define PRIVATE_REQ 1
#define PUBLIC_REQ 2
#define SHARED_REQ 3
#define malloc(x) myallocate(x, __FILE__, __LINE__, PUBLIC_REQ, -1);
#define free(x) mydeallocate(x, __FILE__, __LINE__, PUBLIC_REQ, -1);
#define shalloc(x) myallocate(x, __FILE__, __LINE__, SHARED_REQ, -1);


// ____________________ Struct Defs ________________________

enum thread_status {active, yield, wait_thread, wait_mutex, thread_exit};

typedef struct my_pthread {
        short id;                  	// integer identifier of thread
        int priority;                   // current priority level of this thread
        int intervals_run;              // the number of concecutive intervals this thread has run
        int wait_id;			// TID of thread being waited on
	enum thread_status status;      // the threads current status
	void* ret;                      // return value of the thread
        ucontext_t uc;                  // execution context of given thread
	int num_pages;			// number of pages owned by thread
} my_pthread_t;

typedef struct tid_node {
        short tid;
        struct tid_node* next;
} tid_node_t;

typedef struct Node {
        my_pthread_t* thread;
        struct Node* next;
} Node;

typedef struct Queue {
        Node* back;
        int size;
} Queue;

typedef struct my_pthread_mutex {
        Queue * waiting;                // queue of threads waiting on this mutex
        my_pthread_t * user;            // reference to the thread that currently has the mutex, NULL if not claimed
} my_pthread_mutex_t;

// ______________________ API _________________________

// Pthread Note: Your internal implementation of pthreads should have a running and waiting queue.
// Pthreads that are waiting for a mutex should be moved to the waiting queue. Threads that can be
// scheduled to run should be in the running queue.

// Creates a pthread that executes function. Attributes are ignored, arg is not.
int my_pthread_create( my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

// Explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out and
// another can be scheduled if one is waiting.
void my_pthread_yield();

//Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL,
//any return value from the thread will be saved.
void pthread_exit(void *value_ptr);

// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.
int my_pthread_join(my_pthread_t thread, void **value_ptr);

// Mutex note: Both the unlock and lock functions should be very fast. If there are any threads that are meant to compete for these functions, my_pthread_yield should be called immediately after running the function in question. Relying on the internal timing will make the function run slower than using yield.

// Initializes a my_pthread_mutex_t created by the calling thread. Attributes are ignored.
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

// Locks a given mutex, other threads attempting to access this mutex will not run until it is unlocked.
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

// Unlocks a given mutex.
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

// Destroys a given mutex. Mutex should be unlocked before doing so.
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

// Swaps pages on page fault
void vmem_sig_handler(int signo, siginfo_t *info, void *context);

// Initialized memory for paging
void page_malloc_init();


void * myallocate(size_t size, char * file, int line, int flag, short TID);


void mydeallocate(void * index, char * file, int line, int flag, short TID);

#endif
