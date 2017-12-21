#!/bin/bash -e

mkdir coverage_gtapi_mock_drv
pushd coverage_gtapi_mock_drv

trap "popd" EXIT
mkdir coverage_files
mkdir coverage_report


cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Coverage
make mock gtapi

lcov --directory . --zerocounters

make test
find */**/opae-c.dir -iname "*.gcda" | xargs -i cp {} coverage_files
find */**/opae-c.dir -iname "*.gcno" | xargs -i cp {} coverage_files

lcov -t test_coverage -o coverage.info -c -d coverage_files
lcov --remove coverage.info '/usr/**' 'tests/**' '*/**/CMakeFiles*' --output-file coverage.info.cleaned
genhtml --branch-coverage --function-coverage coverage.info -o coverage_report coverage.info.cleaned

#TODO - Enable coveralls once its integrated with github
#coveralls -l coverage.info.cleaned

echo "coverage-gtapi-mock-drv build PASSED"
