#!/bin/bash -e

mkdir admin_builder
pushd admin_builder

trap "popd" EXIT

cmake ..
make opae.admin.rpm

if [ $? -eq 0 ]
then
	cp python/opae.admin/stage/dist/* .
else
	echo "build-opae-admin-rpm FAILED"
	exit 1
fi

echo "build-opae-admin-rpm PASSED"
