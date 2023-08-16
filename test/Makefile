TESTS = unit/unit-tests.out integration/integration-tests.out pubsub/subscriber/subscriber-tests.out pubsub/publisher/publisher-tests.out

test : $(TESTS)

unit/unit-tests.out: unit/unit-tests.ino ../Redis.h ../Redis.cpp ../RedisInternal.h ../RedisInternal.cpp
	cd unit && make

integration/integration-tests.out: integration/integration-tests.ino ../Redis.h ../Redis.cpp ../RedisInternal.h ../RedisInternal.cpp
	cd integration && make

pubsub/subscriber/subscriber-tests.out: pubsub/subscriber/subscriber-tests.ino ../Redis.h ../Redis.cpp ../RedisInternal.h ../RedisInternal.cpp
	cd pubsub && make

pubsub/publisher/publisher-tests.out: pubsub/publisher/publisher-tests.ino ../Redis.h ../Redis.cpp ../RedisInternal.h ../RedisInternal.cpp
	cd pubsub && make

run: test pubsub
	cd pubsub && make run
	./unit/unit-tests.out
	./integration/integration-tests.out

clean:
	rm -f ../*.o
	cd unit && make clean
	cd integration && make clean
	cd pubsub && make clean