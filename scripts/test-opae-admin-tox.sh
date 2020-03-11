#!/bin/bash -e

mkdir mybuild
pushd mybuild

trap "popd" EXIT

cmake ..
make opae.admin.tox

echo "test-opae-admin-tox PASSED"
