#!/bin/bash
set -x
if [ -z ${1+1} ]; then
	echo "Must specify a build directory"
	exit 1
fi
bld=$(realpath $1)
src=$(realpath ${2:-$PWD})

CMAKE=`which cmake3 2>/dev/null`
if [ -z $CMAKE ]; then
  CMAKE=`which cmake 2>/dev/null`
fi

mkdir -p $bld
echo "Building at $bld from source $src"
$CMAKE -S $src -B $bld -DCPACK_GENERATOR=RPM \
	               -DOPAE_BUILD_LEGACY=ON \
	               -DOPAE_PYTHON_VERSION=3.6 \
	               -DOPAE_BUILD_EXTRA_TOOLS_FPGABIST=ON \
	               -DCMAKE_INSTALL_PREFIX=/usr
$CMAKE --build $bld -- package_rpm -j $(nproc)



