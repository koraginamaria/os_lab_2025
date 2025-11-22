CC=gcc
CFLAGS=-I. -Wall -Wextra -pthread
TARGETS=deadlock_demo parallel_factorial

all: $(TARGETS)

parallel_factorial: parallel_factorial.c
	$(CC) -o parallel_factorial parallel_factorial.c $(CFLAGS)

deadlock_demo: deadlock_demo.c
	$(CC) -o deadlock_demo deadlock_demo.c $(CFLAGS)

clean:
	rm -f $(TARGETS)
