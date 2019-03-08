# Makefile witchcraft example:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -std=gnu11


NAME = main
LIB_NAME = my_library
LIB_SONAME = lib$(LIB_NAME).so

all:
	make static
	make all_tests > static.txt
	make shared
	make all_tests > shared.txt
	make dynamic
	make all_tests > dynamic.txt

static:
# compiling my_library
	$(CC) -c -o $(LIB_NAME).o $(LIB_NAME).c
	ar rcs lib$(LIB_NAME).a $(LIB_NAME).o

# compiling main.c
	$(CC) -c -o $(NAME).o $(NAME).c -D REPORT_FILENAME=\"/dev/null\"

	$(CC) -o $(NAME).out $(NAME).o lib$(LIB_NAME).a

shared:
# compiling my_library
	$(CC) -fPIC -c -o $(LIB_NAME).o $(LIB_NAME).c
	$(CC) -fPIC -shared -Wl,-soname,$(LIB_SONAME) -o $(LIB_SONAME) $(LIB_NAME).o -lc

# compiling main.c to main.o
	$(CC) -c -o $(NAME).o $(NAME).c  -D REPORT_FILENAME=\"/dev/null\"

	$(CC) -L. -Wl,-rpath=. -o $(NAME).out $(NAME).o -l$(LIB_NAME)

dynamic:
# compiling my_library
	$(CC) -fPIC -c -o $(LIB_NAME).o $(LIB_NAME).c
	$(CC) -fPIC -shared -Wl,-soname,$(LIB_SONAME) -o $(LIB_SONAME) $(LIB_NAME).o -lc

# compiling main.c to main.o
	$(CC) -c -o $(NAME).o $(NAME).c -D DLL=\"$(LIB_SONAME)\" -D REPORT_FILENAME=\"/dev/null\"

	$(CC) -L.  -Wl,-rpath=. -o $(NAME).out $(NAME).o -ldl


# Remove all object files, static libraries and shared libraries
clean:
	rm -f *.o *.a *.so *.temp *.out *.rap

# Tests for different sizes of directories
all_tests: test_simple_13k test_simple_25k test_simple_70k test_complex

test_simple_70k:
	./$(NAME).out 10 \
	search_directory /usr/ \\* file.temp \
	move_to_mem \
	remove_block 0

test_simple_25k:
	./$(NAME).out 10 \
	search_directory /usr/src/ \\* file.temp \
	move_to_mem \
	remove_block 0

test_simple_13k:
	./$(NAME).out 10 \
	search_directory /usr/lib/ \\*.so file.temp \
	move_to_mem \
	remove_block 0

test_complex:
	./$(NAME).out 10 \
	search_directory /bin/ \\* file.temp \
	move_to_mem \
	search_directory /usr dif\\* file.temp \
	move_to_mem \
	search_directory /usr \\*mac\\* file.temp \
	remove_block 0 \
	move_to_mem \
	search_directory /usr/lib vim file.temp \
	remove_block 0 \
	remove_block 1 \
	search_directory /usr vim file.temp \
	move_to_mem
