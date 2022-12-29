#!/bin/bash

#check if rpmbuild is installed
yum list installed rpm-build
if [ $? -eq 1 ]; then
	echo "'rpm-build' package not installed.. exiting"
	exit 1
fi

#check if rpmdevtools is installed
yum list installed rpmdevtools
if [ $? -eq 1 ]; then
	echo "'rpmdevtools' package not installed.. exiting"
	exit 1
fi

rm -rf ~/rpmbuild
rpmdev-setuptree

#create source tarball
BUILD_DIR=${PWD}

echo ${PWD}

cd ../..
tar --transform='s/opae-sdk/opae/' \
  --exclude=.git \
  --exclude=.gitignore \
  --exclude=.github \
  --exclude=.travis.yml \
  --exclude=opae.spec.in \
  --exclude=opae.spec \
  --exclude=platforms \
  --exclude=samples/base \
  --exclude=samples/hello_afu \
  --exclude=samples/dummy_afu \
  --exclude=samples/hello_mpf_afu \
  --exclude=samples/intg_xeon_nlb \
  --exclude=samples/base \
  --exclude=scripts \
  --exclude=tests \
  --exclude=bin/ras \
  --exclude=bin/pac_hssi_config \
  --exclude=bin/pyfpgadiag \
  --exclude=bin/pypackager \
  --exclude=bin/utilities \
  --exclude=lib/include/opae/cxx/.clang-format \
  --exclude=lib/libopaecxx/.clang-format \
  --exclude=lib/pyopae/.clang-format \
  --exclude=lib/.clang-format \
  -z -c -f opae.tar.gz opae-sdk



mv opae.tar.gz ~/rpmbuild/SOURCES/
cp "${BUILD_DIR}/../opae.spec" ~/rpmbuild/SPECS/

echo $"${BUILD_DIR}/../opae.spec"


cd ~/rpmbuild/SPECS/

#generate RPMS
rpmbuild -ba opae.spec
if [ $? -eq 1 ]; then
	echo "building rpm failed.. exiting"
	exit 1
fi

rpmbuild -bs opae.spec
if [ $? -eq 1 ]; then
	echo "building srpm failed.. exiting"
	exit 1
fi

#copy RPMS to build directory
cp ~/rpmbuild/RPMS/x86_64/opae-* $BUILD_DIR/
cp ~/rpmbuild/SRPMS/opae-*.src.rpm $BUILD_DIR/
