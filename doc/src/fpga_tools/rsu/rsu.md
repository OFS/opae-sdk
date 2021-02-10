# rsu #

## SYNOPSIS ##
```console
rsu [-h] [-d] {bmcimg,retimer,fpga,nextboot} [PCIE_ADDR]

```

## DESCRIPTION ##

### Mode 1: RSU ###

```console
rsu bmcimg --page=(user|factory) [PCIE_ADDR]
rsu retimer [PCIE_ADDR]
rsu fpga --page=(1|2|factory) [PCIE_ADDR]
```

Perform RSU (remote system update) operation on PAC device
given its PCIe address.
An RSU operation sends an instruction to the device to trigger
a power cycle of the card only. This will force reconfiguration
from flash for either BMC image (on devices that support it) or the
FPGA.

### Mode 2: Next Boot Image ###

```console
rsu nextboot --fpga=(1|2|3|4) [PCIE_ADDR]
```

Set the image that is to boot during the next power cycle.
Currently, the FPGA image is supported by this mode.

1 : fallback boot sequence is User1 -> User2 -> Factory<br>
2 : fallback boot sequence is User2 -> User1 -> Factory<br>
3 : fallback boot sequence is Factory -> User1 -> User2<br>
4 : fallback boot sequence is Factory -> User2 -> User1

## POSITIONAL ARGUMENTS ##
`{bmcimg,retimer,fpga,nextboot}`

type of RSU operation or the Next Boot operation.
   
`PCIE_ADDR` 
PCIe address of device to do rsu (e.g. 04:00.0 or 0000:04:00.0) 

##  OPTIONAL ARGUMENTS ##
`-h, --help`
show this help message and exit

`-d, --debug`
log debug statements

## EXAMPLE ##

```console
# rsu bmcimg --page=user 25:00.0
```

 Triggers a boot of the BMC image for the device with PCIe
 address 25:00.0.
 NOTE: Both BMC and FPGA images will be reconfigured from the user bank.

```console
# rsu bmcimg --page=factory 25:00.0
```

 Triggers a factory boot of the BMC image for the device with
 PCIe address 25:00.0.
 NOTE: The BMC image will be reconfigured from the factory bank.

```console
# rsu fpga --page=2 25:00.0
```

 Triggers a reconfiguration of the FPGA for the device with
 PCIe address 25:00.0.
 NOTE: The FPGA image will be reconfigured from user2 bank.

```console
# rsu fpga --page=factory 25:00.0
```

 Triggers a factory reconfiguration of the FPGA for the device
 with PCIe address 25:00.0.
 NOTE: The FPGA image will be reconfigured from the factory bank.
