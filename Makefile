all: receiver sender

receiver: src/receiver/receiver.c create_socket.o gbnHelper.o
	gcc -o receiver src/receiver/receiver.c create_socket.o gbnHelper.o -lz

sender: src/sender/sender.c sender_help.o packet_interface.o real_address.o create_socket.o
	gcc -o sender src/sender/sender.c sender_help.o packet_interface.o real_address.o create_socket.o -lz

tests: tests/test_no_link.sh tests/test_link.sh FORCE
	./tests/test_link.sh
	./tests/test_no_link.sh

packet_interface.o: src/shared/packet_interface.c
	gcc -c src/shared/packet_interface.c

real_address.o: src/shared/real_address.c
	gcc -c src/shared/real_address.c

wait_for_client.o: src/shared/wait_for_client.c
	gcc -c src/shared/wait_for_client.c

create_socket.o: src/shared/create_socket.c
	gcc -c src/shared/create_socket.c

gbnHelper.o: src/receiver/gbnHelper.c packet_interface.o real_address.o wait_for_client.o
	gcc -c src/receiver/gbnHelper.c packet_interface.o

sender_help.o: src/sender/sender_help.c
	gcc -c src/sender/sender_help.c

clean: FORCE
	rm *.o

clear: clean FORCE
	rm sender receiver

FORCE:
