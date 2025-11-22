CC=gcc
CFLAGS=-I.

all: sequiential_min_max parallel_min_max run_sequential

sequiential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequiential_min_max utils.o find_min_max.o sequiential_min_max.c $(CFLAGS)

parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

run_sequential: sequiential_min_max
	$(CC) -o run_sequential run_sequential.c $(CFLAGS)

utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o sequiential_min_max parallel_min_max run_sequential