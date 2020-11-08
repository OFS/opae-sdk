#!/bin/sh

SCRIPT_DIR=`dirname $0`
SCRIPT_DIR=`realpath $SCRIPT_DIR`
TOP_DIR=`realpath ${SCRIPT_DIR}/..`

# Input is the name of the directory the tarball will
# be unpacked to.
opae_dir=${1:-opae}
trans="s/opae-sdk/${opae_dir}/"

cd ${TOP_DIR}/..
tar --transform=$trans \
  --exclude=_build \
  --exclude=.* \
  --exclude=*~ \
  --exclude=opae-libs/plugins/ase \
  --exclude=external/opae-legacy/tests \
  --exclude=external/opae-legacy/scripts \
  --exclude=external/opae-legacy/tools/coreidle \
  --exclude=external/opae-legacy/tools/hssi \
  --exclude=tools/extra/pyfpgadiag \
  --exclude=tools/extra/pypackager \
  -z -c -f ${opae_dir}.tar.gz opae-sdk
