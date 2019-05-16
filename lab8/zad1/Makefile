# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash
CC = gcc -Wall -pedantic -std=gnu11

REPORT = Times.txt
IN_FILE = ../balloons.ascii.pgm
OUT_FILE = ../blured.ascii.pgm
SPLIT_TYPES = block interleaved
FILTERS = ../33.trans ../55.trans ../1515.trans ../3030.trans ../6060.trans
THREADS = 1 2 4 8

all: main.out

main.out: main.c images.o
	$(CC) -c $< -lm -lpthread
	$(CC) -o $@ images.o main.o -lm -lpthread

images.o: images.c images.h
	$(CC) -c $< -lm

test:
	echo > $(REPORT)
	for type in $(SPLIT_TYPES) ; do \
		for filter in $(FILTERS) ; do \
			for threads in $(THREADS) ; do \
				./main.out $$threads $$type $(IN_FILE) $$filter $(OUT_FILE) >> $(REPORT); \
				printf '\n\n' >> $(REPORT); \
			done ; \
		done ; \
	done


clean:
	rm -f *.o *.a *.so.* *.out *.temp
