FROM fedora:31
RUN dnf install -y python3 python3-pip python3-devel python3-pybind11 cmake make libuuid-devel json-c-devel gcc clang gcc-c++ hwloc-devel tbb-devel rpm-build rpmdevtools git
RUN dnf install -y libedit-devel
WORKDIR /root
COPY docker/buildit.sh /buildit.sh
ENTRYPOINT [ "/buildit.sh" ]

