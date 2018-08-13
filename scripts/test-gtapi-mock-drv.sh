#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv
result=FAILED
function finish() {
	kill $(cat $PWD/fpgad.pid)
	echo "test-gtapi-mock-drv build $result"

}
trap "finish" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make mock gtapi fpgad _opae opae-c-munit
LD_PRELOAD="$PWD/lib/libmock.so" ./bin/fpgad -d -D $PWD -l $PWD/fpgad.log -p $PWD/fpgad.pid
CTEST_OUTPUT_ON_FAILURE=1 make test
result=PASSED
