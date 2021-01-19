# Tests

Minimal tests for the Symbol application.

## Build

Building tests requires [CMake](https://cmake.org/) and [cmocka](https://cmocka.org/).

Once they have been installed: run:

```shell
mkdir build && cd build
cmake ..
make
```

## Running tests

In the build folder, run:

```shell
./test_transaction_parser
```
