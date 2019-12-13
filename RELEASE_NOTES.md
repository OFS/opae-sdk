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

Open Programmable Acceleration Engine (OPAE) 1.4.0 Release Notes
-----------------------------------------------------------------

This document provides the Release Notes for the Open Programmable
Acceleration Engine (OPAE) 1.4.0 release.

System Compatibility
--------------------

-   Hardware: tightly coupled FPGA products and programmable FPGA
    acceleration cards for Intel(R) Xeon(R) processors:
    - Intel(R) PAC with Arria(R) 10 GX FPGA (PCI ID: 0x09c4) FIM version 1.1.2-1 (1.2 Production)
    - Intel(R) Xeon with Integrated FPGA (PCI ID: 0xbcc0) FIM version 6.4.0
    - Intel® FPGA Programmable Acceleration Card N3000 (PCI ID: 0x0b30) FIM version D.1.0.13 (1.0 Production)

-   Operating System: Tested on RedHat 7.6, CentOS 7.6  with Linux Kernel 3.10 and the community 4.19 LTS kernels.

Major Changes from 1.3.0 to 1.4.0
----------------------------------

- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set2.
- Added test cases and Increased test coverage
- Various bug fixes
- Various memory leak fixes
- Various Static code scan bug fixes
- Added new FPGA MMIO API to write 512 bits

- OPAE 1.4.0 may not be compatible with other versions of Linux OS/Kernel

- OPAE & Intel FPGA driver are tested on Intel Programmable Acceleration Card Arria 10 GX FPGA & Intel PAC N3000.

- OPAE & DFL FPGA driver are tested on Intel Programmable Acceleration Card Arria 10 GX FPGA.

- FPGA DFL Linux driver source code patchset2 available 
   - https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers?h=linux-5.4.y


Notes / Known Issues
--------------------


- In addition to supporting the OPAE driver bundled with OPAE SDK releases, the OPAE SDK libraries now
support the FPGA driver that has been upstreamed to the Linux Kernel 5.4.
For more details on this, please see the [OPAE documentation](https://opae.github.io/1.4.0/docs/fpga_dfl_drv/fpga_dfl_drv.html) related to this.

- DFL FPGA driver pactset2 doesn’t support all the features supported by Intel FPAG driver.

- FPGAInfo tool doesn’t clear injected error.

- Partial reconfiguration with SR-IOV

- If using OPAE in a virtualized environment with SR-IOV enabled, we recommend disabling SR-IOV before performing partial reconfiguration. See "Partial Reconfiguration" in the "OPAE Intel FPGA Linux Device Driver Architecture" document for more information

- Driver may not display explicit incompatibility message if loaded on mismatched FIM version

    When trying to insert the Linux kernel driver modules while an FPGA platform with an unsupported FIM version is present in the system, the driver may fail to load and/or fail to print an explicit incompatibility warning message in the system log. Please make sure to use the driver only with a compatible FIM.

- ASE: Multiple ModelSim simulator instances may crash when run on the same host

    When trying to run multiple instances of the ModelSim simulator on a single system, the simulator may crash. Only run one instance of ModelSim at the same time per system.


