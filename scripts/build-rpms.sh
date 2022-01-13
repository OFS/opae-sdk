#!/bin/bash
set -x
if [ -z ${1+1} ]; then
	echo "Must specify a build directory"
	exit 1
fi
bld=$(realpath $1)
src=$(realpath ${2:-$PWD})
cmake=cmake

if ! command -v cmake > /dev/null && command -v cmake3 > /dev/null; then
  cmake=cmake3
fi

mkdir -p $bld
echo "Building at $bld from source $src"
$cmake -S $src -B $bld -DCPACK_GENERATOR=RPM \
	               -DOPAE_PYTHON_VERSION=3.6 \
	               -DOPAE_BUILD_EXTRA_TOOLS_FPGABIST=ON \
	               -DCMAKE_INSTALL_PREFIX=/usr
$cmake --build $bld -- package_rpm -j $(nproc)



