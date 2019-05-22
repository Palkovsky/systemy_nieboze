# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11

all: main.out

main.out: main.c data.o
	$(CC) -c $<  -lpthread
	$(CC) -o $@ data.o main.o -lpthread

data.o: data.c data.h
	$(CC) -c $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp
