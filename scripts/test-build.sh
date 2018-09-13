#!/bin/bash -e

mkdir mybuild
pushd mybuild

trap "popd" EXIT

cmake .. -DBUILD_ASE=1 -DCMAKE_BUILD_TYPE=$TRAVIS_BUILD_TYPE
make opae-c xfpga opae-cxx-core pyopae

echo "test-build PASSED"
