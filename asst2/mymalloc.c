#include <stdio.h>

// These two macros are for accessing the size metadata and allocated metadata of a pointer
// associated with a block in our memory which are stored in the first 4 bytes of eqch block
#define size(ptr) *((short*)(ptr))
#define allocd(ptr) *((short*)((ptr) + 2))

// Here is our block of memory which can hold up to 4000 single byte allocations with four
// four bytes of metadata each, or a single allocation of up to 19996 bytes
static char mem[20000];


void * mymalloc(size_t size, char * file, int line) {

  static short firstmalloc = 1;

  // check if this is the first allocation of memory and set up initial metadata
  if (firstmalloc) {
    size(mem) = 19996; 
    allocd(mem) = 0; 
    firstmalloc = 0;
    
  }

  // check if size is larger than total mem - metadata 
  if (size > 19996) {
    fprintf(stderr, "ERROR: Not Enough Memmory\n\tFile: %s\n\tLine: %d\n", file, line);
    return NULL;
  }
  
  // go through the "linked list" of blocks until we find an available block big enough, if we hit the end,
  // return NULL and report the lack of space
  void * ptr = mem;
  while (ptr < (void*)(mem + 20000)) {
    // a block must be either exactly the right size or have enough extra space to accomodate the 
    // metadata for the free block containing the leftover space 
    if (!allocd(ptr) && (size(ptr) == size || size(ptr) >= size + 4)) {
      break;
    }
    ptr += size(ptr) + 4;
  }

  if (ptr >= (void*)(mem + 20000)) {
    fprintf(stderr, "ERROR: Not Enough Memory\n\tFile: %s\n\tLine: %d\n", file, line);
    return NULL;
  }

  // if a suitable block is found and there is leftover space, divide it into an allocated and free block, 
  // otherwise, simply mark the block as allocated, then return a pointer to the allocated block
  if (size(ptr) > size) {
    short leftover = size(ptr) - size - 4;
    size(ptr) = size;

    size(ptr + size + 4) = leftover;
    allocd(ptr + size + 4) = 0;
  }

  allocd(ptr) = 1;

  return ptr + 4;


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
