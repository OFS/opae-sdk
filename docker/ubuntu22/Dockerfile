FROM ubuntu:22.04
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y python3 python3-pip python3-dev python3-pybind11 git gcc g++ make cmake doxygen pandoc uuid-dev libjson-c-dev libhwloc-dev libtbb-dev libedit-dev libspdlog-dev libcli11-dev libudev-dev bsdmainutils devscripts debhelper dh-python doxygen libnuma-dev
RUN python3 -m pip install setuptools --upgrade --prefix=/usr
RUN python3 -m pip install pyyaml jsonschema pybind11
WORKDIR /root
COPY scripts/build-debs.sh /scripts/build-debs.sh
COPY scripts/test-debs.sh /scripts/test-debs.sh
ENTRYPOINT ["/scripts/build-debs.sh"]
