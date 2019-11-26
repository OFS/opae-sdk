# fpgad #

## SYNOPSIS ##
`fpgad --daemon [--version] [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--socket=<sock>] [--null-bitstream=<file>]`
`fpgad [--socket=<sock>] [--null-bitstream=<file>]`

## DESCRIPTION ##
```fpgad``` periodically monitors and reports the error status reflected in the device driver's error status sysfs files.
```fpgad``` establishes the channel to communicate events to the Open Programmable Accelerator Engine (OPAE) application. 
```fpgad``` programs a NULL bitstream in response to an AP6 (power) event. ```fpgad``` is only available on the Integrated FPGA
Platform. You cannot run ```fpgad``` on the PCIe&reg; Accelerator Card (PAC).

If your system does not support interrupts, you must run ```fpgad``` before the API calls `fpgaRegisterEvent` and
`fpgaUnregisterEvent` can succeed.

Use SIGINT to stop ```fpgad```.

`-v, --version`

    Prints version information and exits.

`-d, --daemon`

    When specified, fpgad executes as a system daemon process.

`-D, --directory <dir>`

    When running in daemon mode, run from the specified directory.
    If omitted when daemonizing, `fpgad` uses /tmp.

`-l, --logfile <file>`

    When running in daemon mode, send output to file. When not in daemon mode, the output goes to stdout.
    If omitted when daemonizaing, fpgad uses /tmp/fpgad.log.

`-p, --pidfile <file>`

    When running in daemon mode, write the daemon's process id to a file.
    If omitted when daemonizing, fpgad uses /tmp/fpgad.pid.

`-m, --umask <mode>`

    When running in daemon mode, use the mode value as the file mode creation mask passed to umask.
    If omitted when daemonizing, fpgad uses 0.

`-s, --socket <sock>`

    Listen for event API registration requests on the UNIX domain socket on the specified path. 
    The default=/tmp/fpga_event_socket. 

`-n, --null-bitstream <file>`

    Specify the NULL bitstream to program when an AP6 event occurs. This option may be specified multiple
    times. The AF, if any, that matches the FPGA's PR interface ID is programmed when an AP6
    event occurs.

## TROUBLESHOOTING ##

If you encounter any issues, you can get debug information in two ways:

1. By examining the log file when in daemon mode.
2. By running in non-daemon mode and viewing stdout.

## EXAMPLES ##

`fpgad --daemon --null-bitstream=my_null_bits.gbs`

 ## Revision History ##
    
 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1.) | No changes from previous release. |
