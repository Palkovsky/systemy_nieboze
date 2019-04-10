# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash

CC = gcc -Wall -pedantic -std=gnu11
SERVER = server
CLIENT = client

all: $(SERVER).out $(CLIENT).out

$(SERVER).out: $(SERVER).o
	$(CC) -o $(SERVER).out $^

$(SERVER).o: $(SERVER).c
	$(CC) -c -o $@ $<

$(CLIENT).out: $(CLIENT).o
	$(CC) -o $(CLIENT).out $^

$(CLIENT).o: $(CLIENT).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp
