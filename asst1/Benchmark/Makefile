CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal

parallelCal: ../my_pthread.o parallelCal.c
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c ../my_pthread.o

vectorMultiply: ../my_pthread.o vectorMultiply.c
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c ../my_pthread.o

externalCal: ../my_pthread.o externalCal.c
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c ../my_pthread.o

clean:
	rm -rf parallelCal vectorMultiply externalCal *.o ./record/
