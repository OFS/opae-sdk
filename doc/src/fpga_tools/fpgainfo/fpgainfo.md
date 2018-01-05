# fpgainfo #

## SYNOPSIS ##
```console
fpgainfo [-h | --help] [-b <bus>] [-d <device>] [-f <function>] <command> [<args>]
```


## DESCRIPTION ##
fpgainfo displays FPGA information derived from sysfs files. The command argument is one of the following: errors, power, or temp. 
Some commands may also have other arguments or options that control the behavior.

For systems with multiple devices, specify the BDF of the target device with -b, -d and -f.

### FPGAINFO COMMANDS ##
`errors`

    Show/clear errors of an FPGA resource that the first argument specifies.
    fpgainfo displays information in human readable form.

`power`

    Show total the power in watts that the FPGA hardware consumes.

`temp`

    Show FPGA temperature values in degrees Fahrenheit.

`port`

   Shows information about the port such as the AFU ID of currently loaded AFU.

`fme`

   Show information about the FPGA platform such as the FPGA Interface Manager (FIM) ID.

## OPTIONAL ARGUMENTS ##
`--help, -h`

    Prints help information and exit.

`-b, --bus`

	PCIe bus number of the target FPGA.
`-d, --device`

	PCIe device number of the target FPGA.

`-f, --function`

	PCIe function number of the target FPGA.

`--clear, -c`

    Clear errors for the given FPGA resource.

### ERRORS ARGUMENTS ###
The first argument to the `errors` command specifies the resource type. It must be one of the following:
   `fme`,`port`,`all`

`fme`

    Show/clear FME errors. 

`port`

    Show/clear PORT errors.

`all`

    Show/clear errors for all resources.


## EXAMPLES ##
This command shows the current power consumtion:
```console
./fpgainfo power
```

This command shows the current temperature reading:
```console
./fpgainfo temp
```

This command shows FME resource errors
```console
./fpgainfo errors fme
```
This command clears all errors on all resources:
```console
./fpgainfo errors all -c
```
