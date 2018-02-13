default: my_pthread.o test.o
	gcc -g my_pthread.o test.o -o test

my_pthread.o:
	gcc -g -c my_pthread.c

test.o:
	gcc -g -c test.c

clean:
	rm test
