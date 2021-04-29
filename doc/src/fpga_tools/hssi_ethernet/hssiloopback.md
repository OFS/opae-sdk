# HSSI ethernet loopback #

## SYNOPSIS ##
```console
hssiloopback [-h] [--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS] --loopback [{enable,disable}]
```

## DESCRIPTION ##
The ```hssiloopback```  tool enables and disable ethernet loopback.


### OPTIONAL ARGUMENTS ##

`-h, --help`

  Prints usage information

`--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS`

  The PCIe address of the desired fpga  in ssss:bb:dd.f format. sbdf of device to program (e.g. 04:00.0 or 0000:04:00.0). Optional when one device in system.


`--loopback [{enable,disable}]`

  Ethernet enable or disable loopback.
  
## EXAMPLES ##

`hssiloopback --pcie-address  0000:04:00.0 --loopback enable`

  Enables ethernet loopback
 
 
 `hssiloopback --pcie-address  0000:04:00.0 --loopback disable`

  Disable ethernet loopback


## Revision History ##

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) | Made the following changes: <br>Expanded descriptions of `nlb_mode_3` and`dma_afu` tests. <br> Added a second example command. |


