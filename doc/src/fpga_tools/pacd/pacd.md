# pacd #

## SYNOPSIS ##
`pacd --daemon [--directory=<dir>] [--logfile=<file>] [--pidfile=<file>] [--umask=<mode>] [--default-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

`pacd [--default-bitstream=<file>] [--segment=<PCIeSegment>] [--bus=<bus>] [--device=<device>] [--function=<function>] [--upper-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--lower-sensor-threshold=<sensor>:<threshold>[:<reset_thresh>]] [--poll-interval <sec>] [--cooldown-interval <sec>] [--no-defaults] [--driver-removal-disable]`

## DESCRIPTION ##
`pacd` periodically monitors the sensors on the Intel&reg; Programmable Acceleration Card (PAC)  Board Management Controller (BMC)
and programs a default bitstream in response to a sensor's value exceeding a specified threshold. pacd is only available on the PCIe\*
Accelerator Card (PAC).

On systems with multiple PACs, `pacd` monitors the sensors for all cards in the system using the specified
sensor threshold values.  If you specify the PCIe\* address  using the `-S`, `-B`, `-D`, `-F` parameters, `pacd` monitors all PACs
matching the PCIe\* address components that you specify.  For example, if you specify `-B 5` only, `pacd` monitors all PACs on
PCIe\* bus `5`.  The sensor thresholds are global. Specifying `-T 11:95.0:93.0` monitors sensor
`11` on all selected PACs. `pacd` triggers if its value exceeds `95.0` and resets its trigger at `93.0`.

Use SIGINT or SIGTERM to stop `pacd`. In daemon mode, run one of the following commands: 

``kill -2 `cat /tmp/pacd.pid` `` or 
``kill -15 `cat /tmp/pacd.pid` ``

In a regular process, type `^C`.


## INSTALLING AS A SYSTEM SERVICE ##
The tools installation process installs all the necessary files required to make `pacd` a `systemd` service, capable of
automatically starting on boot.

In order to start `pacd` as a `systemd` service, first edit the `/etc/sysconfig/pacd.conf` as root.  This file is shown
below.

```
# Intel Programmable Acceleration Card (PAC) daemon variables.
# Monitors Baseboard Management Controller (BMC) sensors.

############## REQUIRED OPTIONS ################

PIDFile=/tmp/pacd.pid

# Specify default Accelerator Function (AF now, formerly GBS) files to consider for partial reconfiguration (PR).
# Include '-n' for each.
# For example: DefaultGBSOptions=-n <Default_GBS_Path> -n <Default_GBS_PATH_2>
DefaultGBSOptions=-n <Default_GBS_Path>
UMask=0
LogFile=/tmp/pacd.log
PollInterval=0
CooldownInterval=0

############## OPTIONAL OPTIONS ################

# Uncomment and specify specific PAC PCIe\* addresses to monitor.
# Default is to monitor all PACs
#BoardPCIAddr=-S 0 -B 5 -D 0 -F 0

# Specify threshold values. -T for upper non-recoverable threshold (UNR), -t for lower non-recoverable threshold (LNR).
# ex.: ThresholdOptions=-T 4:12.5 -t 7:2.25:2.3
ThresholdOptions=

# Extra advanced options.
# ex.: ExtraOptions=--no-defaults --driver-removal-disable
ExtraOptions=
```

Edit the `DefaultGBSOptions=` line, specifying the absolute path(s) of the AF files to load into the device when a
threshold is exceeded.  Prefix each AF file name with `-n`.

Here are commands to start, stop and monitor the `pacd` service:

1. To rescan for services run the following command: `sudo systemctl daemon-reload`
2. To start `pacd` as a service that persists until the next boot, run the following command: `sudu systemctl start pacd`.  
3. To stop the service, run the following command: `sudu systemctl stop pacd`  
4. To have the `pacd` service persist across boots, run the following command:`sudo systemctl enable pacd`
5. To stop the persistent `pacd` service run the following command: `sudo systemctl disable pacd` will reverse this effect.
6.  To ensure that the service has started, use one of the following commands: `sudo systemctl status pacd -l` or `sudo journalctl -xe`

After a successful startup, the `systemctl` command displays current status information similar to the status information shown here:

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

The `journalctl` command displays status information similar to the output shown here:

```
Aug 23 09:34:59 sj-avl-d15-mc.avl systemd[1]: Starting PAC BMC sensor monitor...
-- Subject: Unit pacd.service has begun start-up
-- Defined-By: systemd
-- Support: http://lists.freedesktop.org/mailman/listinfo/systemd-devel
-- 
-- Unit pacd.service has begun starting up.
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: daemon requested
Aug 23 09:34:59 sj-avl-d15-mc.avl pacd[15694]: Thu Aug 23 09:34:59 2018: registering default bitstream "/etc/GBSs/nlb_mode_3.gbs"
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

When running in daemon mode, use the mode value as the file mode creation mask to pass to umask.
If omitted when daemonizing, `0` is used.

`-i, --poll-interval <secs>`

`pacd` polls and checks the sensor values every `secs` seconds.  This is a real number. Consequently,
you may specify a floating-point number such as `2.5` for a  2 1/2 poll interval.

`-c, --cooldown-interval <secs>`

Specifies the time in seconds that `pacd` waits after removing the FPGA driver before
re-enabling the driver. The `cooldown-interval` is the time that the host is not able to access the PAC for
any reason.  Not valid in conjunction with `--driver-removal-disable`.

`-n, --default-bitstream <file>`

Specify the default bitstream to program when a sensor value exceeds the specified threshold.
You can specify this option multiple times. `pacd` reconfigures using the AF that matches the FPGA's PR interface
ID when the sensor's value exceeds the threshold.

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
`<trigger_threshold>`), causes device reconfiguration. `pacd` reconfigures the FPGA with the default configuration bitstream 
you specify with `-n` that matches the FPGA's PR Interface ID.  The sensor is considered triggered (and no PR
performed) until the sensor value drops below `<reset_threshold>`.

You can specify this option multiple times.

The sensors specified are monitored for all of the PACs you specify. There is no mechanism for specifying
per-PAC sensor thresholds.

`-t, --lower-sensor-threshold <sensor>:<trigger_threshold>[:<reset_threshold>]`

Specify the threshold value for a sensor that, when exceeded (sensor value strictly less than
`<trigger_threshold>`), causes the default configuration bitstream specified with `-n` that matches the FPGA's PR
Interface ID to be programmed into the FPGA.  The sensor is considered triggered (and no PR
performed) until the sensor value goes above `<reset_threshold>`.

You can specify this option multiple times.

The `pacd` command monitors the sensors you specify for all the PACs you specify. There is no mechanism for specifying
per-PAC sensor thresholds.

`-N, --no-defaults`

By default, `pacd` monitors the same set of sensors that the BMC monitors that could trigger
a machine reboot.  This sensor set typically includes all settable non-recoverable thresholds.  Specifying
this option tells `pacd` not to monitor these sensors.  This option requires you to specify at least one of `-T`
or `-t` options.

`--driver-removal-disable`

The `--driver-removal-disable` option is an advanced option. The default value of this option is to disable the driver.
When a sensor initially trips requiring a PR of the FPGA, `pacd` performs the following actions: 

1. Removes the FPGA device driver for the device.
2. Waits for a period of time.
3. Re-enables the driver.
4. Reconfigures the default bitstream into the device.

If you specify this option, `pacd` skips disabling the driver and just reconfigures the default bitstream
into the device.

## NOTES ##

`pacd` intends to prevent an over-temperature or power "non-recoverable" event from causing the
FPGA's BMC to shut down the PAC.  Shutting down the PAC results in a PCIe `surprise removal`
which ultimately causes the host to reboot.

There are several issues that you should consider when enabling `pacd`:

1. The application being accelerated should respond appropriately when the device
driver disappears from the system. The application receives a SIGHUP signal when the driver
shuts itself down. On receipt of SIGHUP, the application should clean up and exit as soon as possible.
2. There is a window in which the running system reboots if a PR is in progress when
a sensor's threshold trips.
3. The OS and driver cannot invalidate any pointers that the application has to FPGA MMIO
space.  Intel strongly recommends using the OPAE API to access the MMIO region 
to avoid unanticipated reboots.
4. The OS and driver cannot prevent direct access of host memory from the FPGA, such as
a DMA operation from the AFU to the host. There is a high probability of a reboot if
the `pacd` attempts to PR the FPGA due to a threshold trip event during a DMA operation.

## TROUBLESHOOTING ##

If you encounter any issues, you can get debug information in two ways:

1. By examining the log file when in daemon mode.
2. By running in non-daemon mode and viewing stdout.

## EXAMPLES ##

The following command starts `pacd` as a daemon process, programming `nlb_mode_3.gbs` when
any BMC-triggerable threshold trips.

`pacd --daemon --default-bitstream=nlb_mode_3.gbs`

The following command starts `pacd` as a regular process, programming `nlb_mode_3.gbs` when
sensor 11 (FPGA Core TEMP) exceeds 92.35 degrees C or sensor 0 (Total Input Power) goes
out of the range [9.2 - 19.9] Watts.

`pacd -n=idle.gbs -T 11:92.35 -T 0:19.9 -t 0:9.2`

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.10.15 | DCP 1.2. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1) | Edits for clarity and style.  | 
 | 2018.08.17 | DCP 1.2 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1) | Updated to include new options.  | 
 | 2018.08.08 | DCP 1.2 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1) | Initial revision.  | 
