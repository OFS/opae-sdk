FROM centos:7.6.1810
RUN yum install -y python-devel python3 python3-pip python3-devel python3-pybind11 cmake make libuuid-devel json-c-devel gcc clang gcc-c++  vim hwloc-devel tbb-devel gdb doxygen python-sphinx fontawesome-fonts fontawesome-fonts-web python3-breathe python3-recommonmark python3-sphinx_rtd_theme fedora-review rpm-build rpmdevtools git
WORKDIR /root
COPY docker/buildit.sh /buildit.sh
ENTRYPOINT [ "/buildit.sh" ]

