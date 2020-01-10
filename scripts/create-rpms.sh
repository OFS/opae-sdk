#!/bin/bash

#create source tarball
rm -rf ../build
mkdir ../build
cd ../build
BUILD_DIR=${PWD}
tar czvf opae.tar.gz ../../opae-sdk --transform s/opae-sdk/opae/
cp opae*.tar.gz ~/rpmbuild/SOURCES/
cp ../opae-sdk-rpm.spec ~/rpmbuild/SPECS/

cd ~/rpmbuild/SPECS/

#clean RPMS directory
rm -rf ../RPMS/x86_64/opae-*

#generate RPMS
rpmbuild -ba opae-sdk-rpm.spec

#copy RPMS to build directory
cp ~/rpmbuild/RPMS/x86_64/opae-* $BUILD_DIR/
