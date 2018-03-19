#ifndef PAGE_MALLOC
#define PAGE_MALLOC

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ucontext.h>
#include <stdint.h>
#include "my_pthread_t.h"

void vmem_sig_handler(int signo, siginfo_t *info, void *context);
void page_malloc_init();
void * page_malloc(size_t size, char * file, int line, int request);
void page_free(void * index, char * file, int line, int request);
void page_swap(int P1, int P2);

typedef struct page_meta {
	void* page;			//points to page in memory
	struct page_meta* next;		//next page in memory owned by thread		
	struct page_meta* prev;		//previous page in memory owned by thread
	short free;             	//if the page is free set to 1 and 0 if not free	
	int  TID;               	//the ID of the thread that is occupying this page
} page_meta_t;

#endif

