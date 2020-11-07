#!/bin/sh

here=`dirname $0`
here=`realpath $here`

# Input is the name of the directory the tarball will
# be unpacked to.
opae_dir=${1:-opae}
trans="s/opae-sdk/${opae_dir}/"

cd ${here}/../..
tar --transform=$trans \
  --exclude=_build \
  --exclude=.* \
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
  --exclude=scripts/build-documentation.sh \
  --exclude=scripts/build-pypi.sh \
  --exclude=scripts/build-tests-default.sh \
  --exclude=scripts/coverage-gtapi-mock-drv.sh  \
  --exclude=scripts/cover-ase.sh  \
  --exclude=scripts/cover-py.sh  \
  --exclude=scripts/cover.sh \
  --exclude=scripts/create-rpms.sh \
  --exclude=scripts/create-tarball.sh \
  --exclude=scripts/docker_rpm.sh \
  --exclude=scripts/index_generator.py \
  --exclude=scripts/test-build.sh \
  --exclude=scripts/test-codingstyle-all.sh \
  --exclude=scripts/test-codingstyle-cpp.sh \
  --exclude=scripts/test-gtapi-mock-drv.sh \
  --exclude=scripts/unit-tests.sh \
  --exclude=scripts/valgrind \
  --exclude=scripts/bat \
  --exclude=scripts/push-documentation.sh \
  --exclude=tools/extra/ras \
  --exclude=tools/extra/pac_hssi_config \
  --exclude=tools/extra/pyfpgadiag \
  --exclude=tools/extra/pypackager \
  --exclude=tools/utilities \
  -z -c -f ${opae_dir}.tar.gz opae-sdk
