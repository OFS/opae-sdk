#!/bin/bash -e

mkdir mybuild_gtapi_mock_drv
pushd mybuild_gtapi_mock_drv

trap "popd" EXIT
trap "killall fpgad" EXIT

cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
make gtapi mock fpgad
./bin/fpgad -d
CTEST_OUTPUT_ON_FAILURE=1 make test
echo "test-gtapi-mock-drv build PASSED"
