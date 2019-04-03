# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash

CC = gcc -Wall -pedantic -std=gnu11
TEST_DIR  = test_dir
LIST_FILE = data.lst
NAME = main

all: $(NAME).out

$(NAME).out: $(NAME).o
	$(CC) -o $(NAME).out $^

$(NAME).o: $(NAME).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp

generate_list:
	echo 'ls -la | grep main'   >  $(LIST_FILE)
	echo 'ls -la | grep main | grep main | grep main | grep main' >> $(LIST_FILE)
