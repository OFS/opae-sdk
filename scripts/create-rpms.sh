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

version=`grep 'Version:' ../opae.spec | awk '{ print $2 }'`
full_version=${version}-1

echo ${full_version}
#create source tarball
BUILD_DIR=${PWD}

echo ${PWD}

opae_sdk_tar="s/opae-sdk/opae-"
opae_sdk_tar+=${full_version}
opae_sdk_tar+="/"
echo ${opae_sdk_tar}

cd ../..
tar --transform=${opae_sdk_tar} \
  --exclude=.git \
  --exclude=.gitignore \
  --exclude=.github \
  --exclude=.travis.yml \
  --exclude=opae.spec.in \
  --exclude=opae.spec \
  --exclude=opae-libs/plugins/ase \
  --exclude=opae-libs/cmake/config/libopae-all.spec.in \
  --exclude=opae-libs/cmake/config/run_coverage_test.sh.in \
  --exclude=opae-libs/cmake/config/run_coverage_test_local.sh.in\
  --exclude=platforms \
  --exclude=samples/base \
  --exclude=samples/hello_afu \
  --exclude=samples/object_api \
  --exclude=samples/hello_events \
  --exclude=samples/hello_mpf_afu \
  --exclude=samples/intg_xeon_nlb \
  --exclude=samples/base \
  --exclude=scripts \
  --exclude=python \
  --exclude=tests/object_api \
  --exclude=tests/ase \
  --exclude=tests/ras \
  --exclude=tests/hello_events \
  --exclude=tools/fpgametrics \
  --exclude=tools/libboard/board_dc \
  --exclude=tools/extra/ras \
  --exclude=tools/extra/fpgabist \
  --exclude=tools/extra/pac_hssi_config \
  --exclude=tools/extra/fpgadiag \
  --exclude=tools/extra/c++utils \
  --exclude=tools/extra/pyfpgadiag \
  --exclude=tools/extra/pypackager \
  --exclude=tools/utilities \
  --exclude=opae-libs/include/opae/cxx/.clang-format \
  --exclude=opae-libs/libopaecxx/.clang-format \
  --exclude=opae-libs/pyopae/.clang-format \
  --exclude=opae-libs/.clang-format \
  --exclude=opae-libs/.clang-format \
  -z -c -f opae-${full_version}.tar.gz opae-sdk



mv opae-${full_version}.tar.gz ~/rpmbuild/SOURCES/
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
