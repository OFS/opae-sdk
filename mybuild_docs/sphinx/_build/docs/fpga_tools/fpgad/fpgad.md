# fpgad #

## SYNOPSIS ##
`fpgad --daemon [--version] [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--socket=<sock>] [--null-bitstream=<file>]`
`fpgad [--socket=<sock>] [--null-bitstream=<file>]`

## DESCRIPTION ##
fpgad monitors the device sensors, checking for sensor values that are out of the prescribed range. 

When any of the sensors is detected to be out of bounds, fpgad will focus on keeping the server from rebooting by masking PCIE AER, and send a message to system administrator. System administrator can take further actions like stop the application and stop the FPGA, but fpgad just focus on monitor the sensors and will not take any cooling actions. 

Note: fpgad must be running (as root) and actively monitoring devices when a sensor anomaly occurs in order to initiate Graceful Shutdown.  If fpgad is not loaded during such a sensor anomaly, the out-of-bounds scenario will not be detected, and the resulting effect on the hardware is undefined.

### ARGUMENTS ##

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

This command starts fpgad as a system daemon process:
```console
sudo systemctl start fpgad
```

 ## Revision History ##
    
 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1.) | No changes from previous release. |
 |2020.09.02 | 2.0                                                                                                        |
