FROM centos:8
RUN sed -i 's/mirrorlist/#mirorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*

RUN yum install gcc

RUN yum install -y https://www.elrepo.org/elrepo-release-8.el8.elrepo.noarch.rpm
RUN rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org
RUN yum --enablerepo=elrepo-kernel install -y kernel-ml-headers

RUN yum update -y
RUN yum install -y \
        python3 \
        python3-pip \
        python3-devel \
        python3-wheel \
        python3-pybind11 \
        make \
        libuuid-devel \
        json-c-devel \
        clang \
        gcc-c++ \
        hwloc-devel \
        tbb-devel \
        rpm-build \
        rpmdevtools \
        git \
        libedit-devel \
        epel-release

RUN yum install -y \
        libudev-devel \
        libcap-devel \
        cmake3 \
        openssl11-devel

RUN /usr/bin/python3 -m pip install setuptools --upgrade --prefix /usr
RUN /usr/bin/python3 -m pip install pyyaml jsonschema --prefix=/usr

WORKDIR /root
COPY scripts/build-rpms-new-rhel.sh /scripts/build-rpms-new-rhel.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
ENTRYPOINT ["/scripts/build-rpms-new-rhel.sh"]
