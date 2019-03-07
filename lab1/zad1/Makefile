# Makefile witchcraft example:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11

VERSION = 0
RELEASE = 0.1
NAME = my_library
SONAME = lib$(NAME).so.$(VERSION).$(RELEASE)

all: shared static

# Generate shared library
shared: $(NAME)_pic.o
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(SONAME) $^ -lc

# Generate static library
static: $(NAME).o
	ar rcs lib$(NAME).a $^

# Generating object file
$(NAME).o: $(NAME).c $(NAME).h
	$(CC) -c -o $@ $(NAME).c

# Generating position indepented object file
$(NAME)_pic.o: $(NAME).c $(NAME).h
	$(CC) -fPIC -c -o $@ $(NAME).c

# Remove all object files, static libraries and shared libraries
clean:
	rm -f *.o *.a *.so.* *.out *.temp
