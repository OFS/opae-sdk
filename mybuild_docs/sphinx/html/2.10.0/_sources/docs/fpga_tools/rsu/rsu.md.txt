# rsu #

## SYNOPSIS ##
```console
rsu [-h] [-d] {bmc,bmcimg,retimer,fpga,sdm,fpgadefault} [PCIE_ADDR]

```

## DESCRIPTION ##

### Mode 1: RSU ###

```console
rsu bmc --page=(user|factory) [PCIE_ADDR]
rsu retimer [PCIE_ADDR]
rsu fpga --page=(user1|user2|factory) [PCIE_ADDR]
rsu sdm --type=(sr|pr|sr_cancel|pr_cancel) [PCIE_ADDR]
```

Perform RSU (remote system update) operation on PAC device
given its PCIe address.
An RSU operation sends an instruction to the device to trigger
a power cycle of the card only. This will force reconfiguration
from flash for either BMC, Retimer, SDM, (on devices that support these)
or the FPGA.

### Mode 2: Default FPGA Image ###

```console
rsu fpgadefault --page=(user1|user2|factory) --fallback=<csv> [PCIE_ADDR]
```

Set the default FPGA boot sequence. The --page option determines
the primary FPGA boot image. The --fallback option allows a comma-separated
list of values to specify fallback images.

## POSITIONAL ARGUMENTS ##
`{bmc,bmcimg,retimer,fpga,sdm,fpgadefault}`

type of RSU operation or set Default FPGA Image operation.
   
`PCIE_ADDR` 
PCIe address of device to do rsu (e.g. 04:00.0 or 0000:04:00.0) 

##  OPTIONAL ARGUMENTS ##
`-h, --help`
show this help message and exit

`-d, --debug`
log debug statements

`--force`
force rsu operation

## EXAMPLE ##

```console
# rsu bmc --page=user 25:00.0
```

 Triggers a boot of the BMC image (user page) for the device with PCIe
 address 25:00.0.

```console
# rsu bmc --page=factory 25:00.0
```

 Triggers a factory boot of the BMC image for the device with
 PCIe address 25:00.0.

```console
# rsu fpga --page=user2 25:00.0
```

 Triggers a reconfiguration of the FPGA (user2 page) for the
 device with PCIe address 25:00.0.

```console
# rsu --force fpga --page=user2 25:00.0
```

 Forces a reconfiguration of the FPGA (user2 page) for the
 device with PCIe address 25:00.0. Default behavior is to not perform
 the rsu operation if DPC (downstream port containment) is not supported
 and AER (advanced error reporting) is also not supported. Using --force
 changes this behavior to perform rsu operation regardless but may result
 in a surprise removal of pci devices which may cause the Linux kernel
 to panic.

```console
# rsu fpga --page=factory 25:00.0
```

 Triggers a factory reconfiguration of the FPGA for the device
 with PCIe address 25:00.0.

```console
# rsu sdm --type=sr 25:00.0
```

 Triggers Static Region key programming for the device with
 PCIE address 25:00.0.

```console
# rsu fpgadefault --page=factory --fallback=user1,user2 25:00.0
```

 Sets the FPGA boot sequence to factory with fallbacks user1, user2.
