#!/bin/bash

pushd $(dirname $0)/..

PYCODESTYLE=$(which pycodestyle)
PYLINT=$(which pylint)
FILES=$(find . -iname "*.py" -not -name "cpplint.py" -not -path "./doc/*" \
-not -path "./tools/extra/packager/jsonschema-2.3.0/*")
FILES+=" "
FILES+=$(grep -rl "^#./usr/bin.*python" ./* | grep -v cpplint.py | grep -vE "^\.\/doc\/")

if [ "$TRAVIS_COMMIT_RANGE" != "" ]; then
    CHANGED_FILES=$(git diff --name-only $TRAVIS_COMMIT_RANGE)
    printf "Looking at changed files:\n${CHANGED_FILES}"
    FILES=$(comm -12 <(sort <(for f in $FILES; do printf "$f\n"; done)) <(sort <(for f in $CHANGED_FILES; do printf "./$f\n"; done)))
fi

if [ "$1" == "-v" ]; then
	echo "Checking the following files:"
	echo $FILES
fi

if [ "$FILES" == "" ]; then
    exit 0
fi

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

