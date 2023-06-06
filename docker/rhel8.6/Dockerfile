FROM centos:8
RUN sed -i 's/mirrorlist/#mirorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's/enabled=0/enabled=1/g' /etc/yum.repos.d/CentOS-Linux-PowerTools.repo

RUN yum -y install epel-release epel-next-release

RUN yum update -y
RUN yum install -y \
        make \
        cmake3 \
        git \
        gcc \
        gcc-c++ \
        json-c-devel \
        libuuid-devel \
        python3 \
        python3-devel \
        python3-jsonschema \
        python3-pip \
        rpm-build \
        systemd \
        rpmdevtools \
#        
        python3-wheel \
        python3-pyyaml \
        hwloc-devel \
        tbb-devel \
        libudev-devel \
        libcap-devel \
        cli11-devel \
        spdlog-devel \
        libedit-devel \
        systemd-devel \
        clang 

RUN /usr/bin/python3 -m pip install setuptools --upgrade --prefix /usr
RUN /usr/bin/python3 -m pip install pyyaml jsonschema --prefix=/usr

RUN /usr/bin/python3 -m pip install --upgrade pip --prefix=/usr

RUN yum -y install create-fake-rpm
RUN /usr/bin/python3 -m pip install pybind11 --prefix=/usr
RUN create-fake-rpm --build python3-pybind11 python3-pybind11
RUN mv /noarch/fake-python3-pybind11-0-0.noarch.rpm /noarch/python3-pybind11-0-0.noarch.rpm
RUN yum -y install /noarch/python3-pybind11-0-0.noarch.rpm

WORKDIR /root
COPY scripts/build-rpms-new.sh /scripts/build-rpms-new.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
RUN sed -i 's/fedora/unrestricted/g' /scripts/build-rpms-new.sh
ENTRYPOINT ["/scripts/build-rpms-new.sh"]
