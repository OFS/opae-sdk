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

cd .. 
rm -rf _build
mkdir _build
cd _build
cmake .. -DOPAE_BUILD_LEGACY=ON  -DOPAE_BUILD_TESTS=ON


cd ../..
tar --transform='s/opae-sdk/opae/' \
  --exclude=.git \
  --exclude=.gitignore \
  --exclude=.github \
  --exclude=.travis.yml \
  --exclude=opae.spec.in \
  --exclude=opae.spec \
  --exclude=opae-libs/external \
  --exclude=opae-libs/plugins/ase \
  --exclude=opae-libs/cmake/config/libopae-all.spec.in \
  --exclude=opae-libs/cmake/config/run_coverage_test.sh.in \
  --exclude=opae-libs/cmake/config/run_coverage_test_local.sh.in\
  --exclude=external/opae-legacy/tests \
  --exclude=external/opae-legacy/scripts \
  --exclude=external/opae-legacy/tools/coreidle \
  --exclude=external/opae-legacy/tools/hssi \
  --exclude=platforms \
  --exclude=samples/base \
  --exclude=samples/hello_afu \
  --exclude=samples/dummy_afu \
  --exclude=samples/hello_mpf_afu \
  --exclude=samples/intg_xeon_nlb \
  --exclude=samples/base \
  --exclude=scripts \
  --exclude=tools/extra/ras \
  --exclude=tools/extra/pac_hssi_config \
  --exclude=tools/extra/pyfpgadiag \
  --exclude=tools/extra/pypackager \
  --exclude=tools/utilities \
  --exclude=opae-libs/include/opae/cxx/.clang-format \
  --exclude=opae-libs/libopaecxx/.clang-format \
  --exclude=opae-libs/pyopae/.clang-format \
  --exclude=opae-libs/.clang-format \
  --exclude=opae-libs/.clang-format \
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
