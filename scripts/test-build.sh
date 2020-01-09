#!/bin/bash -e

mkdir mybuild
pushd mybuild

trap "popd" EXIT

cmake .. -DBUILD_ASE=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE
make -j

echo "test-build PASSED"
