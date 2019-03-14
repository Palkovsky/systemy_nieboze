# Makefile witchcraft example:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11
NAME = main

all: $(NAME).o
	$(CC) -o $(NAME).out $^

$(NAME).o: $(NAME).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp
