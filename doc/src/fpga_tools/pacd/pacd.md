# pacd #

## SYNOPSIS ##
`pacd --daemon [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--null-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

`pacd [--null-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

## DESCRIPTION ##
`pacd` periodically monitors the sensors on the PAC Baseboard Management Controller (BMC) and programs a NULL bitstream
in response to a sensor's value exceeding a specified threshold. pacd is only available on the PCIe Accelerator Card (PAC).

On systems with multiple PACs, `pacd` will monitor the sensors for all cards in the system using the specified
sensor threshold values.  If the PCIe address is specified (i.e., `-S`, `-B`, `-D`, `-F`), `pacd` will monitor all PACs
matching the PCIe address components specified.  For example, if the user specifies `-B 5` only, all PACs on
PCIe bus `5` will be monitored.  The sensor thresholds are global, so specifying `-T 11:95.0:93.0` will monitor sensor
`11` on all selected PACs and trigger if its value exceeds `95.0` and reset its trigger at `93.0`.

Use SIGINT or SIGTERM to stop `pacd`, either in daemon mode (``kill -2 `cat /tmp/pacd.pid` `` or ``kill -15 `cat /tmp/pacd.pid` ``)
or `^C` when run as a regular process.

## OPTIONS ##

`-d, --daemon`

When specified, `pacd` executes as a system daemon process.

`-P, --directory <dir>`

When running in daemon mode, run from the specified directory (path).
If omitted when daemonizing, `/tmp` is used.

`-l, --logfile <file>`

When running in daemon mode, send output to file. When not in daemon mode, the output goes to stdout.
If omitted when daemonizing, `/tmp/pacd.log` is used.

`-p, --pidfile <file>`

When running in daemon mode, write the daemon's process id to a file.
If omitted when daemonizing, `/tmp/pacd.pid` is used.

`-m, --umask <mode>`

When running in daemon mode, use the mode value as the file mode creation mask passed to umask.
If omitted when daemonizing, `0` is used.

`-i, --poll-interval <secs>`

`pacd` will poll and check the sensor values every `secs` seconds.  This is a real number, so a
floating-point number can be specified (i.e., `2.5` for two and a half second poll interval).

`-c, --cooldown-interval <secs>`

Specifies the time in seconds that `pacd` will wait after removing the FPGA driver before
re-enabling the driver.  This is the time that the host will not be able to access the PAC for
any reason.  Not valid in conjunction with `--driver-removal-disable`.

`-n, --null-bitstream <file>`

Specify the NULL bitstream to program when a sensor value exceeds the specified threshold.
This option may be specified multiple times. The AF, if any, that matches the FPGA's PR interface
ID is programmed when the sensor's value exceeds the threshold.

`-S, --segment <PCIe segment>`

Specify the PCIe segment (domain) of the PAC of interest.

`-B, --bus <PCIe bus>`

Specify the PCIe bus of the PAC of interest.

`-D, --device <PCIe device>`

Specify the PCIe device of the PAC of interest.

`-F, --function <PCIe function>`

Specify the PCIe function of the PAC of interest.

`-T, --upper-sensor-threshold <sensor>:<trigger_threshold>[:<reset_threshold>]`

Specify the threshold value for a sensor that, when exceeded (sensor value strictly greater than
`<trigger_threshold>`), will cause the NULL bitstream specified with `-n` that matches the FPGA's PR
Interface ID to be programmed into the FPGA.  The sensor will be considered triggered (and no PR
performed) until its value drops below `<reset_threshold>`.

This option can be specified multiple times.

The sensors specified will be monitored for all specified PACs.  There is no mechanism for specifying
per-PAC sensor thresholds.

`-T, --lower-sensor-threshold <sensor>:<trigger_threshold>[:<reset_threshold>]`

Specify the threshold value for a sensor that, when exceeded (sensor value strictly less than
`<trigger_threshold>`), will cause the NULL bitstream specified with `-n` that matches the FPGA's PR
Interface ID to be programmed into the FPGA.  The sensor will be considered triggered (and no PR
performed) until its value goes above `<reset_threshold>`.

This option can be specified multiple times.

The sensors specified will be monitored for all specified PACs.  There is no mechanism for specifying
per-PAC sensor thresholds.

`-N, --no-defaults`

`pacd` will by default monitor the same set of sensors that the BMC monitors that could trigger
a machine re-boot.  This set is typically all settable non-recoverable thresholds.  Specifying
this option tells `pacd` not to monitor these sensors.  This option requires at least one of `-T`
or `-t` to be specified.

`--driver-removal-disable`

This is an advanced option with the default being to disable the driver.  When a sensor is initially
tripped requiring a PR of the FPGA, `pacd` will remove the FPGA device driver for the device, wait
for a period of time, re-enable the driver and then PR the NULL bitstream into the device.

If this option is specified, `pacd` will skip disabling the driver and just PR the NULL bitstream
into the device.

## NOTES ##

`pacd` is intended to prevent an over-temperature or power "non-recoverable" event from causing the
FPGA's BMC to shut down the PAC.  Shutting down the PAC results in a PCIe "surprise removal"
which will cause the host to ultimately reboot.

There are several issues that need to be taken into consideration when enabling `pacd`:

1. The application being accelerated needs to be able to respond appropriately when the device
driver disappears from the system.  The application will receive a SIGHUP signal when the driver
shuts itself down.  On receipt of SIGHUP, the app should clean up and exit as soon as possible.
2. There is a window in which the running system will reboot if a PR is in progress when
a sensor's threshold is tripped.
3. The OS and driver cannot invalidate any pointers that the application has to FPGA MMIO
space.  Utilization of the OPAE API to access the MMIO region is strongly recommended
to avoid unanticipated reboots.
4. The OS and driver cannot prevent direct access of host memory from the FPGA, as in
the case of a DMA operation from the AFU to the host.  There is a high probability of
a reboot if a PR is attempted by `pacd` due to a threshold trip event during a DMA operation.

## TROUBLESHOOTING ##

If you encounter any issues, you can get debug information in two ways:

1. By examining the log file when in daemon mode.
2. By running in non-daemon mode and viewing stdout.

## EXAMPLES ##

The following command will start `pacd` as a daemon process, programming `my_null_bits.gbs` when
any BMC-triggerable threshold is tripped.

`pacd --daemon --null-bitstream=my_null_bits.gbs`

The following command will start `pacd` as a regular process, programming `idle.gbs` when
sensor 11 (FPGA Core TEMP) exceeds 92.35 degrees C or sensor 0 (Total Input Power) goes
out of the range [9.2 - 19.9] Watts.

`pacd -n=idle.gbs -T 11:92.35 -T 0:19.9 -t 0:9.2`

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.08.08 | 1.2 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Initial revision.  | 
 | 2018.08.17 | 1.2 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Updated to include new options.  | 
