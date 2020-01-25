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
rm -rf ../build
mkdir ../build
cd ../build
BUILD_DIR=${PWD}

cd ../..
tar --transform='s/opae-sdk/opae/' \
  --exclude=.git \
  --exclude=.gitignore \
  --exclude=.github \
  --exclude=.travis.yml \
  --exclude=opae.spec.in \
  --exclude=opae.spec \
  --exclude=libopae/plugins/ase \
  --exclude=platforms \
  --exclude=samples/base \
  --exclude=samples/hello_afu \
  --exclude=samples/hello_mpf_afu \
  --exclude=samples/intg_xeon_nlb \
  --exclude=scripts \
  --exclude=testing \
  --exclude=tools/base/fpgaport \
  --exclude=tools/extra/libopae++ \
  --exclude=tools/extra/pac/fpgaflash \
  --exclude=tools/extra/pac/pac_hssi_config \
  --exclude=tools/extra/pac/pyfpgaflash \
  --exclude=tools/extra/packager \
  --exclude=tools/extra/pyfpgadiag \
  --exclude=tools/extra/pypackager \
  --exclude=tools/utilities \
  --exclude=common/include/opae/cxx/.clang-format \
  --exclude=libopae/.clang-format \
  --exclude=libopaecxx/.clang-format \
  --exclude=pyopae/.clang-format \
  --exclude=testing/.clang-format \
  --exclude=testing/xfpga/.clang-format \
  --exclude=tools/base/argsfilter/.clang-format \
  -z -c -f opae.tar.gz opae-sdk

mv opae.tar.gz ~/rpmbuild/SOURCES/
cp "${BUILD_DIR}/../opae.spec" ~/rpmbuild/SPECS/

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
