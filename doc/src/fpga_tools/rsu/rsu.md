# rsu #

## SYNOPSIS ##
```console
rsu [-h] [-f] [-d] {bmcimg,fpga} [bdf]

```

## DESCRIPTION ##

Perform RSU (remote system update) operation on PAC device
given its PCIe address.
An RSU operation sends an instruction to the device to trigger
a power cycle of the card only. This will force reconfiguration
from flash for either BMC image (on devices that support it) or the
FPGA


## POSITIONAL ARGUMENTS ##
`{bmcimg,fpga}`

type of operation
   
 `bdf` 
PCIe address of device to do rsu (e.g. 04:00.0 or 0000:04:00.0) 

##  OPTIONAL ARGUMENTS ##
`-h, --help`
show this help message and exit

`-f, --factory`
reload from factory bank

`-d, --debug`
log debug statements

## EXAMPLE ##

 This will trigger a boot of the BMC image for a device with a pci address
 of 25:00.0.
 NOTE: Both BMC and FPGA images will be reconfigured from user bank.
```console
# rsu bmcimg 25:00.0
```


This will trigger a factory boot of the BMC image for a device with a
 pci address of 25:00.0.
NOTE: Both BMC image will be reconfigured from factory bank and the
```console
# rsu bmcimg 25:00.0 -f
```

This will trigger a reconfiguration of the FPGA only for a device with a
pci address of 25:00.0.
NOTE: The FPGA image will be reconfigured from user bank.
```console
# rsu fpga 25:00.0
```

This will trigger a factory reconfiguration of the FPGA only for a device
with a pci address of 25:00.0.
NOTE: The FPGA image will be reconfigured from the factory bank.
```console
# rsu fpga 25:00.0 -f
```


