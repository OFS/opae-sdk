# OPAE Installation Guide #

## How to download the OPAE SDK ##
OPAE SDK releases are available on [GitHub](https://github.com/OPAE/opae-sdk/releases).
Source code for the OPAE DFL device driver for Linux is also available on [GitHub](https://github.com/OPAE/linux-dfl).

## Install the Fedora  ##
Download the Fedora  (x86_64 version) installation file in [fedora](https://getfedora.org/en/workstation/download/), and install the Fedora  in yourserver. You can choose Fedora Workstation or Fedora server.

## Build the kernel and DFL drivers ##

For building the OPAE kernel and kernel driver, the kernel development environment is required. So before you build the kernel, you must install the required packages. Run the following commands:

```console
$ sudo dnf install gcc gcc-c++ make kernel-headers kernel-devel elfutils-libelf-devel ncurses-devel openssl-devel bison flex
```

Download the OPAE upstream kernel tree from github, for example download from fpga-ofs-dev-5.15-lts branch.
```console
$ git clone https://github.com/OPAE/linux-dfl.git -b fpga-ofs-dev-5.15-lts
```

Configure the kernel.
```console
$ cd linux-dfl
$ cp /boot/config-`uname -r` .config
$ cat configs/dfl-config >> .config
$ echo 'CONFIG_LOCALVERSION="-dfl"' >> .config
$ echo 'CONFIG_LOCALVERSION_AUTO=y' >> .config
$ sed -i -r 's/CONFIG_SYSTEM_TRUSTED_KEYS=.*/CONFIG_SYSTEM_TRUSTED_KEYS=""/' .config
$ sed -i '/^CONFIG_DEBUG_INFO_BTF/ s/./#&/' .config
$ echo 'CONFIG_DEBUG_ATOMIC_SLEEP=y' >> .config
$ make olddefconfig
```

Compile and install the new kernel.
```console
$ make -j $(nproc)
$ sudo make modules_install -j $(nproc)
$ sudo make install
```

Build linux DFL Kernel instructions please also refer to: https://github.com/OPAE/linux-dfl/wiki/Build-the-linux-dfl-kernel

When install finished, reboot your system.
When the system login again, verify the kernel version is correct. For example:
```console
[figo@localhost linux-dfl]$ uname -a
Linux localhost.localdomain 5.15.lts-dfl-g73e16386cda0 #6 SMP Mon Jun 13 21:21:31 -04 2022 x86_64 x86_64 x86_64
```

And also you can check the OPAE dfl drivers have auto-loaded.
```console
[figo@localhost linux-dfl]$ lsmod | grep fpga
ifpga_sec_mgr          20480  1 intel_m10_bmc_secure
fpga_region            20480  3 dfl_fme_region,dfl_fme,dfl
fpga_bridge            24576  4 dfl_fme_region,fpga_region,dfl_fme,dfl_fme_br
fpga_mgr               16384  4 dfl_fme_region,fpga_region,dfl_fme_mgr,dfl_fme
[figo@localhost linux-dfl]$ lsmod | grep dfl
dfl_eth_group          36864  0
dfl_fme_region         20480  0
dfl_emif               16384  0
dfl_n3000_nios         20480  0
dfl_fme_br             16384  0
dfl_fme_mgr            20480  1
dfl_fme                49152  0
dfl_afu                36864  0
dfl_pci                20480  0
dfl                    40960  7 dfl_pci,dfl_fme,dfl_fme_br,dfl_eth_group,dfl_n3000_nios,dfl_afu,dfl_emif
fpga_region            20480  3 dfl_fme_region,dfl_fme,dfl
fpga_bridge            24576  4 dfl_fme_region,fpga_region,dfl_fme,dfl_fme_br
fpga_mgr               16384  4 dfl_fme_region,fpga_region,dfl_fme_mgr,dfl_fme
```

## Build the OPAE-SDK ##
Before you build the OPAE SDK, you must install the required packages. Run the following commands:

### Rocky Linux 8.5 ###

```console
# dnf install -y 'dnf-command(config-manager)'
# dnf config-manager --set-enabled powertools
# dnf install -y epel-release
# dnf check-update
# dnf upgrade -y
# dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml python3-pybind11 git gcc gcc-c++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel libedit-devel systemd-devel rpm-build rpmdevtools pybind11-devel yaml-cpp-devel libudev-devel linuxptp
# python3 -m pip install jsonschema virtualenv pyyaml
```

### Fedora  ###

```console
# dnf check-update
# dnf upgrade -y
# dnf install -y python3 python3-pip python3-devel python3-jsonschema python3-pyyaml python3-pybind11 git gcc g++ make cmake libuuid-devel json-c-devel hwloc-devel tbb-devel libedit-devel rpm-build rpmdevtools pybind11-devel yaml-cpp-devel libudev-devel cli11-devel spdlog-devel linuxptp
# pip3 install jsonschema virtualenv pyyaml
```

### Ubuntu 20.04 ###

```console
# apt-get update
# apt-get upgrade -y
# apt-get install -y python3 python3-pip python3-dev git gcc g++ make cmake uuid-dev libjson-c-dev libhwloc-dev libtbb-dev libedit-dev libudev-dev linuxptp pandoc devscripts debhelper doxygen
# pip3 install jsonschema virtualenv pyyaml pybind11
```

### RHEL 8.2 ###
Register and enable Red Hat subscription to install any packages on the system.

```console
# subscription-manager register --proxy=PROXY --username=USER --password=PASSWORD --auto-attach
```

Set the RHEL version and install packages. Set proxy name and port number.

```console
# subscription-manager release --set=8.2 --proxy proxy-name.com:port number
# subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms
# dnf upgrade -y
# dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
# dnf install -y python3 python3-pip python3-devel gdb vim git gcc gcc-c++ make cmake libuuid-devel rpm-build systemd-devel  nmap
# dnf install -y python3-jsonschema json-c-devel tbb-devel rpmdevtools libcap-devel 
# dnf check-update || true
# dnf install -y spdlog-devel cli11-devel python3-pyyaml python3-pybind11 hwloc-devel libedit-devel
# python3 -m pip install --user jsonschema virtualenv pudb pyyaml
```

Install the latest version of [cmake](https://github.com/Kitware) on top of the outdated cmake package from the package manager.

```console
# cd cmake-3.25.1/
# ./bootstrap --prefix=/usr
# make
# make install
# which cmake
/usr/bin/cmake
```

### Create opae-sdk packages ###

Download the OPAE-SDK source code from github. For example, download from Master branch.

```console
$ git clone https://github.com/OPAE/opae-sdk.git
```

Compile and build the OPAE-SDK RPMs (Fedora, Rocky, RHEL 8.2).
```console
$ cd opae-sdk/packaging/opae/rpm
$ ./create fedora
```



After a successful compile, there are 3 rpm packages generated (Fedora, Rocky, RHEL8.2). For example:
```console
opae-2.1.0-1.fc34.x86_64.rpm
opae-devel-2.1.0-1.fc34.x86_64.rpm
opae-extra-tools-2.1.0-1.fc34.x86_64.rpm
```

Compile and build the OPAE-SDK deb packages (Ubuntu 22.04).
```console
$ cd opae-sdk/packaging/opae/deb
$ ./create
```

After a successful compile, there are 3 deb packages generated (Ubuntu 22.04). For example:
```console
opae_2.1.1-1_amd64.deb  
opae-devel_2.1.1-1_amd64.deb  
opae-extra-tools_2.1.1-1_amd64.deb
```



## OPAE SDK installation with rpm/deb packages ##
The rpm packages generated in the previous step can be installed using these commands:

```console
$ sudo dnf install ./*.rpm
```

The deb packages generated in the previous step can be installed using these commands:

```console
$ sudo dpkg -i  ./*.deb
```


When you installed the rpms, you can run fpgainfo command to check the FPGA FME infomation. For example:
```console
[figo@localhost install_guide]$ fpgainfo fme
Board Management Controller, MAX10 NIOS FW version: D.2.1.24
Board Management Controller, MAX10 Build version: D.2.0.7
//****** FME ******//
Object Id                        : 0xEF00000
PCIe s:b:d.f                     : 0000:08:00.0
Device Id                        : 0x0B30
Socket Id                        : 0x00
Ports Num                        : 01
Bitstream Id                     : 0x2300011001030F
Bitstream Version                : 0.2.3
Pr Interface Id                  : f3c99413-5081-4aad-bced-07eb84a6d0bb
Boot Page                        : user
```

To uninstall the OPAE rpms, you can use this commands
```console
$ dnf list installed | grep opae
$ sudo dnf remove opae*.x86_64
```

To uninstall the OPAE deb, you can use this commands
```console
$ dpkg -l  | grep opae
$ dpkg -r opae-extra-tools:amd64
$ dpkg -r opae-devel:amd64
$ dpkg -r opae
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
