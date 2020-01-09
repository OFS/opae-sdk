#!/bin/bash

mkdir rpmbuilder
pushd rpmbuilder

trap "popd" EXIT

SCRIPT="
rm -rf ~/opae-sdk-x-bat
mkdir ~/opae-sdk-x-bat
cd ~/opae-sdk-x-bat
git clone ssh://git@gitlab.devtools.intel.com:29418/OPAE/opae-sdk-x.git
cd opae-sdk-x
git checkout $CI_COMMIT_REF_NAME
mkdir build && cd build
cmake .. -DCPACK_GENERATOR=RPM
make -j
make package_rpm"

if ssh -o StrictHostKeyChecking=no -l lab $HOST_NCF21 "${SCRIPT}" < /dev/null; then
#if [$? -eq 0 ]; then
	#fetch packages
	scp -o StrictHostKeyChecking=no $OPAE_RPM_BUILDER:~/opae-sdk-x-bat/opae-sdk-x/build/*.rpm .
else
	echo "RPM generation failed ..."
	exit 1
fi

echo "RPM generation done ..."
