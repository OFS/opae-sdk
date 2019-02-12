Open Programmable Acceleration Engine (OPAE) is a software framework for managing and accessing programmable accelerators (FPGAs). Its main parts are:

-   OPAE Software Development Kit (OPAE SDK),

-   OPAE Linux driver for Intel(R) Xeon(R) CPU with Integrated FPGAs and Intel(R) PAC with Arria(R) 10 GX FPGA

-   Basic Building Block (BBB) library for accelerating AFU
    development (not part of this release, but pre-release code is
    available on GitHub: https://github.com/OPAE/intel-fpga-bbb

OPAE is under active development to extend to more hardware platforms, as well as to build up the software stack with additional abstractions to enable more software developers.

OPAE SDK is a collection of libraries and tools to facilitate the development of software applications and accelerators using OPAE. It provides a library implementing the OPAE C API for presenting a streamlined and easy-to-use interface for software applications to discover, access, and manage FPGA devices and accelerators using the OPAE software stack. The OPAE SDK also includes the AFU Simulation Environment (ASE) for end-to-end simulation of accelerator RTL together with software applications using the OPAE C API.

OPAE\'s goal is to accelerate FPGA adoption. It is a community effort to simplify the development and deployment of FPGA applications, so we explicitly welcome discussions and contributions! The OPAE SDK source, unless otherwise noted, is released under a BSD 3-clause license.

More information about OPAE can be found
at http://01.org/OPAE.

Open Programmable Acceleration Engine (OPAE) 1.3.0 Release Notes
-----------------------------------------------------------------

This document provides the Release Notes for the Open Programmable
Acceleration Engine (OPAE) 1.3.0 release.

System Compatibility
--------------------

-   Hardware: tightly coupled FPGA products and programmable FPGA
    acceleration cards for Intel(R) Xeon(R) processors:
    - Intel(R) PAC with Arria(R) 10 GX FPGA (PCI ID: 0x09c4) FIM version 1.0.3 (1.0 Production)
    - Intel(R) Xeon with Integrated FPGA (PCI ID: 0xbcc0) FIM version 6.4.0

-   Operating System: tested on Red Hat Enterprise Linux 7.3 and 7.4, Ubuntu 16.04, 
    SUSE SLE 12 SP3 and CentOS 7.4, with Linux kernels 3.10 through 4.7

Major Changes from 1.2.0 to 1.3.0
----------------------------------

- Updated python requests package used by Sphinx
- Updated fpgad to enumerate for supported devices discarding the previous assumption that it is running on a dual-socket integrated FPGA platform
- Added Python version of fpgamux
- Added deprecation notice for legacy C++ API
- Updated default installation location for packages generated through CMake to "/usr" instead of "/usr/local"
- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set 1. (see Notes below for more information)
- Increased test cases and test coverage
- Cleaned up dead/legacy code
- Various bug fixes
- Various compiler warning fixes
- Various memory leak fixes

Notes / Known Issues
--------------------
-  libopaec++ is being deprecated in favor of the official OPAE C++ API.
While no tools in the OPAE codebase use libopaec++, the code is being kept here
for any other tools that may use it outside of the OPAE repository.
This directory, however, will be removed in future versions of OPAE.
For more information and reference on the official API, see the
[documentation](https://opae.github.io/latest/docs/fpga_api/fpga_cxx_api.html).

-   fpgamux has been ported to use the OPAE Python API.

-   In addition to supporting the OPAE driver bundled with OPAE SDK releases, the OPAE SDK libraries now
support the FPGA driver that has been upstreamed to the Linux Kernel 4.18.
For more details on this, please see the [OPAE documentation](https://opae.github.io/1.3.0/docs/fpga_dfl_drv/fpga_dfl_drv.html) related to this.

-   Seldom in stress tests, kernel panic may be encountered in kernel version 3.10. Preliminary debug information seems to indicate it may be related to hugepage support in the Linux kernel.

-   The current Python distributions included in this release are
    -  opae.fpga-1.3.0.tar.gz - The source files for building the Python bindings. This requires OPAE development package to be installed prior to building
    -  opae.fpga-1.3.0-cp27-cp27mu-linux_x86_64.whl - A binary package built with Python 2.7
    -  opae.fpga-1.3.0-cp34-cp34m-linux_x86_64.whl - A binary package built with Python 3.4
    -  opae.fpga-1.3.0-cp35-cp35m-linux_x86_64.whl - A binary package built with Python 3.5


-   A different OPN is used in the design examples

    The Intel Quartus Prime Pro Edition license uses a design example

    OPN of 10AX115N3F40E2SG, instead of the Intel PAC with Intel Arria
    10 GX FPGA OPN of 10AX115N2F40E2LG. This difference does not impact
    your design.

-   PCIe directed speed changes are not supported

    Only automatic down-training at boot time is supported

-   Virtual Function (VF) may fail to attach or detach when using the
    Linux Red Hat 3.10 kernel
    
    This is a known issue with qemu/kvm and libvirt. Refer to the Red
    Hat website for more information about this issue.

-   The Intel FPGA Dynamic Profiler Tool for OpenCL GUI reports
    frequency and bandwidth incorrectly

    This issue will be resolved in a future version of the Intel
    Acceleration Stack.

- Partial reconfiguration with SR-IOV

  If using OPAE in a virtualized environment with SR-IOV enabled, we recommend disabling SR-IOV before performing partial reconfiguration. See "Partial Reconfiguration" in the "OPAE Intel FPGA Linux Device Driver Architecture" document for more information

- fpgaAssignToInterface() and fpgaReleaseFromInterface() not supported

  The OPAE C API provides functions to assign individual AFCs to host interfaces (i.e. a virtual or physical function). Due to the internal implementation of fpga_token, these functions are not yet supported. Instead, we provide a simplified call fpgaAssignPortToInterface() that can assign a port by number to either the physical function (PF) or virtual function (VF). This function will eventually be replaced by the more generic implementation of fpgaAssignToInterface() and fpgaReleaseFromInterface() in a future release.


- AP6 condition may prevent clearing of port errors

  If the system encounters an AP6 condition (exceeded power or temperature threshold), it will report a port error. These errors can only be cleared (e.g. using fpgainfo) after the AP6 condition has been removed.

- Driver may not display explicit incompatibility message if loaded on mismatched FIM version

  When trying to insert the Linux kernel driver modules while an FPGA platform with an unsupported FIM version is present in the system, the driver may fail to load and/or fail to print an explicit incompatibility warning message in the system log. Please make sure to use the driver only with a compatible FIM.

- ASE: Multiple ModelSim simulator instances may crash when run on the same host

    When trying to run multiple instances of the ModelSim simulator on a single system, the simulator may crash. Only run one instance of ModelSim at the same time per system.


