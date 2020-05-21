#!/bin/bash

set -e
set -o xtrace

mkdir -p unittests
cd unittests

if [ ! -f CMakeCache.txt ]; then
    cmake .. -DOPAE_PYTHON_VERSION=2.7 \
             -DCMAKE_BUILD_TYPE=Coverage \
             -DOPAE_BUILD_LIBOPAE_CXX=ON \
             -DOPAE_BUILD_TESTS=ON \
             -DOPAE_BUILD_LIBOPAE_PY=ON \
             -DOPAE_ENABLE_MOCK=ON \
             -DOPAE_BUILD_SIM=OFF
fi

mkdir -p coverage_files
rm -rf coverage_files/*

make -j 4

lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base 2> /dev/null

LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
ctest -j 4 --timeout 180

find . \( -iname "*.gcda" -or -iname "*.gcno" \) -exec chmod 664 '{}' \;

lcov --directory . --capture --output-file coverage.info 2> /dev/null
lcov -a coverage.base -a coverage.info --output-file coverage.total

lcov --remove coverage.total \
    '/usr/**' \
    '*external/opae-test/**' \
    '*opae-libs/libopaecxx/samples/**' \
    '*opae-libs/tests/**' \
    '*opae-libs/pyopae/**' \
    '*opae-libs/plugins/xfpga/usrclk/**' \
    '*tests/**' \
    '*tools/extra/c++utils/**' \
    '*tools/libboard/board_a10gx/**' \
    '*tools/extra/mmlink/**' \
    '*tools/extra/fpgabist/**' \
    '*tools/extra/fpgadiag/**' \
    '*pybind11/**' \
    --output-file coverage.info.cleaned

genhtml --function-coverage -o coverage_report coverage.info.cleaned 2> /dev/null
exit $?
