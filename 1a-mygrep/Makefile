CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g -c

all: mygrep

mygrep: mygrep.o
	$(CC) -o mygrep mygrep.o
	chmod +x mygrep

mygrep.o: mygrep.c clean
	$(CC) $(CFLAGS) mygrep.c

clean:
	$(RM) mygrep
