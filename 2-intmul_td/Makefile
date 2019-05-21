CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c

all: intmul

intmul: intmul.o
	$(CC) -o intmul intmul.o
	chmod +x intmul

intmul.o: intmul.c
	$(CC) $(CFLAGS) intmul.c

clean:
	$(RM) intmul intmul.o
