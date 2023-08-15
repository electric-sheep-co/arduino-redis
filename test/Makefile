TESTS = unit/unit-tests.out integration/integration-tests.out

test : $(TESTS)

unit/unit-tests.out:
	cd unit && make

integration/integration-tests.out:
	cd integration && make

run: clean test
	./unit/unit-tests.out
	./integration/integration-tests.out

clean:
	rm -f unit/unit-tests.out
	rm -f integration/integration-tests.out