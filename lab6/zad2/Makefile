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
	$(CC) -o $(SERVER).out $^ -lrt

$(SERVER).o: $(SERVER).c
	$(CC) -c -o $@ $< -lrt

$(CLIENT).out: $(CLIENT).o
	$(CC) -o $(CLIENT).out $^ -lrt

$(CLIENT).o: $(CLIENT).c
	$(CC) -c -o $@ $< -lrt

clean:
	rm -f *.o *.a *.so.* *.out *.temp
