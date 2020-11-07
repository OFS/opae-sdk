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

# From
#   Version:        2.0.0
# To
#   2.0.0
version=`grep ^Version "${BUILD_DIR}/../opae.spec" | awk '{ print $2 }'`

${BUILD_DIR}/create-tarball.sh opae-${version}
cd ../..

if [ ! -f opae-${version}.tar.gz ]; then
    echo "Creating the tarball failed"
    exit 1
fi
mv opae-${version}.tar.gz ~/rpmbuild/SOURCES/
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
