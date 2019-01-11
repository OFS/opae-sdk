# fpgainfo #

## SYNOPSIS ##
```console
fpgainfo [-h | --help] <command> [<args>]
```


## DESCRIPTION ##
fpgainfo displays FPGA information derived from sysfs files. The command argument is one of the following:
`errors`, `power`, `temp`, `port` or `fme`. 
Some commands may also have other arguments or options that control their behavior.

For systems with multiple FPGA devices, you can specify the BDF to limit the output to the FPGA resource
with the corresponding PCIe configuration. If not specified, information displays for all resources for
the given command.

### FPGAINFO COMMANDS ##
`errors`

Show/clear errors of an FPGA resource that the first argument specifies.
`fpgainfo` displays information in human readable form.

`power`

Show total the power in watts that the FPGA hardware consumes.

`temp`

 Show FPGA temperature values in degrees Celcius.

`port`

Show information about the port such as the AFU ID of currently loaded AFU.

`fme`

Show information about the FPGA platform including the partial reconfiguration (PR) Interface ID, the OPAE version,
and the FPGA Interface Manager (FIM) ID.

## OPTIONAL ARGUMENTS ##
`--help, -h`

Prints help information and exit.

## COMMON ARGUMENTS ##
The following arguments are common to all commands and are optional.

`-B, --bus`

PCIe bus number of resource.

`-D, --device`

PCIe device number of resource.

`-F, --function`

PCIe function number of resource.

`--json`

Display information as JSON string.

### ERRORS ARGUMENTS ###
The first argument to the `errors` command specifies the resource type. It must be one of the following:
   `fme`,`port`,`all`

`fme`

 Show/clear FME errors. 

`port`

 Show/clear PORT errors.

`all`

Show/clear errors for all resources.

`--clear, -c`

Clear errors for the given FPGA resource.


## EXAMPLES ##
This command shows the current power consumption:
```console
./fpgainfo power
```

This command shows the current temperature reading:
```console
./fpgainfo temp
```

This command shows FME resource errors:
```console
./fpgainfo errors fme
```
This command clears all errors on all resources:
```console
./fpgainfo errors all -c
```
This command shows information of the FME on bus 0x5e
```console
./fpgainfo fme -b 0x5e
```

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Updated description of the `fme` command | 
