#!/bin/bash -e

mkdir mybuild_coverage
pushd mybuild_coverage

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Coverage
make -j

make coverage_opae-c
echo "test-build coverage PASSED"
