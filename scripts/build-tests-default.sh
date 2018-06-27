#!/bin/bash -e

mkdir build_tests_default
pushd build_tests_default

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON
make gtapi
echo "Successfully built tests with default parameters."
