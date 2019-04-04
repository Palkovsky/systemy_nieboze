# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash

CC = gcc -Wall -pedantic -std=gnu11
NAME_1 = master
NAME_2 = slave

all: $(NAME_1).out $(NAME_2).out

$(NAME_1).out: $(NAME_1).o
	$(CC) -o $(NAME_1).out $^

$(NAME_1).o: $(NAME_1).c
	$(CC) -c -o $@ $<

$(NAME_2).out: $(NAME_2).o
	$(CC) -o $(NAME_2).out $^

$(NAME_2).o: $(NAME_2).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp
