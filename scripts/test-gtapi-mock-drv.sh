#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make mock gtapi fpgad
./bin/fpgad -d
make test
kill $(cat /tmp/fpgad.pid)
echo "test-gtapi-mock-drv build PASSED"
