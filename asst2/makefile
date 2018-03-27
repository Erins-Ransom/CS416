default: simple_threads malloc_threads

simple_threads: my_pthread.o simple_threads.o
	gcc -g my_pthread.o simple_threads.o -o simple_threads -lrt

malloc_threads: my_pthread.o malloc_threads.o
	gcc -g -lrt my_pthread.o malloc_threads.o -o malloc_threads -lrt

simple_threads.o: simple_threads.c
	gcc -g -c simple_threads.c

malloc_threads.o: malloc_threads.c
	gcc -g -c malloc_threads.c

my_pthread.o: my_pthread.c
	gcc -g -c  my_pthread.c -lrt

clean:
	rm *.o simple_threads malloc_threads
