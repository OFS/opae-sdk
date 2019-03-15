#!/bin/bash -e

set -o xtrace

function finish() {
	echo "making coverage report"
	find testing -iname "*.gcda" -exec chmod 664 '{}' \;
	find testing -iname "*.gcno" -exec chmod 664 '{}' \;

	lcov --directory testing --capture --output-file coverage.info 2> /dev/null
	lcov --directory ase --capture --output-file coverage.ase 2> /dev/null
	lcov --directory samples/intg_xeon_nlb --capture --output-file coverage.nlb 2> /dev/null

	if [ -d "./samples/hello_intr_afu" ]; then
		lcov --directory samples/hello_intr_afu --capture --output-file coverage.intr 2> /dev/null
		lcov -a coverage.ase -a coverage.info -a coverage.nlb -a coverage.intr --output-file coverage.total
	else
		lcov -a coverage.ase -a coverage.info -a coverage.nlb --output-file coverage.total
	fi
	lcov --remove coverage.total '/usr/**' 'tests/**' '*/**/CMakeFiles*' '/usr/*' 'safe_string/**' 'pybind11/*' 'testing/**' 'samples/**' --output-file coverage.info.cleaned
	genhtml --function-coverage -o coverage_report coverage.info.cleaned 2> /dev/null

}
trap "finish" EXIT

mkdir -p unittests
cd unittests

if [ -d "../samples/hello_intr_afu" ]; then
	if [ ! -f CMakeCache.txt ]; then
		cmake .. -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON -DBUILD_ASE_SAMPLES=ON -DBUILD_ASE_INTR=ON
	fi
else
	if [ ! -f CMakeCache.txt ]; then
		cmake .. -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTS=ON -DBUILD_ASE_SAMPLES=ON
	fi
fi
mkdir -p coverage_files
rm -rf coverage_files/*

echo "Making tests"

if [ -d "../samples/hello_intr_afu" ]; then
	make -j4 test_ase opae-c-ase opae-c-ase-server-intg_xeon_nlb opae-c-ase-server-hello_intr_afu_hw hello_fpga hello_intr_afu
else
	make -j4 test_ase opae-c-ase opae-c-ase-server-intg_xeon_nlb  hello_fpga
fi

lcov --directory . --zerocounters

# ASE need to count the coverage data collected by running hello_fpga and hello_intr_afu sample tests
nohup ./samples/intg_xeon_nlb/hw/ase_server.sh > samples/intg_xeon_nlb/hw/ase-server-nlb.log 2>&1 &
export ASE_WORKDIR=${PWD}/samples/intg_xeon_nlb/hw
sleep 30
LD_PRELOAD=./lib/libopae-c-ase.so ./bin/hello_fpga 2> /dev/null
sleep 10

if [ -d "./samples/hello_intr_afu" ]; then
	nohup ./samples/hello_intr_afu/hw/ase_server.sh > samples/hello_intr_afu/hw/ase-server-intr.log 2>&1 &
	export ASE_WORKDIR=${PWD}/samples/hello_intr_afu/hw
	sleep 30
	LD_PRELOAD=./lib/libopae-c-ase.so ./bin/hello_intr_afu 2> /dev/null
fi

lcov -c -i -d . -o coverage.base 2> /dev/null

LD_LIBRARY_PATH=${PWD}/lib \
CTEST_OUTPUT_ON_FAILURE=1 \
OPAE_EXPLICIT_INITIALIZE=1 \
ctest -j3 --timeout 60





