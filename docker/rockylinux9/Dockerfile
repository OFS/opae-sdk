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
RUN dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml gdb vim git gcc gcc-c++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel sudo numactl-devel

RUN python3 -m pip install --user jsonschema virtualenv pudb pyyaml pybind11 && \
    # setuptools < ~51.x will fail to process the pyproject.toml successfully, so we upgrade
    # /usr is required to target the correct Python installation, as it has been found to fail when only affecting /usr/local
    /usr/bin/python3 -m pip install setuptools --upgrade --prefix /usr && \
    # the pip version has to be at least 10.0.0b1 for the build scripts to run, however the pip installed through dnf is only version 9
    # similar to setuptools, it has been found to fail when only affecting /usr/local
    /usr/bin/python3 -m pip install --upgrade pip --prefix=/usr

WORKDIR /root

COPY scripts/test-rpms.sh /scripts/test-rpms.sh

ENTRYPOINT ["/opae-rockylinux9/opae-sdk/packaging/opae/rpm/create", "unrestricted"]
