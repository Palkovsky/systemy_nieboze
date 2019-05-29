CC = gcc -Wall -pedantic -std=gnu11
LNK = -lpthread

all: client.out server.out
client.out: client.o utils.o
	$(CC) -o $@ $^ $(LNK)
server.out: server.o utils.o
	$(CC) -o $@ $^ $(LNK)
server.o: server.c
	$(CC) -c $< $(LNK)
client.o: client.c
	$(CC) -c $< $(LNK)
utils.o: utils.c utils.h
	$(CC) -c $< $(LNG)

clean:
	rm -f *.out *.o

server: server.out
	rm -f /tmp/xd
	./server.out 2137 /tmp/xd

client: client.out
	./client.out wapiesz NET 2137
