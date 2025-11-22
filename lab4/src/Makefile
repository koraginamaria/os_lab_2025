CC=gcc
CFLAGS=-I. -Wall -Wextra
TARGETS=parallel_min_max process_memory parallel_sum

all: $(TARGETS)

parallel_sum: utils.o sum_utils.o parallel_sum.c
	$(CC) -o parallel_sum utils.o sum_utils.o parallel_sum.c $(CFLAGS) -lpthread

parallel_min_max: utils.o find_min_max.o parallel_min_max.c
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

sum_utils.o: sum_utils.c sum_utils.h
	$(CC) -o sum_utils.o -c sum_utils.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h utils.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f *.o $(TARGETS) parallel_sum
