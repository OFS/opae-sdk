#!/bin/bash

pushd $(dirname $0)/..

PYCODESTYLE=$(which pycodestyle)
FILES=$(find . -iname "*.py")
FILES+=" "
FILES+=$(grep -rl "^#./usr/bin.*python" ./*)

if [ ! -x "$PYCODESTYLE" ]; then
	echo "no pycodestyle"
	popd
	exit 1
fi

$PYCODESTYLE $FILES

if [ $? -eq 1 ]; then
	echo "test-codingstyle-py FAILED"
	popd
	exit 1
else
	echo "test-codingstyle-py PASSED"
	popd
	exit 0
fi

