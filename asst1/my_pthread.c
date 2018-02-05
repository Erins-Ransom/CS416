#include "my_pthread_t.h"


// _________________ Macros _______________________________

#define STACK_SIZE 8388608



// ____________________ Struct Defs ________________________

typedef struct Node {
	my_pthread_t * thread;
	struct Node * next;
	struct Node * prev;
} Node;

typedef struct Queue {
	Node * top;
	Node * bottom;
	int size;
} Queue;

typedef struct my_pthread {
	int thread_id;		//integer identifier of thread
	int argc;		//number of arguments in function
	int exit;		//0 if thread is marked active
	void* ret;		//return value of the thread
	ucontext_t uc;		//execution context of given thread
} my_pthread_t;



// ___________________ Globals ______________________________

static ucontext_t scheduler;
static Queue* active, * waiting;	// add more active queues for priority levels?  one per level? 
static void* ret; 			//used to store return value from terminated thread
static struct * itimerval timer;	// timer to periodically activate the scheduler
static struct * itimerval pause;	// a zero itimerval used to pause the timer
static my_pthread_t * running_thread	// reference to the currently running thread



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
my_pthread_t * get_next(Queue * Q) {
	my_thread_t * ret = NULL;
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
void enqueue(my_pthread_t * thread, Queue * Q) {
	Node * new = malloc(sizeof(Node));
	new->thread = thread;
	new->prev = NULL;
	if (Q->bottom)
		Q->bottom->prev = new;
	new->next = Q->bottom;
	if (!Q->top)
		Q->top = new;
	Q->bottom = new;
	Q->size++;
}


// Function to initialize the scheduler
void scheduler_init() {  		// should we return something? int to signal success/error? 
	// initialize the queues
	active = make_queue();
	waiting = make_queue();

	// create a context/thread for main and enqueue it?

	// set up pause and timer to send a SIGVTALRM every 25 usec
	pause = malloc(sizeof(struct itimerval));
	pause->it_value.tv_sec = 0;
	pause->it_value.tv_usec = 0;
	pause->it_interval.tv_sec = 0;
	pause->it_interval.tv_usec = 0;
	timer = malloc(sizeof(struct itimerval));
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 25;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 25;
	setitimer(ITIMER_VIRTUAL, timer, NULL);

}


// Function to clean up the scheduler
void scheduler_clean() {

}


//Signal handler to activate the scheduler on periodic SIGVTALRM, this is the body of the scheduler
void scheduler_alarm_handler(int signum) {
	// pause the timer
	setitimer(ITIMER_VIRTUAL, pause, timer);

	// check status of currently running thread


	// if needed, select new thread to run


	// enqueue or remove the old thread


	// resume the timer
	setitimer(ITIMER_VIRTUAL, timer, NULL);
	
}

//signal handler to activate the scheduler to store the return value from a terminated thread
void user1_signal_handler(int signum) {
	// I think this will be covered by the above signal handler, but I could be mistaken?
}



// __________________ API ____________________

// Pthread Note: Your internal implementation of pthreads should have a running and waiting queue.
// Pthreads that are waiting for a mutex should be moved to the waiting queue. Threads that can be
// scheduled to run should be in the running queue.

// Creates a pthread that executes function. Attributes are ignored, arg is not.
int my_pthread_create( my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {	

	ucontext_t* ucp = &(my_pthread_t->uc);

	if(getcontext(ucp) == -1) {
		return -1;
	}

	ucp->uc_stack.ss_sp = malloc(STACK_SIZE);	//stack lives on the heap... is this right?
	ucp->uc_ss_size = STACK_SIZE
	
	if(makecontext(ucp, function, my_pthread_t->argc) == -1) {
		return -1;
	}

	thread->exit = 0;
	enqueue(thread, active);
	return 0;
}


// Explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out and
// another can be scheduled if one is waiting.
void my_pthread_yield() {
	// should set some value/flag, maybe in my_pthread struct, to signal this is a yield
	raise(SIGVTALRM);	
}


// Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL,
// any return value from the thread will be saved.
void pthread_exit(void *value_ptr) {

	if(value_ptr != NULL) {
		ret = value_ptr;	//saves value to global variable
	}

	//somehow the current thread needs to be maked as done
	raise(SIGUSR1);
}


// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	while(thread.exit == 0) {
		//waitd for thread to exit
	}

	return *value_ptr;
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

