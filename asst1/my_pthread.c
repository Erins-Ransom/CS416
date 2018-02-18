#include "my_pthread_t.h"


// _________________ Macros _______________________________

#define STACK_SIZE 8000		//default size of call stack, if this is too large, it will corrupt the heap and cause free()'s to segfault
#define NUM_PRIORITY 1		//number of static priority levels
#define THREAD_LIM 1000		// maximum number of threads allowed



// ___________________ Globals ______________________________

static ucontext_t main_context;			// execution context for main 
static Queue * priority_level[NUM_PRIORITY]; 	// array of pointers to queues associated with static priority levels
static Queue * death_row;			// queue for threads waiting to die
static int current_priority;			// current piority level that is being run
static int run_at_priority;			// threads run at the current priority
static struct itimerval * timer;		// timer to periodically activate the scheduler
static struct itimerval * pause;		// a zero itimerval used to pause the timer
static struct itimerval * cont;			// a place to store the current time
static my_pthread_t * running_thread;		// reference to the currently running thread
static short init;				// flag for if the scheduler has been initialized
static int thread_count;			// Counter to generate new, sequential TIDs
static tid_node_t * tid_list;			// pointer to front of list of available TIDs
static my_pthread_t * thread_table[THREAD_LIM];	// references to all current threads
static my_pthread_t * waiting[THREAD_LIM];	// references to waiting threads
static volatile sig_atomic_t done[THREAD_LIM];	// flag for if a thread has finished


// _________________ Utility Functions _____________________

// Function to initialize a Queue
Queue * make_queue() {
	Queue * new = malloc(sizeof(Queue));
	new->back = NULL;
	new->size = 0;
	return new;
}


// Function to get the next context waiting in the Queue
my_pthread_t * get_next(Queue * Q) {
	my_pthread_t * ret = NULL;
	Node * temp = NULL;;

	if (Q->size == 0) {
		return NULL;
	} else if (Q->size == 1) {
		ret = Q->back->thread;
		free(Q->back);
		Q->back = NULL;
	} else {
		ret = Q->back->next->thread;
		temp = Q->back->next;
		if (Q->size == 2) {
			Q->back->next = Q->back;
		} else {
			Q->back->next = Q->back->next->next;
		}
		free(temp);
	}

	Q->size--;
	return ret;

}

// function to add a context to the Queue
void enqueue(my_pthread_t * thread, Queue * Q) {
	Node * new = malloc(sizeof(Node));
	new->thread = thread;
	if (Q->size) {
		new->next = Q->back->next;
		Q->back->next = new;
	} else {
		new->next = new;
	}
	Q->back = new;
	Q->size++;
}


// Function to assign thread_id
int get_ID() {

	int tid;
	tid_node_t * ptr;	

	if (tid_list == NULL) {			//if the list is empty, issue a new ID
		tid = thread_count;
		thread_count++;
	} else {				//otherwise, take a recycled ID from the list
		ptr = tid_list;
		tid_list = tid_list->next;
		tid = ptr->tid;
		free(ptr);	
	}

	if (tid >= THREAD_LIM)			// exceeded max 
		tid = -1;

	return tid;
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

void scheduler_alarm_handler(int signum);

// Function to initialize the scheduler
int scheduler_init() {  		// should we return something? int to signal success/error? 

	thread_count = 1;		//generates the first TID which will be 1
	tid_list = NULL;		//list starts empty
		
	//initialize queues representing priority levels
	//NUM_PRIORITY = 5 so this will make 6 (0-5) new queues
	int i;
	for(i = 0; i < NUM_PRIORITY; i++) {
		priority_level[i] = make_queue();
	}
	death_row = make_queue();

	/*****************************************************
 	 * The below block of code creates a thread for main *
	 * and sets it as the running thread                 *
	 * this block of code will end at the next           *
	 * block of comments		                     *
	 *****************************************************/
	running_thread = malloc(sizeof(my_pthread_t));		//malloc a block of memory for the currently running thread

	if(getcontext(&main_context) == -1) {			//initializes main_context 
                return -1;
        }

	running_thread->uc = main_context;			//sets the context of the main_context as the running thread
	running_thread->status = active;			//sets the status of the main context to active
	running_thread->priority = 0;				//sets the priority level of the main context to 0 (the highest priority)
	running_thread->intervals_run = 0;			//initialized the number of time slices main_context has run to 0
	running_thread->id = 0;					//sets the thread ID for main to 0
	
	/****************************************************
 	* This ends to block of code where main is set as   *
 	* the running thread                                *
 	* ***************************************************/

	// set up pause and timer to send a SIGVTALRM every 25 usec
	/******************************************************
 	* the below block of code sets up the the itimervalue *
 	* for both pause (pause is a zero timer that will be  *
 	* used for temporarily stopping the timer) and timer  *
 	* (sets the scheduler to go off every 25 milliseconds *
 	* This block of code ends at the next block comment   *
 	*****************************************************/
	pause = malloc(sizeof(struct itimerval));		//sets aside memory for the pause timer
	pause->it_value.tv_sec = 0;				//seconds are not used here
	pause->it_value.tv_usec = 0;				//this is a pause timer so the timer should no time should be run
	pause->it_interval.tv_sec = 0;				//seconds are not used here
	pause->it_interval.tv_usec = 0;				//this is a pause timer so the interval should be 0
	timer = malloc(sizeof(struct itimerval));		//set aside memory for the timer
	timer->it_value.tv_sec = 0;				//seconds are not used here
	timer->it_value.tv_usec = 25;				//the initial time should be 25 microseconds
	timer->it_interval.tv_sec = 0;				//seconds are not used here
	timer->it_interval.tv_usec = 25;			//at the experiation of the time the value should be reset to 25 microseconds
	cont = malloc(sizeof(struct itimerval));		//set aside memoory for he cont timer where the current time will be set at a future point
	setitimer(ITIMER_VIRTUAL, timer, NULL);			//start the timer

	// activate the scheduler
	init = 1;
	signal(SIGVTALRM, scheduler_alarm_handler);

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
		case active :
			// check if the thread has finished its alotted time, if not increment its interval counter and resume
			if (running_thread->intervals_run++ < current_priority) { 
				setitimer(ITIMER_VIRTUAL, timer, NULL);
				return;
			}
			// otherwise, drop the priority level and enqueu
			running_thread->intervals_run = 0;
			running_thread->priority = (running_thread->priority + 1)%NUM_PRIORITY; // if its at the bottom, gets moved back to the top

		case yield :
			// enqueue the current thread back in the same priority level
			enqueue(running_thread, priority_level[running_thread->priority]);

			break;

		case wait_mutex :
			// don't really need to do anything here?
			break;

		case thread_exit :
			// if there is a thread waiting on this one, re-activate it
			if (waiting[running_thread->id]) {
				waiting[running_thread->id]->status = active;
				enqueue(waiting[running_thread->id], priority_level[waiting[running_thread->id]->priority]);
			}

			break;

		default :
			//ERROR
			break;
	}

	// update the priority counters
	if (run_at_priority++ < NUM_PRIORITY - current_priority) {
		current_priority = (current_priority + 1)%NUM_PRIORITY;
		run_at_priority = 0;
	}
	

	// select new thread to run and set it as the running thread then swap to the new context
	my_pthread_t * prev_thread = running_thread;
	running_thread = get_next(priority_level[current_priority]);
	while (!running_thread) {
		current_priority = (current_priority + 1)%NUM_PRIORITY;
		running_thread = get_next(priority_level[current_priority]);
	}

	// reset the timer
	setitimer(ITIMER_VIRTUAL, timer, NULL);

	swapcontext(&(prev_thread->uc), &(running_thread->uc));
	
}

// __________________ API ____________________

// Pthread Note: Your internal implementation of pthreads should have a running and waiting queue.
// Pthreads that are waiting for a mutex should be moved to the waiting queue. Threads that can be
// scheduled to run should be in the running queue.

// Creates a pthread that executes function. Attributes are ignored, arg is not.
int my_pthread_create( my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {	

	// check and initialize the scheduler if needed
	if (!init) {
		scheduler_init();
	}

	// pause the timer, this should be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	ucontext_t* ucp = &(thread->uc);

	if(getcontext(ucp) == -1) {
		return -1;
	}

	ucp->uc_stack.ss_sp = malloc(STACK_SIZE);	//stack lives on the heap... is this right? I belive so EF
	ucp->uc_stack.ss_size = STACK_SIZE;
	ucp->uc_link = &main_context;

	makecontext(ucp, (void (*)(void))function, 1, arg);	// thread->argc ? Francisco confirmed argc is always 1

	thread->id = get_ID();		// how are we assigning IDs? In sequence starting at 1
	thread->priority = 0;
	thread->intervals_run = 0;
	thread->ret = NULL;
	thread_table[thread->id] = thread;
	waiting[thread->id] = NULL;
	done[thread->id] = 0;
	enqueue(thread, priority_level[0]);
	thread->status = active;

	// resume timer
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}


// Explicit call to the scheduler requesting that the current context be swapped out
void my_pthread_yield() {
	// set the status of the thread to yield then signal the scheduler
	running_thread->status = yield;
	raise(SIGVTALRM);	
}


// Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL,
// any return value from the thread will be saved.
void pthread_exit(void *value_ptr) {
	// pause the timer
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// set the address of the return value
	running_thread->ret = value_ptr;

	// set thread status to exit and signal the scheduler to take care of it
	running_thread->status = thread_exit;
	done[running_thread->id] = 1;
	raise(SIGVTALRM);
}


// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

	// if the thread to be joined is not finished, wait on it
	if (!done[thread.id]) {
		//pause timer
		setitimer(ITIMER_VIRTUAL, pause, cont);
		running_thread->status = wait_thread;
		waiting[thread.id] = running_thread; 
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		raise(SIGVTALRM);
	}
	
	// pause timer
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// set the return value
	if (value_ptr)
		*value_ptr = thread_table[thread.id]->ret;

	// clean up the finished thread
	waiting[thread.id] = NULL;
	free(thread_table[thread.id]->uc.uc_stack.ss_sp);
	thread_table[thread.id] = NULL;
	free_ID(thread.id);

	// resume
	setitimer(ITIMER_VIRTUAL, cont, NULL);

	return 0; // or error code
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
		// ERROR ?
		return 0;
	} else if (mutex->user) {
		// the mutex is claimed, so we need to wait for it
		enqueue(running_thread, mutex->waiting);
		// mark the thread as waiting
		running_thread->status = wait_mutex;
		// resume timer and signal so another thread can be scheduled
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		raise(SIGVTALRM);
	}
		
	// claim the mutex
	mutex->user = running_thread;

	// resume timer
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}


// Unlocks a given mutex.
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// pause timer, this needs to be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// check that the given thread has the mutex
	if (mutex->user != running_thread) {
		// ERROR
 		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return -1;
	}

	// give the lock to the next in line and reactivate them
	mutex->user = get_next(mutex->waiting);
	if (mutex->user) {
		mutex->user->status = active;
		enqueue(mutex->user, priority_level[mutex->user->priority]);
	}

	// resume the timer and return
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
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
	while (mutex->waiting->size) {
		temp = get_next(mutex->waiting);
		temp->status = active;
		enqueue(temp, priority_level[temp->priority]);
	}
	free(mutex->waiting);
	mutex->user = NULL;

	// resume timer and return
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}

