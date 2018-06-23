#!/bin/bash -e

if [ ! -d coverage_gtapi_mock_drv ]; then
	mkdir coverage_gtapi_mock_drv
	mkdir coverage_gtapi_mock_drv/coverage_files
	mkdir coverage_gtapi_mock_drv/coverage_report
fi

pushd coverage_gtapi_mock_drv
set -o xtrace

function finish {
	kill $(cat /tmp/fpgad.pid)

	find */**/opae-c.dir -iname "*.gcda" -exec chmod 664 '{}' \;
	find */**/opae-c.dir -iname "*.gcno" -exec chmod 664 '{}' \;
	find */**/opae-cxx-core.dir -iname "*.gcda" -exec chmod 664 '{}' \;
	find */**/opae-cxx-core.dir -iname "*.gcno" -exec chmod 664 '{}' \;
	find */**/_opae.dir -iname "*.gcda" -exec chmod 664 '{}' \;
	find */**/_opae.dir -iname "*.gcno" -exec chmod 664 '{}' \;


	find */**/opae-c.dir -iname "*.gcda" | xargs -i cp {} coverage_files
	find */**/opae-c.dir -iname "*.gcno" | xargs -i cp {} coverage_files
	find */**/opae-cxx-core.dir -iname "*.gcda" | xargs -i cp {} coverage_files
	find */**/opae-cxx-core.dir -iname "*.gcno" | xargs -i cp {} coverage_files
	find */**/_opae.dir -iname "*.gcda" | xargs -i cp {} coverage_files
	find */**/_opae.dir -iname "*.gcno" | xargs -i cp {} coverage_files

	lcov --directory coverage_files --capture --output-file coverage.info
	lcov -a coverage.base -a coverage.info --output-file coverage.total
	lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/*' 'safe_string/**' 'tools/**' 'pyopae/pybind11/*' --output-file coverage.info.cleaned
	genhtml --function-coverage coverage.info -o coverage_report coverage.info.cleaned
	popd
}
trap "finish" EXIT


cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Coverage
make mock gtapi fpgad _opae

lcov --directory . --zerocounters
lcov -c -i -d . -o coverage.base

LD_PRELOAD="$PWD/lib/libmock.so" ./bin/fpgad -d
CTEST_OUTPUT_ON_FAILURE=1 make test
LD_PRELOAD="$PWD/lib/libmock.so" PYTHONPATH="$PWD/lib/python2.7" python -m nose2 test_pyopae

echo "coverage-gtapi-mock-drv build PASSED"
