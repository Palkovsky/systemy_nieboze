# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

SHELL = /bin/bash

CC = gcc -Wall -pedantic -std=gnu11
TEST_DIR = test_dir
NAME = main

all: $(NAME).out

$(NAME).out: $(NAME).o
	$(CC) -o $(NAME).out $^

$(NAME).o: $(NAME).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp
	rm -rf $(TEST_DIR)

generate_test_directory:
	rm -rf $(TEST_DIR)
	mkdir $(TEST_DIR)
	cd $(TEST_DIR)

	for number in 1 2 3 4 5 6 7 8 9 10; do \
		touch $(TEST_DIR)/$$number.txt ; \
		mkdir $(TEST_DIR)/$$number ; \
	done

	num1=1 ; while [[ $$num1 -le 10 ]] ; do \
		num2=1 ; while [[ $$num2 -le 5 ]] ; do \
			touch $(TEST_DIR)/$$num1/$$num2.txt ; \
			ln -s  $(TEST_DIR)/$$num1  $(TEST_DIR)/$$num1/$$num2.link ; \
			((num2 = num2 + 1)) ; \
		done ; \
		((num1 = num1 + 1)) ; \
	done
