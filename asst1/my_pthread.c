// name: Eric Knorr, Daniel Parks, Eric Fiore
// netID: erk58, dap291, ejf96

#include "my_pthread_t.h"


// _________________ Macros _______________________________

#define PAGE_SIZE 4096
#define NUM_PAGES 2000
#define STACK_SIZE (16*PAGE_SIZE)	//default size of call stack, if this is too large, it will corrupt the heap and cause free()'s to segfault
#define NUM_PRIORITY 5			//number of static priority levels
#define THREAD_LIM 40			// maximum number of threads allowed
#define MUTEX_LIM 100			// maximum number of mutexes allowed

// These two macros are for accessing the size metadata and allocated metadata of a pointer
// associated with a block in our memory which are stored in the first 4 bytes of eqch block
#define size(ptr) *((int*)(ptr))
#define allocd(ptr) *((short*)((ptr) + 4))


// ___________________ Globals ______________________________

static void * memory;				// simulated main memory
static short * page_meta;			// pointer to the page metadata in memeory
static void * private_mem;			// pointer to the area of memory reserved for the scheduler
static int private_lim;
static void * public_mem;			// pointer to the area of memory for gerneral thread use
static int public_lim;
static int public_pages;
static void * shared_mem;			// pointer to the area of memory for shared use
static int shared_lim;
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
static my_pthread_mutex_t * mutex_list[MUTEX_LIM]; // list of active mutexes
static volatile sig_atomic_t done[THREAD_LIM];	// flag for if a thread has finished
static struct sigaction act;



// _________________ Memory Management ____________________


void vmem_sig_handler(int signo, siginfo_t *info, void *context) {

        // pause the timer
        setitimer(ITIMER_VIRTUAL, pause, cont);

        /*
         * This value represents several things:
         * - the position of the page in a thread's page_list that it tried to access
         * - the position of the actual frame the thread tried to access in memory
         * - from this value the signal handler will determine which pages to swap
         */
        int index = ((intptr_t)info->si_addr - (intptr_t)public_mem)/PAGE_SIZE;

        /*
         * this block of code identifies the location of the metadata associated
         * with the page the thread tried to access. It takes the pointer to the
         * page dervied from index and iterates through the metadata array until
         * it finds a match
         */
        void * wrong_page = public_mem + index*PAGE_SIZE;        // address of page that the thread tried to access

	// if the page has not been used yet, assign it to the current thread and unprotect it
	if (page_meta[2*index] < 0) {
		page_meta[2*index] = running_thread->id;
		page_meta[2*index + 1] = index;
		running_thread->num_pages++;
		mprotect(wrong_page, PAGE_SIZE, PROT_NONE);
		return;
	}

	int swap_index = 0;
	while (swap_index < public_pages) {
		if (page_meta[2*swap_index] == running_thread->id && page_meta[2*swap_index+1] == index) {
			break;
		} else if (page_meta[2*swap_index] < 0 && running_thread->num_pages < index) {
			running_thread->num_pages++;
			break;
		}
	}

	if (swap_index == public_pages) {
		// out of space, go to swap file
		fprintf(stderr, "ERROR: No more space\n");
	}

        // address of page that the thread wanted to access within it's own page_list
        void * right_page = public_mem + swap_index*PAGE_SIZE;


        /*
         * this block of code swaps the page in memory
         * and updates the page metadata
         */
        mprotect(wrong_page, PAGE_SIZE, PROT_READ | PROT_WRITE);         // unlock wrong page
	mprotect(right_page, PAGE_SIZE, PROT_READ | PROT_WRITE);
        char temp_page[PAGE_SIZE];                                       // swap space
        memcpy(temp_page, right_page, PAGE_SIZE);
	page_meta[2*swap_index] = page_meta[2*index];
	page_meta[2*swap_index + 1] = index;
        memcpy(right_page, wrong_page, PAGE_SIZE);
	page_meta[2*index] = running_thread->id;
	page_meta[2*index + 1] = index;
        memcpy(wrong_page, temp_page, PAGE_SIZE);
        mprotect(right_page, PAGE_SIZE, PROT_NONE);

        // resume timer
        setitimer(ITIMER_VIRTUAL, cont, NULL);

        return;
}


int scheduler_init();

void memory_init() {

	if (!init) {
		scheduler_init();
	}

        // pause the timer
        setitimer(ITIMER_VIRTUAL, pause, cont);

        memory = memalign( PAGE_SIZE, PAGE_SIZE*NUM_PAGES);         // allocates memory for paging

	// set up the size and pointers to each section of memory

	shared_lim = 4*PAGE_SIZE;							// 4 pages for shared allocations
	public_lim = (3*(NUM_PAGES - 16*THREAD_LIM - 4)/4)*PAGE_SIZE;			// 3/4 of the remainning space - stacks for general thread use
	public_pages = public_lim/PAGE_SIZE;
	private_lim = ((NUM_PAGES - 16*THREAD_LIM - 4)/4)*PAGE_SIZE - 4*public_pages;	// the remaining 1/4 for metadata and the scheduler

	page_meta = memory + STACK_SIZE*THREAD_LIM;
	private_mem = page_meta + 4*public_pages;
	public_mem = private_mem + private_lim;
	shared_mem = public_mem + public_lim;

	// initialize metadata

	int i;
	for (i = 0; i < 2*public_pages; i++) {
		((short *)page_meta)[i] = -1;
	}

	size(private_mem) = private_lim - 6;
	allocd(private_mem) = 0;

	size(public_mem) = public_lim - 6;
	allocd(public_mem) = 0;

	size(shared_mem) = shared_lim - 6;
	allocd(shared_mem) = 0;

	// set all memory to protected

	void * ptr = public_mem;
	for (i = 0; i < public_pages; i++) {
		mprotect((ptr + i*PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE);
	}


        /*
         * this block sets up the signal handler
         * and allows it to access the address that
         * caused the segfault
         */
        act.sa_sigaction = vmem_sig_handler;
        act.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &act, NULL);

        // resume timer
        setitimer(ITIMER_VIRTUAL, cont, NULL);

}


void * myallocate(size_t size, char * file, int line, int flag, short TID) {

	void * mem;
	int mem_lim;
	static short firstmalloc = 1;

	// check if this is the first allocation of memory and set up initial metadata
	if (firstmalloc) {
		memory_init();
		firstmalloc = 0;
    
	}

	switch (flag) {
		case STACK_ALLOCATION : 
			if (TID >= 0 && TID < THREAD_LIM) {
				return memory + TID*STACK_SIZE;
			} else {
				fprintf(stderr, "ERROR: Thread limit reached\n\tFile: %s\n\tLine: %d\n", file, line);
				return NULL;
			}
		
		case PRIVATE_REQ :
			mem = private_mem;
			mem_lim = private_lim;
			break;

		case PUBLIC_REQ :
			mem = public_mem;
			mem_lim = public_lim;
			break;

		case SHARED_REQ :
			mem = shared_mem;
			mem_lim = shared_lim;
			break;
	}

	// check if size is larger than total mem - metadata 
	if (size > mem_lim - 6) {
		fprintf(stderr, "ERROR: Not Enough Memmory\n\tFile: %s\n\tLine: %d\n", file, line);
		return NULL;
	}
  
	// go through the "linked list" of blocks until we find an available block big enough, if we hit the end,
	// return NULL and report the lack of space
	void * ptr = mem;
	while (ptr < (void*)(mem + mem_lim)) {
		// a block must be either exactly the right size or have enough extra space to accomodate the 
		// metadata for the free block containing the leftover space 
		if (!allocd(ptr) && (size(ptr) == size || size(ptr) >= size + 6)) {
			break;
		}
		ptr += size(ptr) + 6;
	}

	if (ptr >= (void*)(mem + mem_lim)) {
		fprintf(stderr, "ERROR: Not Enough Memory\n\tFile: %s\n\tLine: %d\n", file, line);
    		return NULL;
  	}

  	// if a suitable block is found and there is leftover space, divide it into an allocated and free block, 
 	// otherwise, simply mark the block as allocated, then return a pointer to the allocated block
  	if (size(ptr) > size) {
    		int leftover = size(ptr) - size - 6;
    		size(ptr) = size;

    		size(ptr + size + 6) = leftover;
   		allocd(ptr + size + 6) = 0;
  	}

  	allocd(ptr) = 1;

  	return ptr + 6;
}


void mydeallocate(void * index, char * file, int line, int flag, short TID) {

	void * mem;
	int mem_lim;

        switch (flag) {
                case STACK_ALLOCATION :
                        if (TID >= 0 && TID < THREAD_LIM && !((index - memory)%STACK_SIZE)) {
                                return;
                        } else {
                                fprintf(stderr, "ERROR: Invalid free\n\tFile: %s\n\tLine: %d\n", file, line);
                                return;
                        }

                case PRIVATE_REQ :
                        mem = private_mem;
                        mem_lim = private_lim;
                        break;

                case PUBLIC_REQ :
                        mem = public_mem;
                        mem_lim = public_lim;
                        break;

                case SHARED_REQ :
                        mem = shared_mem;
                        mem_lim = shared_lim;
                        break;
        }

  	//check if the given index is a valid pointer
  	if (index < (void*)mem || (void*)(mem + mem_lim) <= index) {
    		fprintf(stderr, "ERROR: Can only free a valid pointer\n\tFile: %s\n\t%d\n", file, line);
    		return;
  	}

  	// check our list of blocks for the given index
  	void * ptr = mem;
  	if (ptr + 6 == index) {
    		// free and merge if the pointer is at the begining of the list
    		if (!allocd(ptr + size(ptr) + 6)) {
      			size(ptr) += size(ptr + size(ptr) + 6) + 6;
    		}
    		allocd(ptr) = 0;
    		return;
  	}

  	void * prev = ptr;
  	ptr += size(ptr) + 6;

  	while(ptr < (void*)(mem + mem_lim)) {
    		if (ptr + 6 == index && allocd(ptr)) {
      			break; 
    		}
    		prev = ptr;
    		ptr += size(ptr) + 6;
  	}

  	// report if the pointer is not an allocated block
  	if (ptr >= (void*)(mem + mem_lim)) {
    		fprintf(stderr, "ERROR: Pointer is not allocated \n\tFile: %s\n\tLine: %d\n", file, line);
    		return;
  	}

  	// if we find the block, mark it free and combine with adjacent free blocks
  	if (ptr + size(ptr) + 8 < (void*)mem + mem_lim && !allocd(ptr + size(ptr) + 6)) {
    		// merge with next block if its free
    		size(ptr) += size(ptr + size(ptr) + 6) + 6;
  	}

  	allocd(ptr) = 0;

  	if (!allocd(prev)) {
    		// merge with previous block if its free
    		size(prev) += size(ptr) + 6;
  	}

}






// _________________ Utility Functions _____________________

// Function to initialize a Queue
Queue * make_queue() {
	Queue * new = myallocate(sizeof(Queue), __FILE__, __LINE__, PRIVATE_REQ, -1);
	if(new == NULL) {
		return 0;
	}
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
		mydeallocate(Q->back, __FILE__, __LINE__, PRIVATE_REQ, -1);
		Q->back = NULL;
	} else {
		ret = Q->back->next->thread;
		temp = Q->back->next;
		if (Q->size == 2) {
			Q->back->next = Q->back;
		} else {
			Q->back->next = Q->back->next->next;
		}
		mydeallocate(temp, __FILE__, __LINE__, PRIVATE_REQ, -1);
	}

	Q->size--;
	return ret;

}

// function to add a context to the Queue
void enqueue(my_pthread_t * thread, Queue * Q) {
	Node * new = myallocate(sizeof(Node), __FILE__, __LINE__, PRIVATE_REQ, -1);
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
short get_ID() {

	short tid;
	tid_node_t * ptr;	

	if (tid_list == NULL) {			//if the list is empty, issue a new ID
		tid = thread_count;
		thread_count++;
	} else {				//otherwise, take a recycled ID from the list
		ptr = tid_list;
		tid_list = tid_list->next;
		tid = ptr->tid;
		mydeallocate(ptr, __FILE__, __LINE__, PRIVATE_REQ, -1);	
	}

	if (tid >= THREAD_LIM)			// exceeded max 
		tid = -1;

	return tid;
}


// Funtion to free a thread_id
void free_ID(int thread_id) {
	
	// create new node to hold available id, place at the front of the list
	tid_node_t * ptr = myallocate(sizeof(tid_node_t), __FILE__, __LINE__, PRIVATE_REQ, -1);	
	ptr->tid = thread_id;
	ptr->next = tid_list;
	tid_list = ptr;
	return;
}

void scheduler_alarm_handler(int signum);

// Function to initialize the scheduler
int scheduler_init() {  		// should we return something? int to signal success/error? 

	init = 1;
	thread_count = 1;		//generates the first TID which will be 1
	tid_list = NULL;		//list starts empty	
	
	//initialize queues representing priority levels
	//NUM_PRIORITY = 5 so this will make 5 (0-4) new queues
	int i;
	for(i = 0; i < NUM_PRIORITY; i++) {
		priority_level[i] = make_queue();
	}
	death_row = make_queue();

	// initialize thread table to null
	for(i = 0; i < THREAD_LIM; i++) {
		thread_table[i] = NULL;
	}

	for(i = 0; i < MUTEX_LIM; i++) {
		mutex_list[i] = NULL;
	}

	/*****************************************************
 	 * The below block of code creates a thread for main *
	 * and sets it as the running thread                 *
	 * this block of code will end at the next           *
	 * block of comments		                     *
	 *****************************************************/
	running_thread = myallocate(sizeof(my_pthread_t), __FILE__, __LINE__, PRIVATE_REQ, -1);		//malloc a block of memory for the currently running thread

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
	pause = myallocate(sizeof(struct itimerval), __FILE__, __LINE__, PRIVATE_REQ, -1);		//sets aside memory for the pause timer
	pause->it_value.tv_sec = 0;				//seconds are not used here
	pause->it_value.tv_usec = 0;				//this is a pause timer so the timer should no time should be run
	pause->it_interval.tv_sec = 0;				//seconds are not used here
	pause->it_interval.tv_usec = 0;				//this is a pause timer so the interval should be 0
	timer = myallocate(sizeof(struct itimerval), __FILE__, __LINE__, PRIVATE_REQ, -1);		//set aside memory for the timer
	timer->it_value.tv_sec = 0;				//seconds are not used here
	timer->it_value.tv_usec = 25;				//the initial time should be 25 microseconds
	timer->it_interval.tv_sec = 0;				//seconds are not used here
	timer->it_interval.tv_usec = 25;			//at the experiation of the time the value should be reset to 25 microseconds
	cont = myallocate(sizeof(struct itimerval), __FILE__, __LINE__, PRIVATE_REQ, -1);		//set aside memoory for he cont timer where the current time will be set at a future point
	setitimer(ITIMER_VIRTUAL, timer, NULL);			//start the timer

	// activate the scheduler
	signal(SIGVTALRM, scheduler_alarm_handler);

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
			// don't really need to do anything here
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

	// mprotect all of prev_thread's memory and unprotect running_thread's memory
	int i;
	for (i = 0; i < public_pages; i++) {
		if (page_meta[2*i + 1] == i) {
			if (page_meta[2*i] == prev_thread->id) {
				mprotect((public_mem + i*PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE);
			} else if (page_meta[2*i] == running_thread->id) {
				mprotect((public_mem + i*PAGE_SIZE), PAGE_SIZE, PROT_NONE);
			}
		}
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

	if(attr < 0) { //attr is ignored, but if it is explicity a negative value aka someone trying to break our code, return error
		return EINVAL;
	}

	// check and initialize the scheduler if needed
	if (!init) {
		scheduler_init();
	}

	// pause the timer, this should be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	ucontext_t* ucp = &(thread->uc);

	if(getcontext(ucp) == -1) {
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return -1;
	}

	thread->id = get_ID();
	ucp->uc_stack.ss_sp = myallocate(STACK_SIZE, __FILE__, __LINE__, STACK_ALLOCATION, thread->id);
	
	if(ucp->uc_stack.ss_sp == NULL) { //malloc() failed to get get necessary resources, set errno and return
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return EAGAIN;
	}

	ucp->uc_stack.ss_size = STACK_SIZE;
	ucp->uc_link = &main_context;

	makecontext(ucp, (void (*)(void))function, 1, arg);	// thread->argc ? Francisco confirmed argc is always 1

	thread->wait_id = -1;		// not waiting on a thread
	thread->priority = 0;
	thread->intervals_run = 0;
	thread->ret = NULL;
	thread_table[thread->id] = thread;
	waiting[thread->id] = NULL;
	done[thread->id] = 0;
	enqueue(thread, priority_level[0]);
	thread->status = active;

	thread->num_pages = 0;
	
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

	// two threads tried to join with each other
	if(running_thread->id == thread.id) {
		return EDEADLK;
	}

	// thread tries to wait on a thread that is already waiting on calling thread
	if(thread.wait_id == running_thread->id) {
		return EDEADLK;
	}

	//Another thread is already waiting to join with this thread.
	int i;
	for(i = 0; i < THREAD_LIM; i++) {
		if(waiting[i] && waiting[i]->wait_id == running_thread->id) {
			return EINVAL;
		}
	}

	if(thread_table[thread.id] == NULL) {
		return ESRCH;
	}

	// if the thread to be joined is not finished, wait on it
	if (!done[thread.id]) {
		//pause timer
		setitimer(ITIMER_VIRTUAL, pause, cont);
		running_thread->status = wait_thread;
		running_thread->wait_id = thread.id;
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
	mydeallocate(thread_table[thread.id]->uc.uc_stack.ss_sp, __FILE__, __LINE__, STACK_ALLOCATION, thread.id);
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
	
	// no memory to make the queue
	if(mutex->waiting == NULL) {
		return ENOMEM;
	}

	// attempt to reinitialize a mutex already in use
	int i;
	for(i = 0; i < MUTEX_LIM; i++) {
		if(mutex_list[i] == mutex) { 
			return EBUSY;
		}
	}

	// invalid entry for mutexattr
	if(mutexattr < 0) {
		return EINVAL;
	}

	mutex->user = NULL;

	// add mutex to list of active mutexes	
	for(i = 0; i < MUTEX_LIM; i++) {
		if(mutex_list[i] == NULL) {
			mutex_list[i] = mutex;
			break;
		}
	}
	
	return 0;
}


// Locks a given mutex, other threads attempting to access this mutex will not run until it is unlocked.
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// pause timer, this opperation needs to be atomic
	setitimer(ITIMER_VIRTUAL, pause, cont);

	// invalid value for mutex
	if(mutex <= 0) {
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return EINVAL;
	}

	// check whether the mutex is available and assign it or add the thread to the mutex queue
	if (mutex->user == running_thread) { 
		// already have the lock, resume the clock and return
		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return EDEADLK;
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
 		setitimer(ITIMER_VIRTUAL, cont, NULL);
		return EPERM;
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

	// remove mutex from active list, return error if mutex is still locked
	int i;
	for(i = 0; i < MUTEX_LIM; i++) {
		if(mutex_list[i] == mutex ) {
	        	if(mutex->user == NULL) {
				mutex_list[i] == NULL;
				break;
			} else {
				return EBUSY;
			}
		}
	}

	// mutex not found in list
	if(i == MUTEX_LIM) {
		return EINVAL;
	}

	// clean up the mutex and unlock it
	my_pthread_t * temp;
	while (mutex->waiting->size) {
		temp = get_next(mutex->waiting);
		temp->status = active;
		enqueue(temp, priority_level[temp->priority]);
	}
	mydeallocate(mutex->waiting, __FILE__, __LINE__, PRIVATE_REQ, -1);
	mutex->user = NULL;

	// resume timer and return
	setitimer(ITIMER_VIRTUAL, cont, NULL);
	return 0;
}
