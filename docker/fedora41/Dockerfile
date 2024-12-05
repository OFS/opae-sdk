FROM fedora:41
RUN dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml python3-pybind11 python3-setuptools python3-wheel git make cmake libuuid-devel json-c-devel gcc clang gcc-c++ libuuid-devel json-c-devel hwloc-devel tbb-devel libedit-devel rpm-build rpmdevtools pybind11-devel python3-virtualenv yaml-cpp-devel libudev-devel cli11-devel spdlog-devel systemd numactl-devel
WORKDIR /root
COPY scripts/build-rpms-new.sh /scripts/build-rpms-new.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
ENTRYPOINT ["/scripts/build-rpms-new.sh"]
