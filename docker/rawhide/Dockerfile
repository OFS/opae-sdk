ARG FEDORA_TAG=rawhide
FROM fedora:$FEDORA_TAG
RUN dnf update -y
RUN dnf install -y git gcc gcc-c++ clang make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel libedit-devel doxygen python3 python3-pip python3-devel python3-pybind11 python3-sphinx python3-virtualenv systemd rpm-build rpmdevtools
ENTRYPOINT [ "/bin/bash", "-c" ]
