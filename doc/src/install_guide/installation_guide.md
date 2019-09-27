# OPAE Installation Guide #

.. toctree::

.. highlight:: c

.. highlight:: console


## System compatibility ##

The OPAE SDK has been tested on the following configurations.

* Hardware: Tightly coupled FPGA products and programmable FPGA acceleration
  cards for Intel&reg; Xeon&reg; processors
* Operating System: Tested on Red Hat Enterprise Linux 7.3 and 7.4, Ubuntu 16.04, SUSE SLE 12 SP3 and CentOS 7.4, with Linux kernels 3.10 through 4.7
* Integrated FPGA FIM (FPGA Interface Manager) version: 6.4.0
* Arria(R) 10 GX FPGA FIM version: 1.0.3 (1.0 Production)

## How to download the OPAE SDK ##

OPAE SDK releases are available on
[GitHub](https://github.com/OPAE/opae-sdk/releases). There, you can also find
the driver source code and DKMS packages for the respective SDK release.

The various componentes of OPAE are available via the following compressed tar files and RPM packages.

* Source packages for the SDK and the drivers

```console
opae-sdk-<release>.tar.gz                   (BSD License)   (all src for libopae-c, tools, samples, headers )
opae-sdk-<release>.zip                      (BSD License)   (ZIP archive, same content as opae-sdk-<release>.tar.gz)
opae-intel-fpga-driver-<release>.tar.gz     (GPLv2 License) (driver sources)
```

* Binary package for the drivers

```console
opae-intel-fpga-drv-<release>-1.x86_64.rpm  (GPLv2 License) (dkms and driver src to generate \*.ko at installation)
```

## Software requirements ##

For building the kernel driver, the kernel development environment is required.

* gcc                       >= 4.8.5
* cmake                     >= 2.8
* dkms.noarch              (Release is tested with 2.2.0.3-34)

For building libopae-c, tools and samples, the following dependences are required:

* libuuid-devel.x86\_64:   (tested with 2.23.2-33.el7)
* libuuid.x86\_64:         (tested with 2.23.2-33.el7)
* json-c-devel.x86\_64:    json-c-devel-0.11-4.el7\_0.x86\_64.rpm
* json-c.x86\_64:          (tested with 0.11-4.el7\_0)
* cmake.x86\_64:           (tested with 2.8.12.2-2.el7)
* libhwloc.x86\_64

## Driver installation with DKMS rpm and deb packages ##

Install rpm package:
```console
$ sudo yum install opae-intel-fpga-driver-<release>.x86_64.rpm
```

Uninstall rpm package:
```console
$ sudo yum remove opae-intel-fpga-driver-<release>
```

Install deb package:
```console
$ sudo dpkg -i opae-intel-fpga-driver_<release>_amd64.deb
```

Uninstall deb package:
```console
$ sudo dpkg -r opae-intel-fpga-driver
```

During the installation process, the tool will compile the driver from
source then install the driver automatically. Driver installed by these packages
will be automatically install again after system reboot.

## Driver build/installation with driver source package ##

Using the following command to untar the source tar ball:

```console
$ tar zxvf opae-intel-fpga-driver-<release>.tar.gz
```

Following directory shall be extracted at the working directory where the above command is executed.

* `opae-intel-fpga-driver-<release>`

Build the fpga driver from source with following procedures:

```console
$ cd opae-intel-fpga-driver-<release>
$ make
```

Following kernel modules shall be generated from source:

* fpga-mgr-mod.ko
* intel-fpga-afu.ko
* intel-fpga-fme.ko
* intel-fpga-pci.ko

Install the above modules in the following order:

```console
$ sudo insmod fpga-mgr-mod.ko
$ sudo insmod intel-fpga-pci.ko
$ sudo insmod intel-fpga-fme.ko
$ sudo insmod intel-fpga-afu.ko
```

Use lsmod to check if all 4 modules are installed correctly or not:

```console
$ lsmod | grep fpga
```

Output should look like:

```console
intel_fpga_fme         36864  0
intel_fpga_afu         28672  0
intel_fpga_pci         28672  2 intel_fpga_afu,intel_fpga_fme
fpga_mgr_mod           16384  1 intel_fpga_fme
```

Remove the driver modules in the following order:

```console
$ sudo rmmod intel-fpga-afu
$ sudo rmmod intel-fpga-fme
$ sudo rmmod intel-fpga-pci
$ sudo rmmod fpga-mgr-mod
```

## Manual Driver build from RPM package ##
Use the following command to extract the driver source files from the rpm:

```console
$ mkdir opae-intel-fpga-driver-<release>
$ cd opae-intel-fpga-driver-<release>
$ rpm2cpio ../opae-intel-fpga-driver-<release>.rpm | cpio -idmv
```

Build the fpga driver from source with the following procedure:

```console
$ cd ./usr/src/intel-fpga-<release>
$ make
```

## OPAE SDK build/installation from OPAE SDK source ##
Using the following command to untar the source tar ball:

```console
$ tar zxvf opae-sdk-<release>.tar.gz
```

Following directory shall be created at the working directory where the above command is executed.

* `opae-sdk-<release>`

Build the OPAE C library (libopae-c), samples, tools with the following commands:

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake .. 
$ make
```

By default, the OPAE SDK will install into `/usr/local` if you also issue the following:

```console
$ make install
```

You can change this installation prefix from `/usr/local` into something else
by adding `-DCMAKE_INSTALL_PREFIX=<new prefix>` to the `cmake` command above.

Please see Quick Start Guide on how to run the hello\_fpga sample to verify
libopae-c & driver are built correctly.

## Building python distributions for tools ##

The tools that can be built with python distutils are:
 - packager
 - fpgadiag

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake .. -DBUILD_PYTHON_DIST=ON
$ make <toolname>-dist
```
The python distributions will be available in mybuild/<tools-directory>/<toolname>/stage/dist

## Building OPAE SDK rpm and deb packages from the source ##
In addition to building and installation from the source, users can also
generate rpm and deb packages for the SDK. The generated packages can then be
distributed to other users for easy installation. The advantage of this approach
is that the other users do not need to have the build toolchain on their
systems to install the OPAE SDK.

* To build rpm packages follow these steps:

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake ..  -DCPACK_GENERATOR=RPM -DCMAKE_INSTALL_PREFIX=<desired install loacation>
$ make package_rpm
```
.. note::
```
Note: Providing CMAKE_INSTALL_PREFIX is optional, by default the install prefix will be /usr/local.
```
This will generate the following rpm packages. 

```console
opae-<release>.x86_64.rpm               (meta package)
opae-libs-<release>.x86_64.rpm          (libopae-c and samples)
opae-tools-<release>.x86_64.rpm         (base tools)
opae-tools-extra-<release>.x86_64.rpm   (extra tools)
opae-devel-<release>.x86_64.rpm         (headers)
```

* To build deb packages follow these steps:

 .. note::
  ```
  Note: For generating deb packages, cmake version 3.0.0 and above is required.
  ```

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake ..  -DCPACK_GENERATOR=DEB -DCMAKE_INSTALL_PREFIX=<desired install loacation>
$ make package_deb
```
.. note::
```
Note: Providing CMAKE_INSTALL_PREFIX is optional, by default the install prefix will be /usr/local.
```
This will generate the following deb packages.

```console
opae-libs-<release>.x86_64.deb          (libopae-c and samples)
opae-tools-<release>.x86_64.deb         (tools)
opae-tools-extra-<release>.x86_64.deb   (tools)
opae-devel-<release>.x86_64.deb         (headers)
```

## OPAE SDK installation with rpm packages ##
The rpm packages generated in the previous step can be installed
using these commands:

```console
$ sudo yum install opae-<release>.x86_64.rpm
$ sudo yum install opae-libs-<release>.x86_64.rpm
$ sudo yum install opae-tools-<release>.x86_64.rpm
$ sudo yum install opae-tools-extra-<release>.x86_64.rpm
$ sudo yum install opae-devel-<release>.x86_64.rpm
```

```eval_rst
.. note:
    If you want to install all the packages, you can also do:
    $ sudo yum install opae-*.rpm
```
To uninstall:

```console
$ sudo yum remove opae
$ sudo yum remove opae-libs
$ sudo yum remove opae-tools
$ sudo yum remove opae-tools-extra
$ sudo yum remove opae-devel
```

## OPAE SDK installation with deb packages ##
The deb packages generated in the previous step can be installed
using these commands:

```console
$ sudo dpkg -i opae-libs-<release>.x86_64.deb 
$ sudo dpkg -i opae-tools-<release>.x86_64.deb
$ sudo dpkg -i opae-tools-extra-<release>.x86_64.deb
$ sudo dpkg -i opae-devel-<release>.x86_64.deb
```

```eval_rst
.. note:
    If you want to install all the packages, you can also do:
    $ sudo dpkg -i opae-*.deb
```
To uninstall:

```console
$ sudo dpkg -r opae-libs
$ sudo dpkg -r opae-tools
$ sudo dpkg -r opae-tools-extra
$ sudo dpkg -r opae-devel
```

## FPGA Device Access Permissions ##

Access to FPGA accelerators and devices is controlled using file access permissions on the 
Intel&reg; FPGA device files, `/dev/intel-fpga-fme.*` and `/dev/intel-fpga-port.*`, as well as to the files reachable through `/sys/class/fpga/`.

In order to allow regular (non-root) users to access accelerators, you need to grant them read and write permissions on `/dev/intel/fpga-port.*` (with `*` denoting the respective socket, i.e. 0 or 1). E.g.:

```console
$ sudo chmod a+rw /dev/intel-fpga-port.0
```

## Memlock limit ##

Depending on the requirements of your application, you may also want to
increase the maximum amount of memory a user process is allowed to lock. The
exact way to do this depends on your Linux distribution.

You can check the current memlock limit using

```console
$ ulimit -l
```

A way to permanently remove the limit for locked memory for a regular user is
to add the following lines to your /etc/security/limits.conf:

```console
user1    hard   memlock           unlimited
user1    soft   memlock           unlimited
```

This removes the limit on locked memory for user `user1`. To remove it for
all users, you can replace `user1` with `*`:

```console
*    hard   memlock           unlimited
*    soft   memlock           unlimited
```

Note that settings in the /etc/security/limits.conf file don't apply to
services.  To increase the locked memory limit for a service you need to
modify the application's systemd service file and add the line:

```console
[Service]
LimitMEMLOCK=infinity
```

## Hugepage Settings ##

Users need to configure system hugepage to reserve 2MB-hugepages or
1GB-hugepages. For example, the 'hello\_fpga' sample requires several
2MB-hugepages. And the _fpgadiag_ tool requires several 1GB-hugepages.
    
The command below reserves 20 2M-hugepages: 

```console
$ sudo sh -c 'echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages'
```

The command below reserves 4 1GB-hugepages:

```console
$ sudo sh -c 'echo 4 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages'
```


For x86\_64 architecture processors, user can use following command to find out avaiable hugepage sizes:

```console
$ grep pse /proc/cpuinfo | uniq
flags : ... pse ...
```

If this command returns a non-empty string, 2MB pages are supported.

```console
$ grep pdpe1gb /proc/cpuinfo | uniq
flags : ... pdpe1gb ...
```

If this command returns a non-empty string, 1GB pages are supported.

