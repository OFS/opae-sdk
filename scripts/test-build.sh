#!/bin/bash -e

mkdir mybuild
pushd mybuild

trap "popd" EXIT

cmake .. -DBUILD_ASE=1
make -j

echo "test-build PASSED"
