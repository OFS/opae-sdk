#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
. bin/activate
make mock gtapi fpgad pyopae
LD_PRELOAD="$PWD/lib/libmock.so" ./bin/fpgad -d
CTEST_OUTPUT_ON_FAILURE=1 make test
if [ "$(pgrep fpgad)" -eq "$(cat /tmp/fpgad.pid)" ]; then
	kill $(cat /tmp/fpgad.pid)
fi
deactivate

echo "test-gtapi-mock-drv build PASSED"
