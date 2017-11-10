# fpgad #

## SYNOPSIS ##
`fpgad --daemon [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--socket=<sock>] [--null-bitstream=<file>]`
`fpgad [--socket=<sock>] [--null-bitstream=<file>]`

## DESCRIPTION ##
Periodically monitors/reports the error status reflected in the device driver's error status sysfs files.
Establishes the channel by which events are communicated to the OPAE application. Programs a NULL bitstream
in response to AP6 event.

fpgad is required to be running before API calls `fpgaRegisterEvent` and `fpgaUnregisterEvent` will succeed.

Use SIGINT to stop fpgad.

`-d, --daemon`

    When given, fpgad executes as a system demon process.

`-D, --directory <dir>`

    When running in daemon mode, execute from the given directory.
    If omitted when daemonizing, /tmp is used.

`-l, --logfile <file>`

    When running in daemon mode, send output to file. When not in daemon mode, the output is sent to stdout.
    If omitted when daemonizaing, /tmp/fpgad.log is used.

`-p, --pidfile <file>`

    When running in daemon mode, write the daemon's process id to file.
    If omitted when daemonizing, /tmp/fpgad.pid is used.

`-m, --umask <mode>`

    When running in daemon mode, use the mode value as the file mode creation mask passed to umask.
    If omitted when daemonizing, 0 is used.

`-s, --socket <sock>`

    Listen for event API registration requests on sock. The default socket value used by the API is
    /tmp/fpga_event_socket. 

`-n, --null-bitstream <file>`

    Specify the NULL bitstream to program when an AP6 event occurs. This option may be given multiple
    times. The bitstream, if any, that matches the FPGA's PR interface id will be programmed when AP6
    is detected.

## TROUBLESHOOTING ##

If any issues are encountered, try the following for additional debug information:

1. Examine the log file when in daemon mode.
2. Run in non-daemon mode and view stdout.

## EXAMPLES ##

`fpgad --daemon --null-bitstream=my_null_bits.gbs`

