LIB=../lib/
INC=../include/

CC=gcc
CCFLAGS=-m32 -Wall -I$(INC)
LDFLAGS=-L$(LIB) -lt2fs

all: shell.c
	$(CC) $(CCFLAGS) -o shell shell.c $(LDFLAGS)

clean:
	find -type f ! -name '*.c' ! -name 'Makefile' -delete
