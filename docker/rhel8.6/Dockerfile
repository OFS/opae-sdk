FROM centos:8
RUN sed -i 's/mirrorlist/#mirorlist/g' /etc/yum.repos.d/CentOS-*
RUN sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
RUN yum update -y
RUN yum install -y \
        python3 \
        python3-pip \  
        python3-devel \
        json-c \
        libuuid-devel \
        gcc \
        gcc-c++ \
        git \
        make \
        cmake \
        rpm-build \
        rpmdevtools \
        systemd-devel \
        libncurses* \
        hwloc-devel
RUN yum install -y json-c-devel python3-jsonschema
RUN python3 -m pip install jsonschema
WORKDIR /root
COPY scripts/build-rpms-new-rhel.sh /scripts/build-rpms-new-rhel.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
ENTRYPOINT ["/scripts/build-rpms-new-rhel.sh"]
