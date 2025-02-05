FROM rockylinux:8.6
RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y epel-release
RUN dnf check-update || true
RUN dnf upgrade -y
RUN dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml python3-setuptools python3-wheel gdb vim git gcc gcc-c++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel sudo numactl-devel

RUN python3 -m pip install --user jsonschema virtualenv pudb pyyaml pybind11 && \
    # setuptools < ~51.x will fail to process the pyproject.toml successfully, so we upgrade
    # /usr is required to target the correct Python installation, as it has been found to fail when only affecting /usr/local
    /usr/bin/python3 -m pip install setuptools --upgrade --prefix /usr && \
    # the pip version has to be at least 10.0.0b1 for the build scripts to run, however the pip installed through dnf is only version 9
    # similar to setuptools, it has been found to fail when only affecting /usr/local
    /usr/bin/python3 -m pip install --upgrade pip --prefix=/usr

WORKDIR /root

COPY scripts/test-rpms.sh /scripts/test-rpms.sh

ENTRYPOINT ["/opae-rockylinux8/opae-sdk/packaging/opae/rpm/create", "unrestricted"]
