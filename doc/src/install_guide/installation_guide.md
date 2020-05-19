# OPAE Installation Guide #

.. toctree::

.. highlight:: c

.. highlight:: console


## System compatibility ##

The OPAE SDK has been tested on the following configurations.

* Hardware: Intel(R) FPGA Programmable Acceleration Cards: Arria(R) 10 GX, D5005, and N3000.
* Operating System: Tested on Fedora 31, with Linux kernel 5.7.
* Arria&reg 10 GX FPGA FIM version: 1.0.3 (1.0 Production)

## How to download the OPAE SDK ##

OPAE SDK releases are available on [GitHub](https://github.com/OPAE/opae-sdk/releases).
Source code for the OPAE DFL device driver for Linux is also available on [GitHub](https://github.com/OPAE/linux-dfl).

The various componentes of OPAE are available via the following compressed tar files and RPM packages.

* Source packages for the SDK and the drivers

```console
opae-sdk-<release>.tar.gz                   (BSD License)   (all src for libopae-c, tools, samples, headers and ASE)
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

## OPAE SDK build/installation from OPAE SDK source ##
Using the following command to untar the source tar ball:

```console
$ tar zxvf opae-sdk-<release>.tar.gz
```

Following directory shall be created at the working directory where the above command is executed.

* `opae-sdk-<release>`

Build the OPAE C library (libopae-c), samples, tools, and the AFU Simulation Environment (ASE)
library (libopae-c-ase) with the following commands:

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake .. -DOPAE_BUILD_SIM=ON
$ make
```

By default, the OPAE SDK will install into `/usr/local` if you also issue the following:

```console
$ make install
```

You can change this installation prefix from `/usr/local` into something else
by adding `-DCMAKE_INSTALL_PREFIX=<new prefix>` to the `cmake` command above.

Please see Quick Start Guide on how to run the hello\_fpga sample to verify
libopae-c and driver are built correctly.

## Building python distributions for tools ##

The tools that can be built with python distutils are:
 - packager
 - fpgaflash
 - fpgadiag

```console
$ cd opae-sdk-<release>
$ mkdir mybuild
$ cd mybuild
$ cmake .. -DOPAE_BUILD_PYTHON_DIST=ON
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
$ cmake .. -DOPAE_BUILD_SIM=ON -DCPACK_GENERATOR=RPM -DCMAKE_INSTALL_PREFIX=<desired install location>
$ make package_rpm
```
.. note::
```
Note: Providing CMAKE_INSTALL_PREFIX is optional, by default the install prefix will be /usr.
```
This will generate the following rpm packages.

```console
opae-<release>.x86_64.rpm               (meta package)
opae-libs-<release>.x86_64.rpm          (libopae-c and samples)
opae-tools-<release>.x86_64.rpm         (base tools)
opae-tools-extra-<release>.x86_64.rpm   (extra tools)
opae-devel-<release>.x86_64.rpm         (headers)
opae-ase-<release>.x86_64.rpm           (libopae-c-ase)
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
$ cmake .. -DOPAE_BUILD_SIM=ON -DCPACK_GENERATOR=DEB -DCMAKE_INSTALL_PREFIX=<desired install location>
$ make package_deb
```
.. note::
```
Note: Providing CMAKE_INSTALL_PREFIX is optional, by default the install prefix will be /usr.
```
This will generate the following deb packages.

```console
opae-libs-<release>.x86_64.deb          (libopae-c and samples)
opae-tools-<release>.x86_64.deb         (tools)
opae-tools-extra-<release>.x86_64.deb   (tools)
opae-devel-<release>.x86_64.deb         (headers)
opae-ase-<release>.x86_64.deb           (libopae-c-ase)
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
$ sudo yum install opae-ase-<release>.x86_64.rpm
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
$ sudo yum remove opae-ase
```

## OPAE SDK installation with deb packages ##
The deb packages generated in the previous step can be installed
using these commands:

```console
$ sudo dpkg -i opae-libs-<release>.x86_64.deb
$ sudo dpkg -i opae-tools-<release>.x86_64.deb
$ sudo dpkg -i opae-tools-extra-<release>.x86_64.deb
$ sudo dpkg -i opae-devel-<release>.x86_64.deb
$ sudo dpkg -i opae-ase-<release>.x86_64.deb
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
$ sudo dpkg -r opae-ase
```

## FPGA Device Access Permissions ##

Access to FPGA accelerators and devices is controlled using file access permissions on the
Intel&reg; FPGA device files, `/dev/dfl-fme.*` and `/dev/dfl-port.*`, as well as to the files reachable through `/sys/class/fpga_region/`.

In order to allow regular (non-root) users to access accelerators, you need to grant them read and write permissions on `/dev/dfl-port.*` (with `*` denoting the respective socket, i.e. 0 or 1). E.g.:

```console
$ sudo chmod a+rw /dev/dfl-port.0
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

Users need to configure system hugepages to reserve 2MB-hugepages or
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

If this commands returns a non-empty string, 2MB pages are supported.

```console
$ grep pse /proc/cpuinfo | uniq
flags : ... pdpe1gb ...
```

If this commands returns a non-empty string, 1GB pages are supported.
