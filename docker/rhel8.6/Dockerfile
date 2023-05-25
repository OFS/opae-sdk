FROM centos:8
RUN yum install -y python3 python3-pip python3-devel json-c libuuid-devel gcc gcc-c++ git make cmake rpm-build systemd-devel
RUN yum install --disableplugin=subscription-manager --enablerepo=rhel-8-for-x86_64-appstream-rpms -y json-c-devel
RUN python3 -m pip install jsonschema
WORKDIR /root
COPY scripts/build-rpms-new-rhel.sh /scripts/build-rpms-new-rhel.sh
COPY scripts/test-rpms.sh /scripts/test-rpms.sh
ENTRYPOINT ["/scripts/build-rpms-new-rhel.sh"]
