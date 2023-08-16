# Arduino Redis Library

[![Arduino](https://www.vectorlogo.zone/logos/arduino/arduino-ar21.svg)](https://www.arduino.cc)
[![Redis](https://www.vectorlogo.zone/logos/redis/redis-ar21.svg)](https://redis.io/)

A [Redis](https://redis.io/) client library for [Arduino](https://www.arduino.cc). 

Known to support the ESP8266 & ESP32 platforms; may support others without modification (see documentation).

Available via the [Arduino Library Manager](https://www.arduino.cc/en/guide/libraries): simply search for "redis".

Latest release always available for direct download [here](https://github.com/electric-sheep-co/arduino-redis/releases/latest).

## [Current documentation](http://arduino-redis.com)

## [Examples](./examples)

## Questions & Discussion

If you have any issue with the library please [open a ticket here](https://github.com/electric-sheep-co/arduino-redis/issues/new) or email [arduino-redis@electricsheep.co](mailto:arduino-redis@electricsheep.co).

To discuss the project via real-time chat with other developers, you're invited to join the [#arduino-redis channel on Libera.chat](https://web.libera.chat/?channel=#arduino-redis). If you'd prefer to just open an asynchronous discussion in a forum-like system, [create a new Discussion](https://github.com/edfletcher/drc/discussions/new) here.

## Developing


After cloning this repository, you must initialize and update the submodules:

```shell
$ git submodule init && git submodule update --recursive
```

This will download [EpoxyDuino](https://github.com/bxparks/EpoxyDuino) and [AUnit](https://github.com/bxparks/AUnit), dependencies for the test harness described below.

### Test harness

Available in [`./test`](./test). There are two variants, `unit` & `integration`.

#### Build

Requires the same [build depedencies as ExpoyDuino](https://github.com/bxparks/EpoxyDuino#dependencies).

```shell
$ cd test
$ make
```

This will produce binaries named `<variant>.out` in the `test/<variant>` directories.

#### Running

(Using the `integration` tests variant as an example)

```shell
$ cd test
$ make run   # will clean & rebuild
rm unit/unit-tests.out
rm integration/integration-tests.out
cd unit && make
make[1]: Entering directory '/home/ryan/arduino-redis/test/unit'
...
TestRunner started on 27 test(s).
Test IntegrationTests_append passed.
...
Test IntegrationTests_wait_for_expiry_ms passed.
TestRunner duration: 2.654 seconds.
TestRunner summary: 27 passed, 0 failed, 0 skipped, 0 timed out, out of 27 test(s).
```

Requires Redis server available by default at `localhost:6379`. The hostname, port and authentication (if necessary) can be specified at runtime by setting the following environment variables:
* `ARDUINO_REDIS_TEST_HOST`
* `ARDUINO_REDIS_TEST_PORT`
* `ARDUINO_REDIS_TEST_AUTH`

Tests can be filtered by setting `ARDUINO_REDIS_TEST_INCLUDE`, the value of which will be used as the [specification to `TestRunner::include()`](https://github.com/bxparks/AUnit#filtering-test-cases). 

#### Submitting a PR

Please review the [contribution guidelines](./CONTRIBUTING.md) before submission taking important note of the requirement that integration tests must pass and any changed or added functionality include appropriate additional tests. Thank you!

## Authors
* Ryan Joseph (Electric Sheep) <ryan@electricsheep.co> -- maintainer
* Remi Caumette
* Robert Oostenveld
* Mark Lynch <mark.christinat@gmail.com>
* Daniel (https://github.com/darmiel)
* gde-2 (https://github.com/gde-2)
* ErMaVi (https://github.com/ErMaVi)
* Craig Hollabaugh (https://github.com/holla2040) -- maintainer
* Martin (https://github.com/harmrochel)
* Nafraf (https://github.com/nafraf)
