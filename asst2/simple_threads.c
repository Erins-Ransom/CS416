#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

#define ALLOC_SIZE 1024000

void * foo(void* arg)
{
	char * ptr = malloc(ALLOC_SIZE);
	int i; 
	for(i = 0; i < ALLOC_SIZE; i++) {
		ptr[i] = 'd';
	}
	printf("yay threads\n");

	while(1);

	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int x = 30, i = 0;
	my_pthread_t threads[x];
	for (i = 0; i < x; i++) {
		my_pthread_create(&threads[i], NULL, &foo, NULL);
	}

	for (i = 0; i < x; i++) {
		my_pthread_join(threads[i], NULL);	
        }

	return 0;
}
