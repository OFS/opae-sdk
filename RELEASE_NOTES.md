Open Programmable Acceleration Engine (OPAE )is a software framework for managing and accessing programmable accelerators (FPGAs). Its main parts are:

-   OPAE Software Development Kit (OPAE SDK),

-   OPAE Linux driver for Intel(R) Xeon(R) CPU with Integrated FPGAs and Intel(R) PAC with Arria(R) 10 GX FPGA

-   Basic Building Block (BBB) library for accelerating AFU
    development (not part of this release, but pre-release code is
    available on GitHub: \[https://github.com/OPAE/intel-fpga-bbb]

OPAE is under active development to extend to more hardware platforms, as well as to build up the software stack with additional abstractions to enable more software developers.

OPAE SDK is a collection of libraries and tools to facilitate the development of software applications and accelerators using OPAE. It provides a library implementing the OPAE C API for presenting a streamlined and easy-to-use interface for software applications to discover, access, and manage FPGA devices and accelerators using the OPAE software stack. The OPAE SDK also includes the AFU Simulation Environment (ASE) for end-to-end simulation of accelerator RTL together with software applications using the OPAE C API.

OPAE\'s goal is to accelerate FPGA adoption. It is a community effort to simplify the development and deployment of FPGA applications, so we explicitly welcome discussions and contributions! The OPAE SDK source, unless otherwise noted, is released under a BSD 3-clause license.

More information about OPAE can be found
at http://01.org/OPAE.

Open Programmable Acceleration Engine (OPAE) 1.1.0 Release Notes
-----------------------------------------------------------------

This document provides the Release Notes for the Open Programmable
Acceleration Engine (OPAE) 1.1.0 release.

System Compatibility
--------------------

-   Hardware: tightly coupled FPGA products and programmable FPGA
    acceleration cards for Intel(R) Xeon(R) processors (to be released):
    - Intel(R) PAC with Arria(R) 10 GX FPGA (PCI ID: 0x09c4) FIM version 1.0.3 (1.0 Production)
    - Intel(R) Xeon with Integrated FPGA (PCI ID: 0xbcc0) FIM version 6.4.0

-   Operating System: tested on Red Hat Enterprise Linux 7.3, SUSE SLE 12 SP3 and CentOS
    7.4, with Linux kernels 3.10 through 4.7

Major Changes from 1.0.0 to 1.1.0
----------------------------------



- Updated command line options for OPAE tools for a consistent user interface
- Added two new language bindings
  - A C++ Core API that is interoperable with the OPAE C API
  - A Python API which wraps the C++ Core API object model
- Disabled documentation generation by default in make to speed up development
- Implemented CMake build-chain for ASE
- Organized samples directory
- Increased test coverage
- Added Error API
- Added SUSE support
- Cleaned up dead/legacy code
- Various bug fixes

Notes / Known Issues
--------------------

-   Seldom in stress tests, kernel panic may be encountered in kernel version 3.10. Preliminary debug information seems to indicate it may be related to hugepage support in the Linux kernel.

-   Memory leak detected by Valgrind points to global data structures used by enumeration routines. This is due to token_cleanup() function not being called when the library is being unloaded. This does not impact memory performance and will be addressed in next release.

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

-   When simulating the hello\_intr\_afu sample code, the
    af2cp\_sTxPort.c1.hdr.rsvd2\[5:4\] has a value of X
    
    This issue will be resolved in a future version of the Intel Acceleration Stack.

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

 
