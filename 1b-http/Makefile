CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c

all: client server

client: client.o
	$(CC) -o client client.o
	chmod +x client

client.o: client.c
	$(CC) $(CFLAGS) client.c

server: server.o
	$(CC) -o server server.o
	chmod +x server

server.o: server.c
	$(CC) $(CFLAGS) server.c

clean:
	$(RM) client server
