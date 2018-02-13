#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"
#define MAXTHREADS 1000

/********************************************
 * a simple struct that will keep track of 
 * start and end index of number searched
 * for prime numbers
*********************************************/
struct arguments
{
	int start;
	int end;
};

/*********************************************
 * function to find prime numbers by checking
 * if a position in the index is evenly
 * divisible by another number
*********************************************/
void *prime_num_gen(void* arguments)		//prime_num_gen will calculate the number of prime numbes between x and y
{
	struct arguments *args = (struct arguments*)arguments;		//cast the typless arguments pointer to a arguments struct
	if(args->start > args->end)					//check to see if number was input in reverse order
	{
		printf("number were input in reverse order\n");
		pthread_exit(NULL);
	}
	int x = args->start, y = args->end;				//save struct info to a automatic variable
	int num_prime_num = 0;						//the number of prime numbers found
	int flag = 0;							//zero flag indicates a prime number
		
	for(x; x <= y; x++)		//repeat loop for the number of y -x times
	{
		int i = 2;		//loop counter
		for(i ; i < x; i++)	//repeat the loop for x - 2 times
		{
			if((x % i) == 0)	//if x is evenly divisible
			{	
				flag = 1;	//if a number is found to be evenly divisible set flag to 1
				break;		//once number is found to be not prime get out of loop
			}
		}
		if(flag == 0)			//if no prime number found add one to counter
			num_prime_num++;
		flag = 0;			//reset flag for next loop
	}
	printf("I have found %d prime numbers\n", num_prime_num);	//print results
	pthread_exit(NULL);
}

/****************************************
 * function used to calculate the sum of
 * a geometric series of the form a + ar
 * + ar^2 + ar^3...ar^n; a != 0
****************************************/ 
void *geometric_sum()
{
	int upper_limit = 20, sum_index = 0, common_ratio = 1, r = 2, result = 0;

	for(sum_index; sum_index < upper_limit; sum_index++)
	{
		result += (common_ratio * my_pow(r, sum_index));
	}
	
	printf("the result of the geometric series is %d\n", result);
	pthread_exit(NULL);
}

/******************************************
 * my funciton to calculate the exponential
 * function. The built in function demands
 * doubles and I was to use ints. As a
 * result powers to anything other then an
 * int is possible. x is rasied to power y
******************************************/
int my_pow(int x, int y)		//my function to calculate the exponential function (I didn't want to deal with doubles)
{
	if(y == 0)
		return 1;
	int calc_num = 1, i;		//the result of the function and loop counter
	for(i = 1; i <= y; i++)
		calc_num *= x;		
	return calc_num;
}

int main()
{
	my_pthread_t prime_threads[MAXTHREADS];		//for pthread create to keep track of TIDs that determine prime numbers
	my_pthread_t geo_threads[MAXTHREADS];		//for pthread create to keep track of TIDs that calculate goemetric series
	
	struct arguments args;
	args.start = 1;				//starting index to search for prime numbers
	args.end = 25;				//ending incex to search for prime number
	
	/*	
	int i = 0;
	for(i ; i < 100; i++)
	{
		pthread_create(&prime_threads[i], NULL, &prime_num_gen, (void*)&args);	//call function in a new thread
		args.start += 5;		//increase start and end index
		args.end += 200;		//thse lines can (will) cause synchonization problems
	}
	*/

	my_pthread_create(&geo_threads[0], NULL, &geometric_sum, NULL); //call function in a new thread
	my_pthread_join(geo_threads[0], NULL);

	/*
	i = 0;
	for(i ; i < 100; i++)
		my_pthread_join(prime_threads[i], NULL);
	*/
}

