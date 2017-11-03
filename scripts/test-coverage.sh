#!/bin/bash -e

mkdir mybuild_coverage
pushd mybuild_coverage

trap "popd" EXIT

cmake .. -DBUILD_ASE=1 -DCMAKE_BUILD_TYPE=Coverage
make -j

echo "test-build coverage PASSED"
