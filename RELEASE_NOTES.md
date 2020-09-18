Open Programmable Acceleration Engine (OPAE) is a software framework for managing and accessing programmable accelerators (FPGAs). Its main parts are:

-   OPAE Software Development Kit (OPAE SDK),

-   OPAE Linux DFL driver for Intel® FPGA Programmable Acceleration Card

-   Basic Building Block (BBB) library for accelerating AFU
    development (not part of this release, but pre-release code is
    available on GitHub: https://github.com/OPAE/intel-fpga-bbb

OPAE is under active development to extend to more hardware platforms, as well as to build up the software stack with additional abstractions to enable more software developers.

OPAE SDK is a collection of libraries and tools to facilitate the development of software applications and accelerators using OPAE. It provides a library implementing the OPAE C API for presenting a streamlined and easy-to-use interface for software applications to discover, access, and manage FPGA devices and accelerators using the OPAE software stack. The OPAE SDK also includes the AFU Simulation Environment (ASE) for end-to-end simulation of accelerator RTL together with software applications using the OPAE C API.

OPAE\'s goal is to accelerate FPGA adoption. It is a community effort to simplify the development and deployment of FPGA applications, so we explicitly welcome discussions and contributions! The OPAE SDK source, unless otherwise noted, is released under a BSD 3-clause license.

More information about OPAE can be found
at http://01.org/OPAE.



Open Programmable Acceleration Engine (OPAE) 2.0.0 Release Notes
----------------------------------------------------------------------------------------------------------------------

OPAE 2.0.0 release provides SDK, tools, and Linux kernel driver.  The main feature of this release is to support Intel®  FPGA Programmable Acceleration Card N3000 series.

System Compatibility
--------------------
-   Hardware: Tightly coupled FPGA products and programmable FPGA  acceleration cards for Intel(R) Xeon(R) processors:
- IntelÂ® FPGA Programmable Acceleration Card N3000-2 Production Release
- IntelÂ® FPGA Programmable Acceleration Card N3000-V Production Release
- IntelÂ® FPGA Programmable Acceleration Card N3000-3 Production Release


Major Changes from 1.4.1 to 2.0.0
---------------------------------
- Added support to FPGA Linux kernel Device Feature List (DFL) driver.
- Added support to PAC card N3000 series.
- Added PACSign, bitstream_info, fpgasudpate, rsu, fpgaotsu, fpgaport  python tools.
- Added ethernet tools for PAC card N3000.
- Various bug fixes
- Various Static code scan bug fixes
- Added python3 support.
- OPAE USMG API are deprecated.
- Updated OPAE documentation.   

-   Operating System: Tested on Fedora 31 with Linux Kernel 5.8 version.

Source Code:
------------
-   FPGA DFL Linux driver source code:  tag  2.0.0-1
- https://github.com/OPAE/linux-dfl/tree/fpga-upstream-dev-5.8.0

-   SDK and tools source code:  tag 2.0.0-1
- https://github.com/OPAE/opae-sdk/tree/release/2.0.0
- https://github.com/OPAE/opae-libs/tree/release/2.0.0
- https://github.com/OPAE/opae-legacy/tree/release/2.0.0
- https://github.com/OPAE/opae-sim/tree/release/2.0.0
- https://github.com/OPAE/opae-test/tree/release/2.0.0

Notes/Known Issues
------------------
- FPGA DFL kernel driver upstreaming to Linux kernel is on going
- OPAE 2.0 is not compatible with Intel previous production FPGA driver.


Major Changes from 1.4.0 to 2.0.0
----------------------------------

- Added support to FPGA Linux kernel Device Feature List (DFL) driver patch set3,set4 and set5.
- Added support to both PAC card N3000 & D5005 cards.
- Added pacsingn, bitstreaminfo, fpgasudpate, rsu  python tools.
- Added ethernet tools for PAC card N3000.
- Various bug fixes
- Various memory leak fixes
- Various Static code scan bug fixes

- OPAE 2.0.0 tested on Fedora 31  with Linux Kernel 5.8 version.

- FPGA DFL Linux driver source code available in Linux 5.8 kernel
   - https://github.com/OPAE/linux-dfl/tree/fpga-upstream-dev-5.8.0



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

- FPGA DFL Linux driver source code patchset2 available in Linux 5.4 kernel
   - https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers?h=linux-5.4.y


Notes / Known Issues
--------------------

- In addition to supporting the OPAE driver bundled with OPAE SDK releases, the OPAE SDK libraries now
support the FPGA driver that has been upstreamed to the Linux Kernel 5.4.
For more details on this, please see the [OPAE documentation](https://opae.github.io/1.4.0/docs/fpga_dfl_drv/fpga_dfl_drv.html) related to this.

- DFL FPGA driver patchset2 doesn’t support all the features supported by Intel FPGA driver.

- FPGAInfo tool doesn’t clear injected error.

- Partial reconfiguration with SR-IOV

- If using OPAE in a virtualized environment with SR-IOV enabled, we recommend disabling SR-IOV before performing partial reconfiguration. See "Partial Reconfiguration" in the "OPAE Intel FPGA Linux Device Driver Architecture" document for more information

- Driver may not display explicit incompatibility message if loaded on mismatched FIM version

    When trying to insert the Linux kernel driver modules while an FPGA platform with an unsupported FIM version is present in the system, the driver may fail to load and/or fail to print an explicit incompatibility warning message in the system log. Please make sure to use the driver only with a compatible FIM.

- ASE: Multiple ModelSim simulator instances may crash when run on the same host

    When trying to run multiple instances of the ModelSim simulator on a single system, the simulator may crash. Only run one instance of ModelSim at the same time per system.
