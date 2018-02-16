#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void * foo(void* arg)
{
	printf("yay threads\n");
	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int x = 5, i = 0;
	my_pthread_t thread[x];
	for (i = 0; i < x; i++) {
		my_pthread_create(&thread[i], NULL, &foo, NULL);
	}

	for (i = 0; i < x; i++) {
		my_pthread_join(thread[i], NULL);	
        }

	return 0;
}
