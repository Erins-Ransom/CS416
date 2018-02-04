#include "my_pthread_t.h"




// ____________________ Struct Defs ________________________

typedef struct Node {
	ucontext_t * context;
	struct Node * next;
	struct Node * prev;
} Node;

typedef struct Queue {
	Node * top;
	Node * bottom;
	int size;
} Queue;




// _________________ Utility Functions _____________________

// Function to initialize a Queue
Queue * make_queue() {
	Queue * new = malloc(sizeof(Queue));
	new->top = NULL;
	new->bottom = NULL;
	new->size = 0;
	return new;
}


// Function to get the next context waiting in the Queue
ucontext_t * get_next(Queue * Q) {
	ucontext_t * ret = NULL;
	Node * temp = Q->top;
	if (Q->top) {
		ret = Q->top->context;
		Q->top = Q->top->prev;
		free(temp);
		Q->size--;
	}
	if (Q->size == 0)
		Q->bottom = NULL;
	return ret;

}


// function to add a context to the Queue
void enqueue(ucontext_t * context, Queue * Q) {
	Node * new = malloc(sizeof(Node));
	new->context = context;
	new->prev = NULL;
	if (Q->bottom)
		Q->bottom->prev = new;
	new->next = Q->bottom;
	if (!Q->top)
		Q->top = new;
	Q->bottom = new;
	Q->size++;
}


//Signal handler to activate the scheduler on periodic SIGALRM
void scheduler_alarm_handler(int signum) {

}




// __________________ API ____________________

// Pthread Note: Your internal implementation of pthreads should have a running and waiting queue.
// Pthreads that are waiting for a mutex should be moved to the waiting queue. Threads that can be
// scheduled to run should be in the running queue.

// Creates a pthread that executes function. Attributes are ignored, arg is not.
int my_pthread_create( my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

}


// Explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out and
// another can be scheduled if one is waiting.
void my_pthread_yield() {

}


//Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL,
//any return value from the thread will be saved.
void pthread_exit(void *value_ptr) {

}


// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

}


// Mutex note: Both the unlock and lock functions should be very fast. If there are any threads that are meant to compete for these functions, my_pthread_yield should be called immediately after running the function in question. Relying on the internal timing will make the function run slower than using yield.

// Initializes a my_pthread_mutex_t created by the calling thread. Attributes are ignored.
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {

}


// Locks a given mutex, other threads attempting to access this mutex will not run until it is unlocked.
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {

}


// Unlocks a given mutex.
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {

}


// Destroys a given mutex. Mutex should be unlocked before doing so.
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {

}

