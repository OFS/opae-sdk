#!/bin/bash

set -e
set -o xtrace

mkdir -p unittests
cd unittests

if [ ! -f CMakeCache.txt ]; then
    cmake .. -DOPAE_PYTHON_VERSION=3 \
             -DCMAKE_BUILD_TYPE=Coverage \
             -DOPAE_BUILD_LIBOPAE_CXX=ON \
             -DOPAE_BUILD_TESTS=ON \
             -DOPAE_ENABLE_MOCK=ON
fi

mkdir -p coverage_files
rm -rf coverage_files/*

make -j $(nproc)

lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base

LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
ctest --timeout 180

find . \( -iname "*.gcda" -or -iname "*.gcno" \) -exec chmod 664 '{}' \;

lcov --directory . --capture --output-file coverage.info
lcov -a coverage.base -a coverage.info --output-file coverage.total

lcov --remove coverage.total \
    '/usr/**' \
    '*libraries/libopaecxx/samples/**' \
    '*libraries/pyopae/**' \
    '*libraries/plugins/xfpga/usrclk/**' \
    '*libraries/c++utils/**' \
    '*tests/**' \
    '*binaries/mmlink/**' \
    '*binaries/fpgabist/**' \
    '*binaries/fpgadiag/**' \
    '*samples/host_exerciser/**' \
    '*samples/hssi/**' \
    '*samples/n5010-ctl/**' \
    '*samples/n5010-test/**' \
    --output-file coverage.info.cleaned

genhtml --function-coverage -o coverage_report coverage.info.cleaned
exit $?
