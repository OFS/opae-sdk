# fpgaport #

## SYNOPSIS ##
```console
fpgaport [-h] {assign,release} device port
```

## DESCRIPTION ##
The ```fpgaport``` enables and disables virtualization. It assigns
and releases control of the port to the virtual function (VF). By default, the driver
assigns the port to the physical function (PF) in the non-virtualization use case.


## POSITIONAL ARGUMENTS ##
`{assign, release}`

   Action to perform.

`device`

The FPGA device being targeted with this action.

`port`

The number of the port.

## OPTIONAL ARGUMENTS ##
`-h, --help`

Print usage information.

## EXAMPLE ##

`fpgaport release /dev/intel-fpga-fme.0 0`

Release port 0 from physical function control.

`fpgaport assign /dev/intel-fpga-fme.0 0`

Assign port 0 to physical function control.

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | No changes from previous release.  | 
