#!/bin/bash
mkdir $1
cd $1
cmake .. -DCPACK_GENERATOR=RPM -DOPAE_BUILD_LEGACY=ON -DOPAE_PYTHON_VERSION=3.6
make package_rpm -j $(nproc)



