# Enable OPAE on FPGA PCIe drivers #

.. toctree::

.. highlight:: c

.. highlight:: console

FPGA PCIe driver for PCIe-based Field-Programmable Gate Array (FPGA) solutions which implement
the Device Feature List (DFL). This driver provides interfaces for user space applications to
configure, enumerate, open and access FPGA accelerators on the FPGA DFL devices. additionally, it
also enables system level management functions such as FPGA partial reconfiguration, power management,
virtualization with DFL framework and DFL feature device drivers.

OPAE 1.4.0 release supports both FPGA Intel Linux driver as well as Linux FPGA DFL driver patch set2.
Linux PCIe FPGA DFL driver supports Intel FPGA devices.

FPGA DFL Linux driver source code patchset2 available 
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers?h=linux-5.4.y

FPGA DFL Linux driver source code patchset1 available 
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/fpga?h=v4.19.14


![FPGA PCIe driver Architecture](pcie_dfl_drv_arch.PNG "FPGA PCIe driver Architecture")


List of FPGA features enabled on different FPGA drivers

| FPGA Features                                            | OPAE/FPGA Intel driver                            |OPAE/FPGA DFL driver version 2                        | OPAE/FPGA DFL driver version 1                        |
|----------------------------------------------------------|---------------------------------------------------|------------------------------------------------------|-------------------------------------------------------|
| FPGA Device Enumeration                                  | YES                                               | YES                                                  |YES                                                    |
| Memory map, FPGA control & status registers              | YES                                               | YES                                                  |YES                                                    |
| Shared system memory                                     | YES                                               | YES                                                  | YES                                                   |
| Low-latency notifications                                | YES                                               | NO                                                   |NO                                                     |
| Partial Reconfiguration                                  | YES                                               | YES                                                  |NO                                                     |
| Assign /Release Accelerators to host interfaces          | YES                                               | YES                                                  |NO                                                     |
| Metrics/Telemetry                                        | YES                                               | YES                                                  |NO                                                     |
| FPGA Events                                              | YES                                               | NO                                                   |NO                                                     |


List of OPAE tools enabled on different FPGA drivers:

| OPAE tool            | OPAE/FPGA Intel driver                            |OPAE/FPGA DFL driver version 2                         |OPAE/FPGA DFL driver version 1                         |
|----------------------|---------------------------------------------------|-------------------------------------------------------|-------------------------------------------------------|
| hello_fpga           | YES                                               | YES                                                   | YES                                                   |
| fpgaconf             | YES                                               | YES                                                   | NO                                                    |
| fpgad                | YES                                               | NO                                                    | NO                                                    |
| fpgainfo             | YES                                               | YES                                                   | NO                                                    |
| fpgametrics          | YES                                               | YES                                                   | NO                                                    |
| hello_events         | YES                                               | NO                                                    | NO                                                    |
| hssi_config          | YES                                               | NO                                                    | NO                                                    |
| hssi_loopback        | YES                                               | NO                                                    | NO                                                    |
| object_api           | YES                                               | YES                                                   | NO                                                    |
| mmlink               | YES                                               | YES                                                   | NO                                                    |
| bist_app             | YES                                               | NO                                                    | NO                                                    |
| coreidle             | YES                                               | NO                                                    | NO                                                    |
| discover_fpgas       | YES                                               | NO                                                    | NO                                                    |
| fpga_dma_test        | YES                                               | NO                                                    | YES                                                   |
| hello_cxxcore        | YES                                               | YES                                                   | NO                                                    |
| ras                  | YES                                               | NO                                                    | NO                                                    |
| userclk              | YES                                               | YES                                                   | NO                                                    |
| nlb0                 | YES                                               | NO                                                    | NO                                                    |
| nlb3                 | YES                                               | NO                                                    | NO                                                    |
| nlb7                 | YES                                               | NO                                                    | NO                                                    |
| packager             | YES                                               | YES                                                   | YES                                                   |
