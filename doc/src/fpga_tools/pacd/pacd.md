# pacd #

## SYNOPSIS ##
`pacd --daemon [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--default-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

`pacd [--default-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

## DESCRIPTION ##
`pacd` periodically monitors the sensors on the PAC Baseboard Management Controller (BMC) and programs a default bitstream
in response to a sensor's value exceeding a specified threshold. pacd is only available on the PCIe Accelerator Card (PAC).

On systems with multiple PACs, `pacd` will monitor the sensors for all cards in the system using the specified
sensor threshold values.  If the PCIe address is specified (i.e., `-S`, `-B`, `-D`, `-F`), `pacd` will monitor all PACs
matching the PCIe address components specified.  For example, if the user specifies `-B 5` only, all PACs on
PCIe bus `5` will be monitored.  The sensor thresholds are global, so specifying `-T 11:95.0:93.0` will monitor sensor
`11` on all selected PACs and trigger if its value exceeds `95.0` and reset its trigger at `93.0`.

Use SIGINT or SIGTERM to stop `pacd`, either in daemon mode (``kill -2 `cat /tmp/pacd.pid` `` or ``kill -15 `cat /tmp/pacd.pid` ``)
or `^C` when run as a regular process.


## INSTALLING AS A SYSTEM SERVICE ##
The tools installation process will install all the necessary files required to make `pacd` a `systemd` service, capable of
automatically starting on boot if desired.

In order to start `pacd` as a `systemd` service, first edit the file `/etc/sysconfig/pacd.conf` as root.  This file is shown
below.

```
# Intel Programmable Acceleration Card (PAC) daemon variables.
# Monitors Baseboard Management Controller (BMC) sensors.

############## REQUIRED OPTIONS ################

PIDFile=/tmp/pacd.pid

# Specify default GBS files to consider for PR.  Include '-n' for each.
# ex.: DefaultGBSOptions=-n <Default_GBS_Path> -n <Default_GBS_PATH_2>
DefaultGBSOptions=-n <Default_GBS_Path>
UMask=0
LogFile=/tmp/pacd.log
PollInterval=0
CooldownInterval=0

############## OPTIONAL OPTIONS ################

# Uncomment and specify specific PAC PCI address to monitor.
# Default is to monitor all PACs
#BoardPCIAddr=-S 0 -B 5 -D 0 -F 0

# Specify threshold values. -T for UNR, -t for LNR.
# ex.: ThresholdOptions=-T 4:12.5 -t 7:2.25:2.3
ThresholdOptions=

# Extra advanced options.
# ex.: ExtraOptions=--no-defaults --driver-removal-disable
ExtraOptions=
```

Edit the `DefaultGBSOptions=` line, specifying the absolute path(s) of the GBS files to be loaded into the device when a
threshold has been exceeded.  Prefix each GBS file name with `-n`.

To start the service, first tell `systemd` to rescan for services using the command `sudo systemctl daemon-reload`,
then issue the command `sudu systemctl start pacd`.  This will start `pacd` as a service, and it
will persist until the next boot.  To stop the service, use `sudu systemctl stop pacd`.  In order for `pacd`
to persist across boots, issue `sudo systemctl enable pacd`; `sudo systemctl disable pacd` will reverse this effect.

To ensure that the service has been started, use either the `sudo systemctl status pacd -l` or `sudo journalctl -xe`.
Using `systemctl`, successful startup will display something similar to the following:

```
sudo systemctl status pacd -l
● pacd.service - PAC BMC sensor monitor
   Loaded: loaded (/usr/lib/systemd/system/pacd.service; disabled; vendor preset: disabled)
   Active: active (running) since Thu 2018-08-23 09:34:59 PDT; 2s ago
  Process: 15694 ExecStart=/usr/local/bin/pacd -d $DefaultGBSOptions -P /usr/local/bin -m $UMask -l $LogFile -p $PIDFile -i $PollInterval -c $CooldownInterval $BoardPCIAddr $ThresholdOptions $ExtraOptions (code=exited, status=0/SUCCESS)
 Main PID: 15698 (pacd)
   CGroup: /system.slice/pacd.service
           └─15698 /usr/local/bin/pacd -d -n /etc/GBSs/default.gbs -P /usr/local/bin -m 0 -l /tmp/pacd.log -p /tmp/pacd.pid -i 0 -c 0

Aug 23 09:34:59 sj-avl-d15-mc.avl systemd[1]: Starting PAC BMC sensor monitor...
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon requested
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: registering default bitstream "/etc/GBSs/default.gbs"
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon path is /usr/local/bin
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon umask is 0x0
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon log file is /tmp/pacd.log
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon pid file is /tmp/pacd.pid
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: Polling interval set to 0.000000 sec
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: Cooldown delay set to 0.000000 sec
Aug 23 09:34:59 sj-avl-d15-mc.avl systemd[1]: Started PAC BMC sensor monitor.
```

The `journalctl` output will look similar to:

```
Aug 23 09:34:59 sj-avl-d15-mc.avl systemd[1]: Starting PAC BMC sensor monitor...
-- Subject: Unit pacd.service has begun start-up
-- Defined-By: systemd
-- Support: http://lists.freedesktop.org/mailman/listinfo/systemd-devel
-- 
-- Unit pacd.service has begun starting up.
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon requested
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: registering NULL bitstream "/etc/GBSs/NULL.gbs"
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon path is /usr/local/bin
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon umask is 0x0
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon log file is /tmp/pacd.log
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon pid file is /tmp/pacd.pid
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: Polling interval set to 0.000000 sec
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: Cooldown delay set to 0.000000 sec
Aug 23 09:34:59 sj-avl-d15-mc.avl systemd[1]: Started PAC BMC sensor monitor.
-- Subject: Unit pacd.service has finished start-up
-- Defined-By: systemd
-- Support: http://lists.freedesktop.org/mailman/listinfo/systemd-devel
-- 
-- Unit pacd.service has finished starting up.
-- 
-- The start-up result is done.
```

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

`-n, --default-bitstream <file>`

Specify the default bitstream to program when a sensor value exceeds the specified threshold.
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
`<trigger_threshold>`), will cause the default bitstream specified with `-n` that matches the FPGA's PR
Interface ID to be programmed into the FPGA.  The sensor will be considered triggered (and no PR
performed) until its value drops below `<reset_threshold>`.

This option can be specified multiple times.

The sensors specified will be monitored for all specified PACs.  There is no mechanism for specifying
per-PAC sensor thresholds.

`-t, --lower-sensor-threshold <sensor>:<trigger_threshold>[:<reset_threshold>]`

Specify the threshold value for a sensor that, when exceeded (sensor value strictly less than
`<trigger_threshold>`), will cause the default bitstream specified with `-n` that matches the FPGA's PR
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
for a period of time, re-enable the driver and then PR the default bitstream into the device.

If this option is specified, `pacd` will skip disabling the driver and just PR the default bitstream
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
