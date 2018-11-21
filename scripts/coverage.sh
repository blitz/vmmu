#!/bin/sh

set -e -u

TARGET=$1

scons -Q -j$(nproc) --cache-disable --coverage "$1"

lcov --base-directory . --directory . --zerocounters -q
./"$TARGET"
lcov --base-directory . --directory . -c -o coverage.info
lcov --remove coverage.info "/usr/*" -o coverage.info
lcov --remove coverage.info "*/test_*.cpp" -o coverage.info
rm -r coverage/
genhtml -o coverage/ -t "Page Table Walker" coverage.info

