Open Programmable Acceleration Engine
-----------------------------------------------------------------

Open Programmable Acceleration Engine (OPAE) is a software framework for managing and accessing programmable accelerators (FPGAs). Its main parts are:

-   OPAE Software Development Kit (OPAE SDK),

-   OPAE Linux driver for Intel(R) Xeon(R) CPU with Integrated FPGAs and Intel(R) PAC with Arria(R) 10 GX FPGA

-   Basic Building Block (BBB) library for accelerating AFU
    development (not part of this release, but pre-release code is
    available on GitHub: https://github.com/OPAE/intel-fpga-bbb

OPAE is under active development to extend to more hardware platforms, as well as to build up the software stack with additional abstractions to enable more software developers.

OPAE SDK is a collection of libraries and tools to facilitate the development of software applications and accelerators using OPAE. It provides a library implementing the OPAE C API for presenting a streamlined and easy-to-use interface for software applications to discover, access, and manage FPGA devices and accelerators using the OPAE software stack. The OPAE SDK also includes the AFU Simulation Environment (ASE) for end-to-end simulation of accelerator RTL together with software applications using the OPAE C API.

OPAE's goal is to accelerate FPGA adoption. It is a community effort to simplify the development and deployment of FPGA applications, so we explicitly welcome discussions and contributions! The OPAE SDK source, unless otherwise noted, is released under a BSD 3-clause license.

More information about OPAE can be found
at http://01.org/OPAE.

Open Programmable Acceleration Engine (OPAE) 1.4.1 Release Notes
-----------------------------------------------------------------

OPAE 1.4.1 release provides SDK and tools that have been incorporated into Fedora to support FPGA kernel driver that is upstreamed to Linux 5.6 kernel as of March 2020.  The main features that this package and the driver include are:

-   Basic functionalities including PR (Programmable Region?), PCIe, FME (FPGA Management Engine), and AFU (Accelerator Functional Unit)
-   SRIOV, Error Handling, User Clock
    The driver can be found here:
    https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers?h=linux-5.6.y


System Compatibility
--------------------

-   Hardware: tightly coupled FPGA products and programmable FPGA acceleration cards for Intel(R) Xeon(R) processors:
       Intel(R) PAC with Arria(R) 10 GX FPGA (PCI ID: 0x09c4) FIM version 1.1.2-1 (1.2 Production)
-   Operating System: Tested on Fedora 31 with Linux Kernel 5.6


Major Changes from 1.4.0 to 1.4.1
----------------------------------

-   OPAE git repository layout changes

       The opae-sdk repository has been reorganized into five total repositories.
       The following table describes the repositories and how they are integrated into opae-sdk.

| Repository      | Description | Integration     |
| :---        |    :----:   |          ---: |
| opae-sdk      | Contains tools built on top of OPAE libraries  and/or kernel interfaces.       | Master repository.  |
| opae-libs   | Contains libraries that implement the OPAE APIs.        | Added as a git subtree to opae-sdk/opae-libs.      |
| opae-legacy   | Contains legacy tools designed for Skylake + FPGA platforms.        | Added as an external project in CMake.   |
| opae-sim   | Contains simulation projects like ASE.        |  Added as an external project in CMake.      |
| opae-test  | Contains mock framework for unit tests.        | Added as an external project in CMake.      |


-   Removed Safe String module dependency
-   Removed pybind11 3rd component from OPAE source repository.  pybind11 is now dynamically loaded
-   Ported python tools to python3.6


Notes / Known Issues
--------------------
-   This release supports FPGA driver that has been upstreamed to the Linux Kernel 5.6 as of March, 2020.  The driver does not support all FPGA features available on Intel(R) PAC with Arria(R) 10 GX FPGA cards or Intel® FPGA Programmable Acceleration Card N3000 cards.
