#!/bin/bash

pushd $(dirname $0)/..

PYCODESTYLE=$(which pycodestyle)
PYLINT=$(which pylint)
FILES=$(find . -iname "*.py" -not -name "cpplint.py")
FILES+=" "
FILES+=$(grep -rl "^#./usr/bin.*python" ./* | grep -v cpplint.py)

if [ ! -x "$PYLINT" ]; then
	echo "no pylint"
	popd
	exit 1
fi

if [ ! -x "$PYCODESTYLE" ]; then
	echo "no pycodestyle"
	popd
	exit 1
fi

echo -e "\n===== pycodestyle ====="
$PYCODESTYLE $FILES
if [ $? -ne 0 ]; then
	echo "test-codingstyle-py FAILED"
	popd
	exit 1
fi

echo -e "\n===== pylint -E ====="
$PYLINT -E -f parseable $FILES
if [ $? -ne 0 ]; then
	echo "test-codingstyle-py FAILED"
	popd
	exit 1
fi

echo "test-codingstyle-py PASSED"
popd
exit 0