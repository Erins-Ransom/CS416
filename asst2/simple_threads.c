#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void * foo(void* arg)
{
	char * ptr1 = malloc(12);
	sprintf( ptr1 , "yay threads\n");
	printf("%s", ptr1);

	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int x = 20, i = 0;
	my_pthread_t threads[x];
	for (i = 0; i < x; i++) {
		my_pthread_create(&threads[i], NULL, &foo, NULL);
	}

	for (i = 0; i < x; i++) {
		my_pthread_join(threads[i], NULL);	
        }

	return 0;
}
