FROM fedora:35
RUN dnf install -y python3 python3-pip python3-devel python3-pybind11 cmake make libuuid-devel json-c-devel gcc clang gcc-c++ hwloc-devel tbb-devel rpm-build rpmdevtools git libedit-devel libudev-devel
RUN python3 -m pip install setuptools --upgrade --prefix=/usr
RUN python3 -m pip install pyyaml jsonschema
WORKDIR /root
COPY scripts/build-rpms-new.sh /scripts/build-rpms-new.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
ENTRYPOINT ["/scripts/build-rpms-new.sh"]
