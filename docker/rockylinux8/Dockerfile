FROM rockylinux:8.5.20220308
RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled powertools
RUN dnf install -y epel-release
RUN dnf check-update || true
RUN dnf upgrade -y
RUN dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml gdb vim git gcc gcc-c++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel sudo

RUN python3 -m pip install --user jsonschema virtualenv pudb pyyaml pybind11 && \
    /usr/bin/python3 -m pip install setuptools --upgrade --prefix /usr && \
    /usr/bin/python3 -m pip install --upgrade pip --prefix=/usr

WORKDIR /root
COPY scripts/build-rpms-new.sh /scripts/build-rpms-new.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
RUN sed -i 's/fedora/unrestricted/g' /scripts/build-rpms-new.sh && \
        grep 'create unrestricted' /scripts/build-rpms-new.sh
ENTRYPOINT ["/scripts/build-rpms-new.sh"]