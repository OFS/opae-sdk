#!/bin/bash
rpmdir=$(realpath ${1:-$PWD})
rpmdir="${rpmdir}/packaging/opae/rpm"

dnf install -y $rpmdir/opae*.rpm
if [ $? -ne 0 ]; then
	echo "Could not install OPAE RPMs"
	exit 1
fi

failures=0
test_exit_code(){
	expected_code=$1
	shift
	cmd="$@"
	echo "testing: $cmd"
	$cmd > output.dat
	actual_code=$?
	if [ $expected_code != $actual_code ]; then
		failures=$(echo 1+$failures | bc)
		echo "$cmd: expected $expected_code, got $actual_code" >> errors.txt
		cat output.dat
	fi
}

if [ -f errors.txt ]; then
	unlink errors.txt
fi

test_exit_code 0 "fpgainfo -h"
test_exit_code 1 "fpgaconf -h"
test_exit_code 0 "pci_device -h"
test_exit_code 0 "fpgasupdate -h"
test_exit_code 0 "hssi -h"
test_exit_code 0 "dummy_afu -h"
test_exit_code 0 "opae.io  -h"
test_exit_code 0 "PACSign -h"
test_exit_code 1 "fpgadiag -h"
test_exit_code 100 "fpgadiag -m lpbk1 -h"
test_exit_code 100 "fpgadiag -m read -h"
test_exit_code 100 "fpgadiag -m write -h"
test_exit_code 100 "fpgadiag -m trput -h"

dd if=/dev/urandom  of=dummy.bin bs=1 count=1024 2> /dev/null
test_exit_code 0 \
	"PACSign PR -H openssl_manager -t UPDATE -y -i dummy.bin -o dummy.signed"
echo "unexpected exit codes shown below"
if [ -f errors.txt ]; then
	cat errors.txt
fi
echo "failures: $failures"
exit $failures
