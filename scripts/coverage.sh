#!/bin/sh

set -e -u

TARGET=ptwalker

scons -Q -j$(nproc) --cache-disable --coverage "./$TARGET"

lcov --base-directory . --directory src --directory tesy --zerocounters -q
./"$TARGET"
lcov --base-directory . --directory . -c -o coverage.info
lcov --remove coverage.info "/usr/*" -o coverage.info
lcov --remove coverage.info "*/test/*" -o coverage.info
rm -rf coverage/
genhtml -o coverage/ -t "Page Table Walker" coverage.info

