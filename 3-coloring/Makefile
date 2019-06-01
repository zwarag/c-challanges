CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c
SEM=-lrt -pthread
#SEM=-pthread

all: supervisor generator

supervisor: supervisor.o 
	$(CC) -o supervisor supervisor.o $(SEM)
	chmod +x supervisor

supervisor.o: clean supervisor.c
	$(CC) $(CFLAGS) supervisor.c

generator: generator.o 
	$(CC) -o generator generator.o $(SEM)
	chmod +x generator 

generator.o: clean generator.c
	$(CC) $(CFLAGS) generator.c

clean:
	$(RM) supervisor supervisor.o generator generator.o
