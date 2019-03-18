# Makefile witchcraft:
# app: lib.c main.c
# $@ ==> app
# $< ==> lib.c
# $^ ==> lib.c main.c

CC = gcc -Wall -pedantic -std=gnu11
NAME = main
REPORT = raport.txt

all: $(NAME).o
	$(CC) -o $(NAME).out $^

$(NAME).o: $(NAME).c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.a *.so.* *.out *.temp

test: all
	echo  > $(REPORT)

	for block_size in 1 4 512 1024 4096 8192 ; do \
		make test_cpy BS=$$block_size RC=500000; \
		make test_cpy BS=$$block_size RC=1000000; \
	done

	for block_size in 1 4 512 1024 4096 8192 ; do \
		make test_sort BS=$$block_size RC=400; \
		make test_sort BS=$$block_size RC=800; \
	done

	make clean

test_cpy:
	./main.out generate bytes.temp 1 $(RC) >> /dev/null # Generate RC bytes
	echo "============== COPY LIB Buffer Size=$(BS), Bytes=$(RC) ==============" >> $(REPORT)
	./main.out copy bytes.temp cpy_lib.temp $(RC) $(BS) lib >> $(REPORT) # Copy RC bytes with buffer of BS using lib function
	echo "\n" >> $(REPORT)
	echo "============== COPY SYS Buffer Size=$(BS), Bytes=$(RC) ==============" >> $(REPORT)
	./main.out copy bytes.temp cpy_sys.temp $(RC) $(BS) sys >> $(REPORT) # Copy RC bytes with buffer of BS using lib function
	echo "\n" >> $(REPORT)

test_sort:
	./main.out generate bytes.temp $(RC) $(BS) >> /dev/null
	./main.out copy bytes.temp cpy.temp $(RC) $(BS) lib >> /dev/null
	echo "============== SORT LIB Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out sort bytes.temp $(RC) $(BS) lib >> $(REPORT)
	echo "\n" >> $(REPORT)
	echo "============== SORT SYS Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out sort cpy.temp $(RC) $(BS) sys >> $(REPORT)
	echo "\n" >> $(REPORT)
