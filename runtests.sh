#!/bin/bash -x
make FILE_NAME="sample_tests/9x9_boards/tests100easy81.txt"NUM_TESTS="100"
make FILE_NAME="sample_tests/9x9_boards/tests100evil81.txt"NUM_TESTS="100"
make FILE_NAME="sample_tests/9x9_boards/tests1000easy81.txt"NUM_TESTS="1000"
make FILE_NAME="sample_tests/9x9_boards/tests1000evil81.txt"NUM_TESTS="1000"
make FILE_NAME="sample_tests/9x9_boards/tests10000easy81.txt"NUM_TESTS="10000"
make FILE_NAME="sample_tests/9x9_boards/tests10000evil81.txt"NUM_TESTS="10000"