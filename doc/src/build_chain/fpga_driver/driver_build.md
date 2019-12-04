Building the OPAE Intel FPGA driver (out-of-tree)
============================================

The Intel FPGA driver included with OPAE SDK releases is packaged as an RPM or
DEB package as well as a source tarball. Starting with OPAE SDK release of 1.4,
the driver can be built from source out-of-tree but require the following
packages:

For RPM package managers (Red Hat, CentOS, Fedora, etc.)
* kernel-headers
* kernel-devel
* gcc
* make

For DEB package managers (Debian, Ubuntu, etc.)
* kernel-headers-generic
* gcc
* make

After installation necessary distribution packages, follow the steps in the
example below to build the Intel Kernel driver.
_NOTE_ The example below references Intel FPGA Kernel driver version 2.0.2. but
can be applied to later versions.


``` {.bash}

tar zxf opae-intel-fpga-driver-2.0.2-1.tar.gz
cd opae-intel-fpga-driver-2.0.2
make

```

