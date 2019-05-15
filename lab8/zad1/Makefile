# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash
CC = gcc -Wall -pedantic -std=gnu11

all: main.out

main.out: main.c images.o
	$(CC) -c $< -lm -lpthread
	$(CC) -o $@ images.o main.o -lm -lpthread

images.o: images.c images.h
	$(CC) -c $< -lm

clean:
	rm -f *.o *.a *.so.* *.out *.temp
