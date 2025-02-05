ARG tag=9.4
# Pull non-official image to retrieve archived versions that were
# removed from the official library, such as rockylinux:8.4.
# https://forums.rockylinux.org/t/rockylinux-8-4-not-in-official-rockylinux-docker-images-tag-list/5248
FROM rockylinux/rockylinux:${tag}
# https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact
ARG tag
# When ${tag} is explicitly given as a minor version, e.g., 9.4,
# install packages from the vault repositories instead of the default
# mirrors. Otherwise, packages will be upgraded or installed from the
# specified to the latest minor release, e.g., 9.4 is upgraded to 9.5.
RUN sed -i -e '/^mirrorlist/s,^,#,' -e "s,^#\\?\\(baseurl=http://dl.rockylinux.org\\)/\$contentdir/\$releasever/,\\1/vault/rocky/${tag}/," /etc/yum.repos.d/rocky*.repo
RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled crb
RUN dnf install -y epel-release
RUN dnf check-update || true
RUN dnf upgrade -y
RUN dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml python3-pybind11 python3-setuptools python3-wheel gdb vim git gcc gcc-c++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel numactl-devel
WORKDIR /root

COPY scripts/test-rpms.sh /scripts/test-rpms.sh

ENTRYPOINT ["/opae-rockylinux9/opae-sdk/packaging/opae/rpm/create", "unrestricted"]
