#!/bin/bash -e

cd tools/pac_sign

python3 setup.py bdist_rpm --no-autoreq --force-arch=x86_64

if [ $? -eq 0 ]
then
	mkdir ../../pacsign_builder
	cp dist/* ../../pacsign_builder/
else
	echo "build-pacsign-rpm FAILED"
	exit 1
fi
echo "build-pacsign-rpm PASSED"