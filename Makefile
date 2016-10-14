#Flags for GCC
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -O2 -D_FORTIFY_SOURCE=2
CFLAGS += -fstack-protector-all
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE

CLIB += -lz
CLIB += -lcunit

default: sender.o receiver.o
	gcc $(CFLAGS) -o sender src/sender.o $(CLIB)
	gcc $(CFLAGS) -o receiver src/receiver.o $(CLIB)

tests: sender receiver tests.o
	gcc $(CFLAGS) $L(HOME)/local/lib -o tests_ex tests.o $(CLIB)
	./tests_ex

compile:
	gcc $(CFLAGS) -c src/sender.c $(CLIB)
	gcc $(CFLAGS) -I$(HOME)/local/include -c tests/tests.c $(CLIB)

.PHONY: clean rebuild

clean:
	rm *.o tests_ex sender receiver

rebuild: clean compile
