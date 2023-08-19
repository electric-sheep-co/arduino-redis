TESTS = subscriber/subscriber-tests.out publisher/publisher-tests.out

test : $(TESTS)

subscriber/subscriber-tests.out: subscriber/subscriber-tests.ino
	cd subscriber && make
	
publisher/publisher-tests.out: publisher/publisher-tests.ino
	cd publisher && make

run: test
	bash -c 'echo -e "./subscriber/subscriber-tests.out\n./publisher/publisher-tests.out" | parallel --delay=1'

clean:
	cd subscriber && make clean
	cd publisher && make clean
