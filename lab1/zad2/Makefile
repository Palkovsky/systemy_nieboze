# Makefile witchcraft example:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11

LIB_NAME = my_library
NAME = main

all: $(NAME).o $(LIB_NAME).o
	$(CC) -o $(NAME).out $^

# Generating library object file
$(NAME).o: $(NAME).c
	$(CC) -c -o $@ $<

# Generating library object file
$(LIB_NAME).o: $(LIB_NAME).c $(LIB_NAME).h
	$(CC) -c -o $@ $<

# Remove all object files, static libraries and shared libraries
clean:
	rm -f *.o *.a *.so.* *.temp

test_simple: all
	./main.out 10 search_directory ~/host/FIND_TEST/ \\* file.temp move_to_mem remove_block 0
	make clean

test_complex: all
	./main.out 10 \
	search_directory /bin/ \\* file.temp \
	move_to_mem \
	search_directory /usr dif\\* file.temp \
	move_to_mem \
	search_directory /usr \\*mac\\* file.temp \
	remove_block 0 \
	move_to_mem \
	search_directory /bin/ vim file.temp \
	remove_block 0 \
	remove_block 1 \
	search_directory /sys vim file.temp \
	move_to_mem
	make clean
