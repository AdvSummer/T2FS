LIB=../lib/
INC=../include/

CC=gcc
CCFLAGS=-m32 -Wall -I$(INC) -g
LDFLAGS=-L$(LIB) -lt2fs

all: shell.c test.c
	$(CC) $(CCFLAGS) -o shell shell.c $(LDFLAGS)
	$(CC) $(CCFLAGS) -o test test.c $(LDFLAGS)

clean:
	find -type f ! -name '*.c' ! -name 'Makefile' -delete
