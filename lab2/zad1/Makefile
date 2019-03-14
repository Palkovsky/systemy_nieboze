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

raport_all: all
	echo "" > $(REPORT)
	make raport BS=1 RC=400
	make raport BS=1 RC=800

	make raport BS=4 RC=400
	make raport BS=4 RC=800

	make raport BS=512 RC=400
	make raport BS=512 RC=800

	make raport BS=1024 RC=400
	make raport BS=1024 RC=800

	make raport BS=4096 RC=400
	make raport BS=4096 RC=800

	make raport BS=8192 RC=400
	make raport BS=8192 RC=800

	make clean

raport:
	./main.out generate bytes.temp $(RC) $(BS) >> /dev/null
	echo "============== COPY LIB Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out copy bytes.temp cpy_lib.temp $(RC) $(BS) lib >> $(REPORT)
	echo "\n" >> $(REPORT)
	echo "============== COPY SYS Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out copy bytes.temp cpy_sys.temp $(RC) $(BS) sys >> $(REPORT)
	echo "\n" >> $(REPORT)
	echo "============== SORT LIB Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out sort cpy_lib.temp $(RC) $(BS) lib >> $(REPORT)
	echo "\n" >> $(REPORT)
	echo "============== SORT SYS Record Size=$(BS), Record Count=$(RC) ==============" >> $(REPORT)
	./main.out sort cpy_sys.temp $(RC) $(BS) sys >> $(REPORT)
	echo "\n" >> $(REPORT)
