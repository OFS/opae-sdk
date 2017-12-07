#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON
make

make test
echo "test-gtapi-mock-drv build PASSED"
