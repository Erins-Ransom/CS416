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
	my_pthread_t foo_thread;
	my_pthread_create(&foo_thread, NULL, &foo, NULL);
	my_pthread_join(foo_thread, NULL);	
	return 0;
}
