#!/bin/bash

set -e
set -o xtrace

mkdir -p unittests
cd unittests

if [ ! -f CMakeCache.txt ]; then
    cmake .. -DOPAE_PYTHON_VERSION=3.6 \
             -DCMAKE_BUILD_TYPE=Debug \
             -DOPAE_BUILD_LIBOPAE_CXX=ON \
             -DOPAE_BUILD_TESTS=ON \
             -DOPAE_BUILD_LIBOPAE_PY=ON \
             -DOPAE_ENABLE_MOCK=ON \
             -DOPAE_BUILD_SIM=OFF
fi

make -j 4


LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
ctest -j 4 --timeout 180

exit $?
