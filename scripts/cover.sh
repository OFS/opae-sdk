#!/bin/bash -e

set -o xtrace

function finish {
	#kill $(cat $PWD/fpgad.pid)

	find testing -iname "*.gcda" -exec chmod 664 '{}' \;
	find testing -iname "*.gcno" -exec chmod 664 '{}' \;


	find testing -iname "*.gcda" | xargs -i cp {} coverage_files
	find testing -iname "*.gcno" | xargs -i cp {} coverage_files

	lcov --directory coverage_files --capture --output-file coverage.info
	lcov -a coverage.base -a coverage.info --output-file coverage.total
	lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/*' 'safe_string/**' 'pybind11/*' 'testing/**' --output-file coverage.info.cleaned
	genhtml --function-coverage -o coverage_report coverage.info.cleaned
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
make -j4 test_unit xfpga


lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base


LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
 make test
