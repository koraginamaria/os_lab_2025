CC=gcc
CFLAGS=-I. -Wall -Wextra -pthread
TARGETS=client server
LIBRARY=common

all: $(TARGETS)


client: client.c common.o
	$(CC) -o client client.c common.o $(CFLAGS)


server: server.c common.o
	$(CC) -o server server.c common.o $(CFLAGS)


common.o: common.c common.h
	$(CC) -o common.o -c common.c $(CFLAGS)

clean:
	rm -f $(TARGETS) *.o

