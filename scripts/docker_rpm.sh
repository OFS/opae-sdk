#!/bin/bash
#
#

cat > .gitattributes << EOF
.git* export-ignore
.travis.yml export-ignore
opae.spec.in export-ignore
opae.spec export-ignore
opae-libs/external export-ignore
opae-libs/tests export-ignore
opae-libs/cmake/config/libopae-all.spec.in export-ignore
opae-libs/cmake/config/run_coverage_test.sh.in export-ignore
opae-libs/cmake/config/run_coverage_test_local.sh.inexport-ignore
external export-ignore
platforms export-ignore
samples/base export-ignore
samples/hello_afu export-ignore
samples/hello_mpf_afu export-ignore
samples/intg_xeon_nlb export-ignore
samples/base export-ignore
scripts export-ignore
tests export-ignore
tools/fpgametrics export-ignore
tools/libboard/board_dc export-ignore
tools/extra/ras export-ignore
tools/extra/fpgabist export-ignore
tools/extra/pac_hssi_config export-ignore
tools/extra/fpgadiag export-ignore
tools/extra/c++utils export-ignore
tools/extra/pyfpgadiag export-ignore
tools/extra/pypackager export-ignore
tools/utilities export-ignore
.clang-format export-ignore
EOF


cat > buildrpm.sh << EOF
#!/bin/bash
rpmdev-setuptree
cp /tmp/rpmbuild/opae.tar.gz ~/rpmbuild/SOURCES/.
rpmbuild -ba /tmp/rpmbuild/opae.spec
newgrp mock
mock -r fedora-rawhide-x86_64 rebuild ~/rpmbuild/SRPMS/opae-1.4.0*.src.rpm
fedora-review --rpm-spec -v -n ~/rpmbuild/SRPMS/opae-1.4.0*.src.rpm
cp ~/rpmbuild/SRPMS/*.rpm /tmp/rpmbuild/.
cp ~/rpmbuild/RPMS/*/*.rpm /tmp/rpmbuild/.
cp ~/.cache/fedora-review.log /tmp/rpmbuild/.
cp opae/review.txt /tmp/rpmbuild/.
EOF
chmod a+x buildrpm.sh

git archive --format tar --prefix opae/ --worktree-attributes HEAD | gzip > opae.tar.gz
mkdir rpmbuild
cp opae.spec rpmbuild/.
cp opae.tar.gz rpmbuild/.
cp buildrpm.sh rpmbuild/.

docker pull opae/fedora-builder:latest
docker run -v $PWD/rpmbuild:/tmp/rpmbuild --rm --cap-add SYS_ADMIN --security-opt apparmor=unconfined opae/fedora-builder:latest -c /tmp/rpmbuild/buildrpm.sh
