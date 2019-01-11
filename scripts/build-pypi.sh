#!/bin/bash

mkdir mybuild
cd mybuild

cmake .. -DBUILD_PYTHON_DIST=ON
make -j4
make pyopae-dist
cp pyopae/stage/dist/opae.fpga-*.tar.gz .
echo "Python distribution generated"