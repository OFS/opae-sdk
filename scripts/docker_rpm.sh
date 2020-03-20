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
opae-libs/plugins/ase export-ignore
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

git archive --format tar --prefix opae/ --worktree-attributes HEAD | gzip > opae.tar.gz

cat > buildrpm.sh << EOF
#!/bin/bash
rpmbuild -ba ~/rpmbuild/SPECS/opae.spec
cp ~/rpmbuild/*RPMS/*.rpm /tmp/rpmbuild/.
newgrp mock
mock -r fedora-rawhide-x86_64 rebuild ~/rpmbuild/SRPMS/opae-1.4.0*.src.rpm
fedora-review --rpm-spec -v -n ~/rpmbuild/SRPMS/opae-1.4.0*.src.rpm
EOF
chmod a+x buildrpm.sh


cat > fedora.Dockerfile << EOF
FROM fedora
RUN dnf install -y python3 python3-pip python3-devel cmake make libuuid-devel json-c-devel gcc clang vim hwloc-devel gdb doxygen python-sphinx fedora-review rpm-build rpmdevtools
RUN usermod -G mock root
RUN rpmdev-setuptree
WORKDIR /root
COPY opae.tar.gz rpmbuild/SOURCES/.
COPY opae.spec rpmbuild/SPECS/.
COPY buildrpm.sh .
ENTRYPOINT ["/bin/bash"]
CMD ["-c", "./buildrpm.sh"]
EOF

docker build . --file fedora.Dockerfile --tag opae-fedora-review:latest
mkdir rpmbuild
docker run -v $PWD/rpmbuild:/tmp/rpmbuild --rm --cap-add SYS_ADMIN --security-opt apparmor=unconfined opae-fedora-review:latest
