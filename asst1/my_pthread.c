#include "my_pthread_t.h"


// _________________ Macros _______________________________

#define STACK_SIZE 8388608	//default size of call stack
#define NUM_PRIORITY 5		//number of static priority levels


// ____________________ Struct Defs ________________________
//             * MOVED TO THE HEADER FILE *
/* 
enum thread_status {running, yield, wait_thread, wait_mutex, unlock, thread_exit, embryo};

typedef struct tid_node {
	int tid;
	struct tid_node* next;
} tid_node_t;

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
	int thread_id;			//integer identifier of thread
	int priority;			// current priority level of this thread
	int intervals_run;		// the number of concecutive intervals this thread has run
	enum thread_status status;	// the threads current status
	void* ret;			//return value of the thread
	struct my_pthread * waiting;	// reference to a thread waiting on this thread to exit, otherwise NULL
	ucontext_t uc;			//execution context of given thread
} my_pthread_t;

typedef struct my_pthread_mutex {
	Queue * waiting;		// queue of threads waiting on this mutex
	my_pthread_t * user;		// reference to the thread that currently has the mutex, NULL if not claimed
} my_pthread_mutex_t;
*/


// ___________________ Globals ______________________________

static ucontext_t main_context;			//execution context for main 
static Queue* priority_level[NUM_PRIORITY]; 	//array of pointers to queues associated with static priority levels
static int current_priority;			// current piority level that is being run
static int run_at_priority;			// threads run at the current priority
static void* ret; 				//used to store return value from terminated thread
static struct * itimerval timer;		// timer to periodically activate the scheduler
static struct * itimerval pause;		// a zero itimerval used to pause the timer
static struct * itimerval cont;			// a place to store the current time
static my_pthread_t * running_thread		// reference to the currently running thread
static short init;				// flag for if the scheduler has been initialized
static int thread_count;			// Counter to generate new, sequential TIDs
static tid_node_t * tid_list;			// pointer to front of list of available TIDs


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


// Function to assign thread_id
int get_ID() {

	int tid;
	tid_node_t * ptr;	

	if(tid_list == NULL) {			//if the list is empty, issue a new ID
		tid = thread_count;
		thread_count++;
	} else {				//otherwise, take a recycled ID from the list
		ptr = tid_list;
		tid_list = tid_list->next;
		TID = ptr->tid;
		free(ptr);	
	}

	return TID;
}


// Funtion to free a thread_id
void free_ID(int thread_id) {
	
	// create new node to hold available id, place at the front of the list
	tid_node_t * ptr = malloc(sizeof(tid_node_t));	
	ptr->tid = thread_id;
	ptr->next = tid_list;
	tid_list = ptr;
	return;
}


// Function to initialize the scheduler
int scheduler_init() {  		// should we return something? int to signal success/error? 

	thread_counter = 1;		//start threads at 1
	tid_list = NULL;		//list starts empty
		
	//initialize queues representing priority levels
	int i;
	for(i = 0; i < NUM_PRIORITY; i++) {
		priority_level[i] = make_queue();
	}

	// create a context/thread for main and set it as runnning_thread
	running_thread = malloc(sizeof(my_pthread_t));

	if(getcontext(&main_context) == -1) {
                return -1;
        }

        if(makecontext(&main_context, main, 2) == -1) {
                return -1;
        }

	running_thread->uc = main_context;
	running_thread->status = running;
	running_thread->priority = 0;
	running_thread->intervals_run = 0;

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
	cont = malloc(sizeof(struct itimerval));
	setitimer(ITIMER_VIRTUAL, timer, NULL);

	return 0;
}


// Function to clean up the scheduler
void scheduler_clean() {

}

//queue priority stuff
int maintenance_cycle(void) {
	
	return 0;
}

//Signal handler to activate the scheduler on periodic SIGVTALRM, this is the body of the scheduler
void scheduler_alarm_handler(int signum) {
	// pause the timer
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// check status of currently running thread
	switch (running_thread->status) {
		case running :
			// check if the thread has finished its alotted time, if not increment its interval counter and resume
			if (running_thread->intervals_run < ) { // was stopped prematurely
				setitimer(ITIMER_VIRTUAL, timer, NULL);
				return;
			}
			// otherwise, drop the priority level and enqueu
			running_thread->intervals_run = 0;
			running_thread->priority = (running_thread->priority + 1)%NUM_PRIORITY; // if its at the bottom, gets moved back to the top

		case yield :
			// enqueue the current thread back in the same priority level
			enqueue(running_thread, priority_level[running->thread->priority]);

			break;

		case wait_thread :
			// move running thread to the waiting queue 
			// enqueu(running_thread, waiting);
			// NOTE: waiting queue is obsolete, don't need to do anything here

			break;

		case wait_mutex :
			// don't really need to do anything here?
			break;

		case thread_exit :
			// take care of return values
			ret = running_thread->ret;

			// clean up current thread
			free(running_thread->uc.uc_stack.ss_sp)	//free stack space
			free (running_thread);

			break;

		case embryo :
			// what do you do here?????
			// ANSWER: This state is not needed, thread creation is atomic so the scheduler will not interupt it

		default :

	}

	// update the priority counters
	if (run_at_priority++ > current_priority) {
		current_priority = (current_priority + 1)%NUM_PRIORITY;
		run_at_priority = 0;
	}

	// select new thread to run and set it as the running thread then swap to the new context
	my_pthread_t * prev_thread = running_thread;
	running_thread = get_next(priority_level[current_priority]);
	swapcontext(prev_thread->uc, running_thread->uc);

	// reset the timer
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

	thread->status = embryo;	//thread in process of being created

	// check and initialize the scheduler if needed
	if (!init) {
		scheduler_init();
	}

	// pause the timer, this should be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	ucontext_t* ucp = &(my_pthread_t->uc); // = thread->uc ? I'm pretty sure it's right because the context is actually in the struct. I did that to simplify memory allocation

	if(getcontext(ucp) == -1) {
		return -1;
	}

	ucp->uc_link = main_context;
	ucp->uc_stack.ss_sp = malloc(STACK_SIZE);
	ucp->uc_stack_ss_size = STACK_SIZE;
	
	if(makecontext(ucp, function, 1) == -1) {  // thread->argc ? Francisco confirmed argc is always 1
		return -1;
	}

	thread->thread_id = get_ID();  // how are we assigning IDs? In sequence starting at 1
	thread->priority = 0;
	thread->intervals_run = 0;
	thread->ret = NULL;
	thread->waiting = NULL;
	enqueue(thread, priority_level[0]);
	thread->status = running;

	// resume timer
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}


// Explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out and
// another can be scheduled if one is waiting.
void my_pthread_yield() {
	// set the status of the thread to yield then signal the scheduler
	running_thread->status = yield;
	raise(SIGVTALRM);	
}


// Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL,
// any return value from the thread will be saved.
void pthread_exit(void *value_ptr) {

	if(value_ptr != NULL) {
		ret = value_ptr;	//saves value to global variable
	}

	// set thread status to and signal the scheduler to take care of it
	running_thread->status = exit;
	raise(SIGVTALRM);
}


// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	// pause timer, should this be atomic?
	setitimer(ITIMER_VIRTUAL, pause, cont);
	
	// set status of the current thread
	running_thread->status = wait_thread;
	thread.waiting = running_thread;  // the thread that called is the one waiting on this thread, no search required
//	thread.waiting = search(thread.thread_id, priority_level);	//need a function that searches the queue for a given thread by TID

	// resume timer and signal so another thread can be scheduled
	setitimer(ITIMVER_VIRTUAL, cont, NULL);
	raise(SIGVTALRM);

	return thread.ret;
}


// Mutex note: Both the unlock and lock functions should be very fast. If there are any threads that are meant to compete for these functions, my_pthread_yield should be called immediately after running the function in question. Relying on the internal timing will make the function run slower than using yield.

// Initializes a my_pthread_mutex_t created by the calling thread. Attributes are ignored.
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	mutex->waiting = make_queue();
	mutex->user = NULL;
	return 0;
}


// Locks a given mutex, other threads attempting to access this mutex will not run until it is unlocked.
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// pause timer, this opperation needs to be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// check whether the mutex is available and assign it or add the thread to the mutex queue
	if (mutex->user == running_thread) { 
		// already have the lock, resume the clock and return
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return 0;
	} else if (mutex->user) {
		// the mutex is claimed, so we need to wait for it
		enqueue(running_thread, mutex->waiting);
		// mark the thread as waiting
		running_thread->status = wait_mutex;
		// resume timer and signal so another thread can be scheduled
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		raise(SIGVTALRM);
		return 0;
	} else {
		mutex->user = running_thread;
	}
	// resume timer
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}


// Unlocks a given mutex.
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// should we make the thread lock the mutex before it can unlock it or do we leave it to the user to use the mutex properly?
	int exit_code = my_pthread_mutex_lock(mutex);
	if (exit_code)
		return exit_code;

	// pause timer, this needs to be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// check that the given thread has the mutex
	if (mutex->user == running_thread) {
		mutex->user = NULL;
	else // Huston, we have a problem. 
		exit_code = -1;

	// give the lock to the next in line and reactivate them
	my_pthread_mutex_t * next = get_next(mutex->queue);
	if (next) {
		next->status = running;
		enqueu(next, active);
	}

	// resume the timer and return
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return exit_code;
}


// Destroys a given mutex. Mutex should be unlocked before doing so.
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
// questions on semantics:
//	- should a thread need to claim/lock a mutex before destroying it?
//	- do we leave it on the user to be safe?


	// pause timer, does this need to be atomic?
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// clean up the mutex and unlock it
	my_pthread_t * temp;
	while (mutex->size) {
		temp = get_next(mutex->waiting);
		temp->status = running;
		enqueu(temp, prioritiy_level[temp->priority]);
	}
	free(mutex->waiting);
	mutex->user = NULL;

	// resume timer and return
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}

