#Flags for GCC
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wshadow
CFLAGS += -Wextra
CFLAGS += -O2 -D_FORTIFY_SOURCE=2
CFLAGS += -fstack-protector-all
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE

CLIB += -lz
CLIB += -L$(HOME)/include/lib
CLIB += -lcunit

defaul: sender.o receiver.o sender_helper.o
	gcc $(CFLAGS) -o sender sender.o sender_help.o $(CLIB)
	gcc $(CFLAGS) -o receiver src/receiver.o $(CLIB)

tests: sender_help.o tests.o real_address.o create_socket.o packet_interface.o queue.o
	gcc $(CFLAGS) -o tests_ex tests.o sender_help.o real_address.o create_socket.o packet_interface.o queue.o $(CLIB)
	./tests_ex

tests.o:
	gcc $(CFLAGS) -I$(HOME)/local/include -c tests/tests.c $(CLIB)
sender_help.o:
	gcc $(CLFAGS) -c src/sender_help.c $(CLIB)

sender.o:
	gcc $(CFLAGS) -c src/sender.c $(CLIB)

real_address.o:
	gcc $(CFLAGS) -c src/real_address.c $(CLIB)

create_socket.o:
	gcc $(CFLAGS) -c src/create_socket.c $(CLIB)

packet_interface.o:
	gcc $(CFLAGS) -c src/packet_interface.c $(CLIB)

queue.o:
	gcc $(CFLAGS) -c src/queue.c
.PHONY: clean 

clean:
	rm *.o tests_ex

