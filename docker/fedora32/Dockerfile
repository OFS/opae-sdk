FROM fedora:32
RUN dnf install -y python3 python3-pip python3-devel python3-pybind11 cmake make libuuid-devel json-c-devel gcc clang gcc-c++ hwloc-devel tbb-devel rpm-build rpmdevtools git
WORKDIR /root
COPY docker/buildit.sh /buildit.sh
ENTRYPOINT [ "/buildit.sh" ]

