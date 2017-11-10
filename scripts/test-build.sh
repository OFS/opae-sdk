#!/bin/bash -e

mkdir mybuild
pushd mybuild

trap "popd" EXIT

cmake .. -DBUILD_ASE=1
make

echo "test-build PASSED"
