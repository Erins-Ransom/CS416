#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

int main()
{
	int *a = (int*) malloc(10 * sizeof(int));
	int *b = (int*) malloc(5 * sizeof(int));
	free(a);
	int *c = (int*) malloc(5 * sizeof(int));
	free(b);
	free(c);
	printf("malloc has been tested\n");
}
