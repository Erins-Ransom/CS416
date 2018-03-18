#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

int main()
{
	int *a = (int*) malloc(10 * sizeof(int));
	int *b = (int*) malloc(5 * sizeof(int));
	free(a);
	int *c = (int*) malloc(5 * sizeof(int));
	char *d = (char*) malloc(6000 * sizeof(char));
	free(b);
	free(c);
	free(d);
	printf("malloc has been tested\n");
}
