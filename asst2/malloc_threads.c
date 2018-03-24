#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "my_pthread_t.h"


void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}


void * foo(void* arg)
{
	int i, size = 8000;
	char * buffer = malloc(size);
	buffer[size-1] = '\0';

	for (i=0; i<size-1; i++) {
		buffer[i] = (char)(32 + rand()%94);
	}

	int * array = malloc(size*sizeof(int));

	array[0] = 0;
	array[1] = 1;
	for (i=2; i<size; i++) {
		array[i] = array[i-1] + array[i-2];
	}

	free(buffer);

	int * more_numbers[100];

	for (i=0; i<100; i++) {
		more_numbers[i] = malloc((1 + rand()%100)*sizeof(int));
		*more_numbers[i] = array[i];
//		delay(10);
		fprintf(stdout, "Fib# %d is %d\n", i, *more_numbers[i]);
		fflush(stdout);
	}

	free(array);

	for (i=0; i<100; i++) {
		free(more_numbers[i]);
	}

	pthread_exit(NULL);
}


int main(int argc, char** argv)
{
	int x = 10, i = 0;
	my_pthread_t threads[x];
	for (i = 0; i < x; i++) {
		my_pthread_create(&threads[i], NULL, &foo, NULL);
	}

	for (i = 0; i < x; i++) {
		my_pthread_join(threads[i], NULL);	
        }

	return 0;
}
