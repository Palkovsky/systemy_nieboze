# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11
TEST_DIR = test_dir
NAME_1 = variant_1
NAME_2 = variant_2

all: $(NAME_1).out $(NAME_2).out

$(NAME_1).out: $(NAME_1).o
	$(CC) -o $(NAME_1).out $^

$(NAME_2).out: $(NAME_2).o
	$(CC) -o $(NAME_2).out $^

$(NAME_1).o: $(NAME_1).c
	$(CC) -c -o $@ $<

$(NAME_2).o: $(NAME_2).c
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
	done
