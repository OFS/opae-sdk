#!/bin/bash -e

mkdir coverage_gtapi_mock_drv
pushd coverage_gtapi_mock_drv

trap "popd" EXIT
mkdir coverage_files
mkdir coverage_report


cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Coverage
source bin/activate
make mock gtapi fpgad pyopae


lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base

LD_PRELOAD="$PWD/lib/libmock.so" ./bin/fpgad -d
sleep 1
CTEST_OUTPUT_ON_FAILURE=1 make test
kill $(cat /tmp/fpgad.pid)
deactivate

find */**/opae-c.dir -iname "*.gcda" -exec chmod 664 '{}' \;
find */**/opae-c.dir -iname "*.gcno" -exec chmod 664 '{}' \;
find */**/opae-cxx-core.dir -iname "*.gcda" -exec chmod 664 '{}' \;
find */**/opae-cxx-core.dir -iname "*.gcno" -exec chmod 664 '{}' \;


find */**/opae-c.dir -iname "*.gcda" | xargs -i cp {} coverage_files
find */**/opae-c.dir -iname "*.gcno" | xargs -i cp {} coverage_files
find */**/opae-cxx-core.dir -iname "*.gcda" | xargs -i cp {} coverage_files
find */**/opae-cxx-core.dir -iname "*.gcno" | xargs -i cp {} coverage_files

lcov --directory coverage_files --capture --output-file coverage.info
lcov -a coverage.base -a coverage.info --output-file coverage.total
lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/include/c++/**' 'safe_string/**' 'tools/**' --output-file coverage.info.cleaned
genhtml --function-coverage coverage.info -o coverage_report coverage.info.cleaned

echo "coverage-gtapi-mock-drv build PASSED"
