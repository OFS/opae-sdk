# fpgainfo #

## SYNOPSIS ##
```console
fpgainfo [-h | --help] [-s | --socket-id] <command> [<args>]
```


## DESCRIPTION ##
fpgainfo is a tool to show FPGA information derived from sysfs files. The command argument
is one of the following: errors, power, temp and is used to specify what type of information
to report. Some commands may also have other arguments/options that can be used to control the
behavior of that command.

## COMMON OPTIONS ##
`--help, -h`

    Print help information and exit.

`--socket-id, -s`

    Socket ID encoded in BBS. Default=0


### FPGAINFO COMMANDS ##
`errors`

    Show/clear errors of an FPGA resource as specified by the first argument.
    Error information is parsed to display in human readable form.

`power`

    Show total power consumed by the FPGA hardware in watts

`temp`

    Show FPGA temperature values in degrees Farenheit

### ERRORS OPTIONS ###
`--clear, -c`

    Clear errors for the given FPGA resource

### ERRORS ARGUMENTS ###
The first argument to the `errors` command is used to specify what kind of
resource to act on. It must be one of the following:
`fme`,`port`,`first_error`,`pcie0`,`pcie1`,`bbs`,`gbs`,`all`
More details on the errors reported for the resource can be found below:


### ERRORS RESOURCES ###
`fme`

    Show/clear errors pertaining to the FME

`port`

    Show/clear errors pertaining to the PORT

`first_error`

    Show/clear first errors encountered by the FPGA

`pcie0`

    Show/clear errors pertaining to the PCIE0 lane

`pcie1`

    Show/clear errors pertaining to the PCIE1 lane

`bbs`

    Show/clear errors pertaining to the BBS (blue bitstream)

`gbs`

    Show/clear errors pertaining to the GBS (green bitstream)

`all`

    Show/clear errors for all resources


## EXAMPLES ##
This command shows the current power consumtion:
```console
./fpgainfo power
```

This command shows the current temperature reading:
```console
./fpgainfo temp
```

This command shows the errors for the FME resource:
```console
./fpgainfo errors fme
```
This command clears all the errors on all resources:
```console
./fpgainfo errors all -c
```
