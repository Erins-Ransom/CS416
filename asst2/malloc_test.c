#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

int main()
{
	int *p = (int*) malloc(5 * sizeof(int));
	char *c = (char*) malloc(100 * sizeof(char));
	*(p) = 1;
	free(p);
	double *d = (double*) malloc(30 * sizeof(double));
	free (d);
	free(c);
	printf("malloc has been tested\n");
}
