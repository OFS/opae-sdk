#!/bin/bash

mkdir debbuilder
pushd debbuilder

trap "popd" EXIT

cmake .. -DCPACK_GENERATOR=DEB
make -j
make package_deb

echo "DEB generation done ..."
