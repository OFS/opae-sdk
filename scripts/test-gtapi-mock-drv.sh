#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make
CTEST_OUTPUT_ON_FAILURE=1 make test
echo "test-gtapi-mock-drv build PASSED"
