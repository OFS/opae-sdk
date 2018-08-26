#!/bin/bash -e

set -o xtrace

function finish {
	#kill $(cat $PWD/fpgad.pid)

	find */**/munit-opae-c.dir -iname "*.gcda" -exec chmod 664 '{}' \;
	find */**/munit-opae-c.dir -iname "*.gcno" -exec chmod 664 '{}' \;


	find */**/munit-opae-c.dir -iname "*.gcda" | xargs -i cp {} coverage_files
	find */**/munit-opae-c.dir -iname "*.gcno" | xargs -i cp {} coverage_files

	lcov --directory coverage_files --capture --output-file coverage.info
	lcov -a coverage.base -a coverage.info --output-file coverage.total
	lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/*' 'safe_string/**' 'tools/**' 'pybind11/*' 'testing/**' --output-file coverage.info.cleaned
	genhtml --function-coverage -o coverage_report coverage.info.cleaned
	popd
}
trap "finish" EXIT

mkdir -p unittests
cd unittests

if [ ! -f CMakeCache.txt ]; then
	cmake .. -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON -DNEW_UNITS=ON -DBUILD_LIBOPAE_PY=OFF
fi


mkdir -p coverage_files
rm -rf coverage_files/*

echo "Making tests"
make munit-opae-c

lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base
./bin/munit-opae-c --gtest_filter="enum*:buffer*:*properties*"

