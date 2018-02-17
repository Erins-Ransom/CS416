#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_pthread_t.h"

static my_pthread_mutex_t MUTEX;

void * foo(void* arg)
{
	my_pthread_mutex_lock(&MUTEX);

	FILE* fd = fopen("shared.txt", "a+");
	int i;
	for(i = 0; i < 10000; i++) {
		fprintf(fd, "AAA ");
	}

	my_pthread_mutex_unlock(&MUTEX);
	
	pthread_exit(NULL);
}

void * bar(void* arg)
{       
	my_pthread_mutex_lock(&MUTEX);
        
	FILE* fd = fopen("shared.txt", "a+");
       	int i;
        for(i = 0; i < 10000; i++) {
                fprintf(fd, "ZZZ ");
        }
 
        my_pthread_mutex_unlock(&MUTEX);
        
        pthread_exit(NULL);
}

int main(int argc, char** argv)
{
	int x = 2, i = 0;
	my_pthread_t threads[x];

	my_pthread_mutex_init(&MUTEX, NULL);

	my_pthread_create(&threads[0], NULL, &foo, NULL);	
	my_pthread_create(&threads[1], NULL, &bar, NULL);

	my_pthread_join(threads[0], NULL);
        my_pthread_join(threads[1], NULL);

	my_pthread_mutex_destroy(&MUTEX);

	return 0;
}
