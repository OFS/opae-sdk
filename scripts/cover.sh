#!/bin/bash -e

set -o xtrace

function finish() {
	echo "making coverage report"
	find testing -iname "*.gcda" -exec chmod 664 '{}' \;
	find testing -iname "*.gcno" -exec chmod 664 '{}' \;

	lcov --directory testing --capture --output-file coverage.info 2> /dev/null
	lcov --directory ase --capture --output-file coverage.ase 2> /dev/null

	lcov -a coverage.base -a coverage.info -a coverage.ase --output-file coverage.total
	lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/*' 'safe_string/**' 'pybind11/*' 'testing/**' 'samples/**' --output-file coverage.info.cleaned
	genhtml --function-coverage -o coverage_report coverage.info.cleaned 2> /dev/null

}
trap "finish" EXIT

mkdir -p unittests
cd unittests

if [ ! -f CMakeCache.txt ]; then
	cmake .. -DOPAE_PYTHON_VERSION=2.7 -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON -DBUILD_LIBOPAE_PY=ON -DBUILD_ASE_SAMPLES=ON
fi

mkdir -p coverage_files
rm -rf coverage_files/*

echo "Making tests"
make -j4 test_unit xfpga modbmc fpgad-xfpga test_ase opae-c-ase opae-c-ase-server-intg_xeon_nlb hello_fpga

lcov --directory . --zerocounters

nohup ./samples/intg_xeon_nlb/hw/ase_server.sh > samples/intg_xeon_nlb/hw/ase-server-nlb.log 2>&1 &
export ASE_WORKDIR=${PWD}/samples/intg_xeon_nlb/hw
sleep 30
LD_PRELOAD=./lib/libopae-c-ase.so ./bin/hello_fpga 2> /dev/null

lcov -c -i -d . -o coverage.base 2> /dev/null

LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
ctest -j3 --timeout 60

