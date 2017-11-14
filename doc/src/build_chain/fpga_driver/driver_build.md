Building the OPAE Intel FPGA driver (in-tree)
=============================================

``` {.bash}

git clone ssh://«Place your username here»@git-amr-1.devtools.intel.com:29418/cpt_sys_sw-intel-fpga
cd cpt_sys_sw-intel-fpga
git config user.name "«Place your name here»"
git config user.email "«Place your email here»"
curl -k https://git-amr-1.devtools.intel.com/gerrit/tools/hooks/commit-msg -o .git/hooks/commit-msg
git checkout fpga-all
git pull

make oldconfig
make prepare
make modules SUBDIRS=drivers/fpga

```

.. warning::

```
  Kernel modules built using in-tree building cannot *insmod’d* or
  *modprobe’d* into the currently running kernel (e.g. in-tree building
  the Intel FPGA driver when running on a workstation with different
  kernel); unless the currently running kernel is the same than kernel on
  Intel FPGA driver repository.
```

Building the OPAE Intel FPGA driver (out-of-tree)
============================================

Steps
-----

1.  Fetch the Intel FPGA kernel repository
2.  Fetch the Intel FPGA internal tools repository
3.  Configure the Intel FPGA internal tools project
4.  Build the driver
5.  Build distribution package (tarball)
6.  Build RPM package (RPM)
7.  Insert the driver into currently running kernel

.. note::

```
  The resulting kernel objects has only been tested for correct functionality
  between kernel 3.10 up to kernel 4.10.
```

Fetch the Intel FPGA kernel repository
--------------------------------------

``` {.bash}

git clone ssh://«Place your username here»@git-amr-1.devtools.intel.com:29418/cpt_sys_sw-intel-fpga
cd cpt_sys_sw-intel-fpga
git config user.name "«Place your name here»"
git config user.email "«Place your email here»"
curl -k https://git-amr-1.devtools.intel.com/gerrit/tools/hooks/commit-msg -o .git/hooks/commit-msg
git checkout feature/cmake
git pull

```

Fetch the Intel FPGA internal tools repository
----------------------------------------------

``` {.bash}

git clone ssh://«Place your username here»@git-amr-1.devtools.intel.com:29418/cpt_sys_sw-fpga-internal
cd cpt_sys_sw-fpga-internal
git config user.name "«Place your name here»"
git config user.email "«Place your email here»"
curl -k https://git-amr-1.devtools.intel.com/gerrit/tools/hooks/commit-msg -o .git/hooks/commit-msg
git checkout develop
git pull

```

Configure the Intel FPGA internal tools project
-----------------------------------------------

``` {.bash}

cd cpt_sys_sw-fpga-internal
cd fpga_driver/fpga-kernel
mkdir mybuild
cd mybuild
cmake .. «user configuration flags»

```

Valid «user configuration flags» are:

  cmake flag                                                             Optional or mandatory   Purpose                       Default value
  ---------------------------------------------------------------------- ----------------------- ----------------------------- ---------------
  -DINTEL_FPGA_DRIVER_VER_MAJOR                                          Optional                Driver major version          0
  -DINTEL_FPGA_DRIVER_VER_MINOR                                          Optional                Driver minor version          1
  -DINTEL_FPGA_DRIVER_VER_REV                                            Optional                Driver revision version       0
  -DKERNEL_SOURCE_DIR=«path to the root of the FPGA kernel repository»   Mandatory               Path for driver source code   None

Build the driver
----------------

``` {.bash}

cd cpt_sys_sw-fpga-internal
cd fpga_driver/fpga_kernel
cd mybuild # (created during previous step)
make «user target»

```

Valid «user targets» are:

  make target                Purpose
  -------------------------- --------------------------------------------------------------------------------------
  make                       Compiles the driver
  make dist                  Creates distributable driver tarball intel-fpga-dkms_0.1.0.tar.gz
  make intel-fpga-dkms-deb   Create Debian DKMS-enabled driver installer package: intel-fpga-dkms_0.1.0_amd64.deb
  make intel-fpga-dkms-rpm   Create Redhat DKMS-enabled driver installer package

Inserting the driver into currently running kernel
--------------------------------------------------

After building the driver (previous step), driver can be inserted into
the currently running kernel.

``` {.bash}

cd cpt_sys_sw-fpga-internal
cd fpga_driver/fpga_kernel
cd mybuild # (created during previous step)
sudo insmod ./drivers/fpga/fpga-mgr-mod.ko
sudo insmod ./drivers/fpga/intel/intel-fpga-pci.ko
sudo insmod ./drivers/fpga/intel/intel-fpga-afu.ko
sudo insmod ./drivers/fpga/intel/intel-fpga-fme.ko

```

.. warning::

```
   Before running applications; system administrator should request OS to enable allocation of 2,048kB memory pages. System adminitrator ('root') should run following command for such purpose:

   echo 20 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```
