#!/bin/bash

mkdir source_code
pushd source_code

trap "popd" EXIT

cmake .. -DCPACK_GENERATOR=TGZ
make dist

echo "Tarball generated .."
