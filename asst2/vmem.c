#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include "my_pthread_t.h"

#define PAGE_SIZE 4096
#define PAGE_LIMIT 512

void* thread_space;
void* page_temp;
struct sigaction act;
my_pthread_t running_thread;

void vmem_sig_handler(int signo, siginfo_t *info, void *context)
{
	int index = (int)((intptr_t)info->si_addr/PAGE_SIZE);					//page thread attempting to access
	void* swap_out = thread_space + index*PAGE_SIZE;					//page that thread was denied access to
	
	page_node_t* ptr = running_thread.pages;
	int i;
	for(i = 0; i < index; i++) {
		ptr = ptr->next;
	}

	void* swap_in = thread_space + ptr->page_num*PAGE_SIZE;					//page that thread means to access

	//swap pages
	memcpy(page_temp, swap_out, PAGE_SIZE);
	memcpy(swap_out, swap_in, PAGE_SIZE);
	memcpy(swap_in, page_temp, PAGE_SIZE);
}

void vmem_init()
{
	// set up signal handler
	act.sa_sigaction = vmem_sig_handler;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, NULL);

	// allocate thread-reserved memory and space for page swapping
	thread_space = malloc(PAGE_SIZE*PAGE_LIMIT);
	page_temp = malloc(PAGE_SIZE);
}
