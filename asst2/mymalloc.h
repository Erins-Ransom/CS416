#ifndef MYMALLOC
#define MYMALLOC

#include <stdio.h>

#define malloc(x) mymalloc(x, __FILE__, __LINE__)
#define free(x) myfree(x, __FILE__, __LINE__)

void * mymalloc(size_t size, char * file, int line);
void myfree(void * index, char * file, int line);


#endif
