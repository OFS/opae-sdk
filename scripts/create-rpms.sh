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

SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`realpath $SCRIPT_DIR`

#create source tarball
TOP_DIR=`realpath ${SCRIPT_DIR}/..`

echo ${TOP_DIR}

cd ${TOP_DIR}
rm -rf _build
mkdir _build
cd _build
cmake .. -DOPAE_BUILD_LEGACY=ON  -DOPAE_BUILD_TESTS=ON
cd ..

# From
#   Version:        2.0.0
# To
#   2.0.0
version=`grep ^Version "${TOP_DIR}/opae.spec" | awk '{ print $2 }'`

${SCRIPT_DIR}/create-tarball.sh opae-${version}

if [ ! -f ../opae-${version}.tar.gz ]; then
    echo "Creating the tarball failed"
    exit 1
fi
mv ../opae-${version}.tar.gz ~/rpmbuild/SOURCES/
cp "${TOP_DIR}/opae.spec" ~/rpmbuild/SPECS/

echo $"${TOP_DIR}/opae.spec"

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
cp ~/rpmbuild/RPMS/x86_64/opae-* $TOP_DIR/_build
cp ~/rpmbuild/SRPMS/opae-*.src.rpm $TOP_DIR/_build
