#include <stdio.h>
#include "mymalloc.h"

// These two macros are for accessing the size metadata and allocated metadata of a pointer
// associated with a block in our memory which are stored in the first 4 bytes of eqch block
#define size(ptr) *((short*)(ptr))
#define allocd(ptr) *((short*)((ptr) + 2))
#define TOTSIZE 8000000

// Here is our block of memory which is 8MB total size
// The first 1% of the block is saved for metadata
static char mem[TOTSIZE];
mem_block *meta_ptr = (void*)mem;

void * mymalloc(size_t size, char * file, int line) {

  //static short firstmalloc = 1;

  // check if this is the first allocation of memory and set up initial metadata
  /*
    if (firstmalloc) {
    size(mem) = 19996; 
    allocd(mem) = 0; 
    firstmalloc = 0;
  }
  */
	mem_block *curr_block = NULL, *next_block = NULL;
	void* return_value = NULL;
	
	if(!(meta_ptr->size))
	{
		meta_ptr->size = TOTSIZE - (TOTSIZE/100);	//set size of block to 99% of total block size
		meta_ptr->is_free = 1;				//set block to available
		meta_ptr->next = NULL;				//no next block yet so set to NULL
		meta_ptr->alloc_start = (void*)(mem + (TOTSIZE/100) + 1);	//set beginning of first byte after end of metadata area
	}

  // check if size is larger than total mem - metadata 
  if (size > TOTSIZE - (TOTSIZE - (TOTSIZE/100))) {
    fprintf(stderr, "ERROR: Not Enough Memmory\n\tFile: %s\n\tLine: %d\n", file, line);
    return NULL;
    }
  
  // go through the "linked list" of blocks until we find an available block big enough, if we hit the end,
  // return NULL and report the lack of space
  /*
  void * ptr = mem;
  while (ptr < (void*)(mem + 20000)) {
    // a block must be either exactly the right size or have enough extra space to accomodate the 
    // metadata for the free block containing the leftover space 
    if (!allocd(ptr) && (size(ptr) == size || size(ptr) >= size + 4)) {
      break;
    }
    ptr += size(ptr) + 4;
  }
  */
	curr_block = meta_ptr;		//set curr to beginning of linked list of metadata

	while((curr_block->size < size) || (curr_block->is_free == 0 && curr_block->next != NULL))	//search through list of metadata blocks until find a block that is big enough, is free and the end of list is not reached
		++curr_block;			//go to the next metadata block

	/****************************************
 	* if the requested block of memory	*
 	* overuns the total amount of		*
 	* remaining memory print an error and	*
 	* return NULL				*
 	* **************************************/
	if((curr_block->alloc_start + size) > (meta_ptr->alloc_start + 7920000))
	{
		fprintf(stderr, "ERROR: Not Enough Memory\n\tFile: %s\n\tLine: %d\n", file, line);
		return NULL;
	}	

	/****************************************
 	* if the requested size if equal to the	*
 	* current size return the current block	*
 	* **************************************/
	if(curr_block->size == size)
	{
		curr_block->is_free = 0;
		curr_block->size = size;
		return_value = curr_block->alloc_start;
	}
  

	/****************************************
 	* if the requested block of memory is	*
 	* larger then the current size		*
 	* create a new block memory by 		*
 	* splitting up the existing block	*
 	* **************************************/
	else if(curr_block->size > size)	//if the size of the current block of memory is larger then requested
	{
		next_block = (void*)(++curr_block);				//set up a new metadata area one block farther
		next_block->is_free = 1;					//set the block of memory to free
		next_block->next = curr_block->next;				//set the new metadata area to point to the next metadata area
		next_block->size = curr_block->size - size;			//set the new block of memory to the current block of memory minus the requested size
		next_block->alloc_start = (void*)(curr_block->alloc_start + size + 1);//set the new metadata block to point to the new allocatable memory
		curr_block->size = size;					//set the current block to the size of the requested block		
		curr_block->is_free = 0;					//set the free flag to not free
		curr_block->next = next_block;					//set the current metadata to point to the next block
		return_value = curr_block->alloc_start;
	}

	else
	{
		fprintf(stderr, "ERROR: Not Enough Memory to allocate\n");
		return_value = NULL;
	}


	return return_value;
  /*
  if (ptr >= (void*)(mem + 20000)) {
    fprintf(stderr, "ERROR: Not Enough Memory\n\tFile: %s\n\tLine: %d\n", file, line);
    return NULL;
  }
  */


  // if a suitable block is found and there is leftover space, divide it into an allocated and free block, 
  // otherwise, simply mark the block as allocated, then return a pointer to the allocated block
  /*
    if (size(ptr) > size) {
    short leftover = size(ptr) - size - 4;
    size(ptr) = size;

    size(ptr + size + 4) = leftover;
    allocd(ptr + size + 4) = 0;
  }

  allocd(ptr) = 1;

  return ptr + 4;
  */

}


void myfree(void * index, char * file, int line) {

  //check if the given index is a valid pointer
  if (index < (void*)mem || (void*)(mem + 20000) <= index) {
    fprintf(stderr, "ERROR: Can only free a valid pointer\n\tFile: %s\n\t%d\n", file, line);
    return;
  }

  // check our list of blocks for the given index
  void * ptr = mem;
  if (ptr + 4 == index) {
    // free and merge if the pointer is at the begining of the list
    if (!allocd(ptr + size(ptr) + 4)) {
      size(ptr) += size(ptr + size(ptr) + 4) + 4;
    }
    allocd(ptr) = 0;
    return;
  }
  void * prev = ptr;
  ptr += size(ptr) + 4;

  while(ptr < (void*)(mem + 20000)) {
    if (ptr + 4 == index && allocd(ptr)) {
      break; 
    }
    prev = ptr;
    ptr += size(ptr) + 4;
  }

  // report if the pointer is not an allocated block
  if (ptr >= (void*)(mem + 20000)) {
    fprintf(stderr, "ERROR: Pointer is not allocated \n\tFile: %s\n\tLine: %d\n", file, line);
    return;
  }

  // if we find the block, mark it free and combine with adjacent free blocks
  if (ptr + size(ptr) + 6 < (void*)mem + 20000 && !allocd(ptr + size(ptr) + 4)) {
    // merge with next block if its free
    size(ptr) += size(ptr + size(ptr) + 4) + 4;
  }

  allocd(ptr) = 0;

  if (!allocd(prev)) {
    // merge with previous block if its free
    size(prev) += size(ptr) + 4;
  }

}
