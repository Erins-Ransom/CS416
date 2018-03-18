#include <stdio.h>
#include "mymalloc.h"

// These two macros are for accessing the size metadata and allocated metadata of a pointer
// associated with a block in our memory which are stored in the first 4 bytes of eqch block
#define size(ptr) *((short*)(ptr))
#define allocd(ptr) *((short*)((ptr) + 2))
#define TOTSIZE 8388608
#define PAGE_SIZE 4092
#define PAGEAREASTART TOTSIZE/100
#define LIBPAGESTART (TOTSIZE - (TOTSIZE/4))

// Here is our block of memory which is 8MB total size
// The first 1% of the block is saved for metadata
// The last 25% of block is reserved for paging requests from library
static char mem[TOTSIZE];
mem_block *meta_ptr = (void*)mem;	//meta_ptr will point to the beginning of the allocatable memory where the metadata is stores 

void * mymalloc(size_t size, char * file, int line, int request) {

  //static short firstmalloc = 1;

  // check if this is the first allocation of memory and set up initial metadata
  /*
    if (firstmalloc) {
    size(mem) = 19996; 
    allocd(mem) = 0; 
    firstmalloc = 0;
  }
  */
	mem_block *curr_block = NULL, *prev_block = NULL;
	void* return_value = NULL;
	
	/********************************
 	* if the is is the first time	*
 	* malloc is called set up the	*
 	* first meta data block		*
 	* ******************************/
	if(!(meta_ptr->page_num))
	{
		curr_block = meta_ptr;
		prev_block = meta_ptr;
		int mem_index = PAGEAREASTART;			//to keep track of the offset where of mempages
		int page_num = 1;				//to assign page numbers
		while(mem_index < (TOTSIZE - PAGE_SIZE))	//while there is still room to add a new page
		{
			curr_block->is_free = 1;		//set page to available
			curr_block->page_num = page_num++;	//set the page number and advance page number
			curr_block->TID = 0;			//there is no thread assigned yet so put it to zero
			curr_block->alloc_start = (void*)(mem + mem_index);//set the beginning address of page
			curr_block->next_page = NULL;		//set the next page to null
			mem_index += PAGE_SIZE;			//advance the index to where the next page will start
			prev_block->next = ++curr_block;	//set the next link for block
			curr_block->next = NULL;		//set next to NULL as this block will always be the last block
			++prev_block;				//advance the prev_block position
		}
	}

  // check if size requested is larger than total mem - metadata 
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

	/****************************************
 	* search through list of metadata block *
 	* until find a block that is big enough,*
 	* is free and end of list is not reached*
 	* **************************************/
	if(request == 0)				//if the request is not from the threading library
	{
		while((!(curr_block->is_free) && (curr_block->next != NULL)) && ((void*)curr_block < (void*)(mem + LIBPAGESTART)))	
			++curr_block;			//go to the next metadata block
	}
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
	if(size <= PAGE_SIZE)
	{
		curr_block->is_free = 0;
		//curr_block->size = size;
		return_value = curr_block->alloc_start;
	}
  

	/****************************************
 	* if the requested block of memory is	*
 	* larger then the current size		*
 	* create a new block memory by 		*
 	* splitting up the existing block	*
 	* **************************************/
	else if(size > PAGE_SIZE)	//if the size of the current block of memory is larger then requested
	{
		int num_page_need = 0;					//how many pages are needed to store memory request
		int num_page_found = 0;					//how many consequtive pages found
		if(size % PAGE_SIZE == 0)				//if the size requested is a multiple of page size
			num_page_need = size/PAGE_SIZE;			//the number of pages needed is size requested divided by the page size
		else							//if the size requested is not a multiple of page size
			num_page_need = (size/PAGE_SIZE) + 1;		//the number of pages needed is the size reqested divided by the page size + 1
		mem_block *saved_block = NULL;				//temporary storage of block
		//searching for a consequtive blocks that can store the allocation
		while(curr_block->next != NULL)				//while not at the end of the list
		{
			if(num_page_need == num_page_found)		//if the number of free pages found is equal to the number of free pages need
				break;
			if(curr_block->is_free && !num_page_found)	//if the current block is free and no free pages have yet been found
			{
				saved_block = curr_block;		//save the current block
				++num_page_found;			//increase the num_page_found variable
			}
			else if(curr_block->is_free && num_page_found)	//if current block is free and the number of pages is greater then zero
				++num_page_found;			//increase the num_page_found varaible
			else if(!curr_block->is_free && num_page_found)	//if the current block is not free and the num_page_found is greater then zero
				num_page_found = 0;			//reset num_page_found variable
			++curr_block;					//increase the curr_block position
		}
		if(num_page_need == num_page_found)			//if the needed amount of blocks have been found
			return_value = saved_block->alloc_start;		//save the return value
		else							//if the needed amount of block have not been found
		{
			fprintf(stderr, "cannot find the amount of memory needed\n");
			return_value = NULL;				//return NULL
		}
		int i = 0;
		for(i; i <= num_page_need; i++)				//set the link of pages to the next page linked page
		{
			saved_block->next_page = saved_block->next;
			saved_block->is_free = 0;
			++saved_block;
		}
		/*
		mem_block *best_fit = curr_block;			//keep saving the best fit found so far
		int best_fit_size = curr_block->size;			//save the best fitting size found so far
		for(curr_block; curr_block->next != NULL; ++curr_block)
		{
			if(curr_block->is_free && curr_block->size >= size && curr_block->size < best_fit_size)	//if the current block size is greator or equal and current block size is smaller then best fit so far and the current block is free
			{
				best_fit = curr_block;						//current block becomes new best fit
				if(best_fit->size == size)					//if the best fit is equal to the size requested the best fit is found
					break;							//break out
				best_fit_size = curr_block->size;				//best fit size is the current size
			}
		}
		if(best_fit_size != size)							//block only need to be split if there is a delta between size of block and requested size
			best_fit = mem_split(best_fit, size);
		return_value = best_fit->alloc_start;
		*/
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

/****************************************
 * this function will split a block of	*
 * memory that is too large into	*
 * smaller pieces. a check is needed to	*
 * see if the next meta data slot is 	*
 * occupied. if it is occoupied shift	*
 * all block down one slot and then	*
 * create new block in newly open spot	*
 * if next slot is unocupied create	*
 * new metadata in new open spot	*
 * *************************************/
mem_block* mem_split(mem_block *best_fit, size_t size)
{
	mem_block *next_block = NULL;

	if(best_fit->next == NULL || !(best_fit->next->size))					//if the next metadata slot is not initialized or initialized but empty
	{
		next_block = (void*)(best_fit + 1);		//set up a new metadata area one block farther
		next_block->is_free = 1;			//set the new block of memory to available
		next_block->next = best_fit->next;		//set the mew metadata area to point to the next meta data area
		next_block->size = best_fit->size - size;	//set the new block size
		next_block->alloc_start = (void*)(best_fit->alloc_start + size);//set the new metadata block to point to the new allocatable memory
		best_fit->size = size;				//set the current block to the size of the requested block
		best_fit->is_free = 0;				//set the free flag to not free
		best_fit->next = next_block;			//set the current metadata to point to the next block
	}
	else							//if the next metadata slot is already occupied
	{
		mem_block* saved_block = best_fit;		//to save the current position so we can go back later
		while(best_fit->next != NULL)			//go to end of metadata list
			++best_fit;
		while(best_fit != saved_block)
		{
			next_block = (void*)(best_fit + 1);				//the new metablock will be used to transfer the previous postion into new block
			next_block->is_free = best_fit->is_free;			//copy over next if block is free
			if(best_fit->next == NULL)					//if the current block next points to NULL copy over NULL
				next_block->next = best_fit->next;
			else								//if the current block next point to next block copy over two positions away
				next_block->next = best_fit->next->next;		
			next_block->size = best_fit->size;				//copy over size info
			next_block->alloc_start = (void*)(best_fit->alloc_start);	//copy over allocatable mem location
			best_fit->next = next_block;
			--best_fit;
		}
		next_block = (void*)(best_fit + 1);		//set up a new metadata are one block farther
		next_block->is_free = 1;			//set the new block of memory to available
		next_block->next = best_fit->next->next;	//copy over next block info
		next_block->size = best_fit->size - size;	//set the new block size
		next_block->alloc_start = (void*)(best_fit->alloc_start + size);//set the new metadata block to poitn to the new allocatable memory
		best_fit->is_free = 0;			//set the free flag to not free
		best_fit->next = next_block;			//set the current meatadta to point to the next block
	}
	return best_fit;
}


void myfree(void * index, char * file, int line, int request) {

  //check if the given index is a valid pointer
  if (index < (void*)mem || (void*)(mem + TOTSIZE) <= index) {
    fprintf(stderr, "ERROR: Can only free a valid pointer\n\tFile: %s\n\t%d\n", file, line);
    return;
  }

	mem_block *curr_block = meta_ptr, *prev_block = NULL;	//set a pointer to search through metadata block
	
	/****************************************
 	* search through each block until the   *
 	* end of the metadata area is found or  *
 	* the block being searched for is found *
 	* **************************************/
	while(curr_block->alloc_start != index && (void*)curr_block < meta_ptr->alloc_start)
		curr_block++;

	/****************************************
 	* check to see if the index has been	*
 	* found if it has not been found report	*
 	* error and return			*
 	* **************************************/
	if(curr_block->alloc_start != index)
	{
		fprintf(stderr, "ERROR: Pointer is not allocated \n\tFile: %s\n\tLine: %d\n", file, line);
		return;
	}

	curr_block->is_free = 1;		//the searched for block has been found so set to free
	while(curr_block->next_page != NULL)	//reset the next page to NULL
		curr_block++->next_page = NULL;
	
	/***************************************
 	* check to see if the sorrounding block*
 	* are free and join if they are        *
 	* *************************************/
	/*
	curr_block = meta_ptr;			//set current block to the beginning of metadata to search through entire list
	while(curr_block->next != NULL)			//while the next metadata block is not null
	{
		if(curr_block->is_free && curr_block->next->is_free)	//if current block is free and next block is free
		{
			curr_block->size += curr_block->next->size;	//combine the sizes of the two blocks
			if(curr_block->next->next == NULL)		//if the next blocks next is null the next block is the end and we only to combine the two blocks
			{
				curr_block->next = curr_block->next->next;	//set the current block next to null
				break;					//break out of loop
			}
			prev_block = curr_block;			//set the prev block equal to the current block
			curr_block = curr_block->next->next;		//advance the current block to two blocks ahead
			prev_block->next = curr_block; 			//have the freed block skip a block and point to two blocks away
			continue;					//continue with loop
		}
		curr_block = curr_block->next;				//if the current and next blocks were not free keep searching
	} 	
	*/
	return;
  /*
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
  */
  /*
  // report if the pointer is not an allocated block
  if (ptr >= (void*)(mem + 20000)) {
    fprintf(stderr, "ERROR: Pointer is not allocated \n\tFile: %s\n\tLine: %d\n", file, line);
    return;
  }
  */
  /*
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
  */
}
