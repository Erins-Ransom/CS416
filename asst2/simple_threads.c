#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void * foo(void* arg)
{
	void* ptr1 = page_malloc(8192, __FILE__, __LINE__, 1);
	sprintf( (char*)(ptr1+2000), "yay threads\n");
	printf("%s", (char*)(ptr1+2000));

	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int x = 3, i = 0;
	my_pthread_t threads[x];
	for (i = 0; i < x; i++) {
		my_pthread_create(&threads[i], NULL, &foo, NULL);
	}

	for (i = 0; i < x; i++) {
		my_pthread_join(threads[i], NULL);	
        }

	return 0;
}
