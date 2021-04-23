# HSSI ethernet tools #

## SYNOPSIS ##
```console
hssistats [-h] [--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS]
```

## DESCRIPTION ##
The ```hssistats```  tool provides the MAC statistics.


### OPTIONAL ARGUMENTS ##

`-h, --help`

  Prints usage information

`--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS`

  The PCIe address of the desired fpga in ssss:bb:dd.f format. sbdf of device to program (e.g. 04:00.0 or 0000:04:00.0). Optional when one device in system.

## EXAMPLES ##

`hssistats --pcie-address  <0000:04:00.0`

  prints the MAC statistics 


## SYNOPSIS ##
```console
hssiloopback [-h] [--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS] --loopback [{enable,disable}]
```

## DESCRIPTION ##
The ```hssiloopback```  tool enables and disable loopback.


### OPTIONAL ARGUMENTS ##

`-h, --help`

  Prints usage information

`--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS`

  The PCIe address of the desired fpga  in ssss:bb:dd.f format. sbdf of device to program (e.g. 04:00.0 or 0000:04:00.0). Optional when one device in system.


`--loopback [{enable,disable}]`

  Ethernet enable or disable loopback.
  
## EXAMPLES ##

`hssiloopback --pcie-address  <0000:04:00.0` --loopback enable

  Enables ethernet loopback
 
 
 `hssiloopback --pcie-address  <0000:04:00.0` --loopback disable

  Disable ethernet loopback


## SYNOPSIS ##
```console
hssimac [-h] [--pcie-address PCIE_ADDRESS] [--mtu [MTU]]
```

## DESCRIPTION ##
The ```hssimac```  tool provides Maximum TX and RX frame size.


### OPTIONAL ARGUMENTS ##

`-h, --help`

  Prints usage information

`--pcie-address PCIE_ADDRESS, -P PCIE_ADDRESS`

  The PCIe address of the desired fpga  in ssss:bb:dd.f format. sbdf of device to program (e.g. 04:00.0 or 0000:04:00.0). Optional when one device in system.


`--mtu [MTU]`

  Maximum allowable ethernet frame length.

## EXAMPLES ##

`hssimac --pcie-address  <0000:04:00.0> --mtu`

  prints Maximum TX and RX frame size.



## Revision History ##

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) | Made the following changes: <br>Expanded descriptions of `nlb_mode_3` and`dma_afu` tests. <br> Added a second example command. |


