default: simple_threads test

simple_threads: my_pthread.o simple_threads.o
	gcc -g my_pthread.o simple_threads.o -o simple_threads

test: my_pthread.o test.o
	gcc -g my_pthread.o test.o -o test

simple_threads.o: simple_threads.c
	gcc -g -c simple_threads.c

test.o: test.c
	gcc -g -c test.c

my_pthread.o:
	gcc -g -c my_pthread.c

clean:
	rm *.o test simple_threads
