default: simple_mutex simple_threads test

simple_mutex: my_pthread.o simple_mutex.o
	gcc -g my_pthread.o simple_mutex.o -o simple_mutex

simple_threads: my_pthread.o simple_threads.o
	gcc -g my_pthread.o simple_threads.o -o simple_threads

test: my_pthread.o test.o
	gcc -g my_pthread.o test.o -o test

simple_mutex.o: simple_mutex.c
	gcc -g -c simple_mutex.c

simple_threads.o: simple_threads.c
	gcc -g -c simple_threads.c

test.o: test.c
	gcc -g -c test.c

my_pthread.o: my_pthread.c
	gcc -g -c my_pthread.c

clean:
	rm *.o test simple_threads simple_mutex
