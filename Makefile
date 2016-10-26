all:
	if [ ! -d "build" ]; then mkdir build; fi
	cd build && cmake ..
	cd build && make all
	cp build/receiver receiver
	cp build/sender sender

tests: FORCE
	cd build && make test

clear: FORCE
	if [ -d "build" ]; then rm -r build; fi
	rm sender
	rm receiver

FORCE :
