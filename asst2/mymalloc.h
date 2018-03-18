#ifndef MYMALLOC
#define MYMALLOC

#include <stdio.h>

#define THREADREQ 0
#define LIBRARYREQ 1

#define malloc(x) mymalloc(x, __FILE__, __LINE__, THREADREQ)
#define free(x) myfree(x, __FILE__, __LINE__, THREADREQ)

void * mymalloc(size_t size, char * file, int line, int request);
void myfree(void * index, char * file, int line, int request);

typedef struct memBlock
{
size_t size;			//the size of the allocation
struct memBlock *next;		//points to the beginning of the next block
struct memBlock *next_page;	//points to the next block containing data for the same allocation
void *alloc_start;		//points to the beginning of the allocated block
short is_free;			//if the block is free set to 1 and 0 if not free
short page_num;			//the page number
int  TID;			//the ID of the thread that is occupying this page
}mem_block;


mem_block* mem_split(mem_block *best_fit, size_t size);

#endif
