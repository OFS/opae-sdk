#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make mock gtapi fpgad
LD_PRELOAD="$PWD/lib/libmock.so" ./bin/fpgad -d
CTEST_OUTPUT_ON_FAILURE=1 make test
kill $(cat /tmp/fpgad.pid)

echo "test-gtapi-mock-drv build PASSED"
