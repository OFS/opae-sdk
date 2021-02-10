#!/bin/bash

cmake=cmake

if ! command -v cmake > /dev/null && command -v cmake3 > /dev/null; then
  cmake=cmake3
fi

mkdir $1
cd $1
$cmake .. -DCPACK_GENERATOR=RPM -DOPAE_BUILD_LEGACY=ON -DOPAE_PYTHON_VERSION=3.6 -DOPAE_BUILD_EXTRA_TOOLS_FPGABIST=ON
make package_rpm -j $(nproc)



