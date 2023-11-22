# Time of Day (ToD) #
## SYNOPSIS ##

The Intel FPGA ToD driver in the kernel space exposes ToD IP as PHC (PTP Hardware Clock) device to the Linux PTP (Precision Time Protocol) stack to synchronize the system clock to its ToD information.
The phc2sys utility of Linux PTP stack is used to access ToD information and synchronize the system clock.

Install the Linux PTP utilities:
```console
# sudo yum install linuxptp
```

phc_ctl and phc2sys utilities (linuxptp package) are used to control the PHC device and synchronize the system clock to its ToD information.

phc_ctl: directly controls PHC device clock.
```console
Usage: phc_ctl [options] <device> -- [command]

    device         ethernet or ptp clock device
 
Options:
    -l [num]       set the logging level to 'num'
    -q             do not print messages to the syslog
    -Q             do not print messages to stdout
    -v             prints the software version and exits
    -h             prints this message and exits

Commands:
 specify commands with arguments. Can specify multiple commands to be executed in order. 
 Seconds are read as double precision floating point values.

    set  [seconds]  set PHC time (defaults to time on CLOCK_REALTIME)
    get             get PHC time
    adj  <seconds>  adjust PHC time by offset
    freq [ppb]      adjust PHC frequency (default returns current offset)
    cmp             compare PHC offset to CLOCK_REALTIME
    caps            display device capabilities (default if no command given)
    wait <seconds>  pause between commands
                    This command may be useful for sanity checking whether the PHC clock is
                    running as expected.
                    The arguments specified in seconds are read as double precision floating point
                    values, and will scale to nanoseconds. This means providing a value of 5.5 means 5
                    and one half seconds. This allows specifying fairly precise values for time.
```

phc2sys: synchronize two clocks.
```console
Usage: phc2sys [options]

Automatic configuration:
    -a             turn on autoconfiguration
    -r             synchronize system (realtime) clock
                   repeat -r to consider it also as a time source
 
Manual configuration:
    -c [dev|name]  slave clock (CLOCK_REALTIME)
    -d [dev]       master PPS device
    -s [dev|name]  master clock
    -O [offset]    slave-master time offset (0)
    -w             wait for ptp4l
 
Options:
    -f [file]      configuration file
    -E [pi|linreg] clock servo (pi)
    -P [kp]        proportional constant (0.7)
    -I [ki]        integration constant (0.3)
    -S [step]      step threshold (disabled)
    -F [step]      step threshold only on start (0.00002)
    -R [rate]      slave clock update rate in HZ (1.0)
    -N [num]       number of master clock readings per update (5)
    -L [limit]     sanity frequency limit in ppb (200000000)
    -M [num]       NTP SHM segment number (0)
    -u [num]       number of clock updates in summary stats (0)
    -n [num]       domain number (0)
    -x             apply leap seconds by servo instead of kernel
    -z [path]      server address for UDS (/var/run/ptp4l)
    -l [num]       set the logging level to 'num' (6)
    -t [tag]       add tag to log messages
    -m             print messages to stdout
    -q             do not print messages to the syslog
    -v             prints the software version and exits
    -h             prints this message and exits
```

## DESCRIPTION ##

The phc2sys utility is used to synchronize the system clock to the PTP Hardware Clock (PHC) or ToD clock. The phc_ctl utility is used to directly control PHC clock device.

### Configuring the PTP service ###

1.  Install the linuxptp package:
```console
# sudo yum install linuxptp
```
2.  Check PTP device is created successfully by the ToD driver. 

ToD driver registering as PHC device (clock_name: dfl_tod) to the Linux PTP stack and exposing to the Linux kernel to synchronize the system clock to its ToD information.
```console
# cat /sys/class/ptp/ptp0/clock_name
dfl_tod
```

3.  Configure phc2sys service on a system:

The phc2sys service is configured in the /etc/sysconfig/phc2sys configuration file. Define start-up option for phc2sys daemon in /etc/sysconfig/phc2sys.
The master clock is /dev/ptp0 device and the slave clock is system clock/CLOCK_REALTIME:
```console
 OPTIONS="-s /dev/ptp0 -c CLOCK_REALTIME -r -O 0 -R 16"
```

4.  Start phc2sys service:
```console
# service phc2sys start
```

5.  Stop phc2sys service:
```console
# service phc2sys stop
```

## Examples ##

### using phc_ctl utility ###

Read the current clock time from the PHC clock device:
```console
# sudo phc_ctl /dev/ptp0 get
```

Set the PHC clock time to CLOCK_REALTIME:
```console
# sudo phc_ctl /dev/ptp0 set
```

Set PHC clock time to 0:
```console
# sudo phc_ctl /dev/ptp0 set 0.0
```

Set PHC clock time to 0 and wait for 10 sec and read the clock time:
```console
# sudo phc_ctl /dev/ptp0 set 0.0 wait 10.0 get
```

Set and compare PHC clock time to CLOCK_REALTIME:
```console
# sudo phc_ctl /dev/ptp0 set cmp
```

Read the PHC device capabilities:
```console
# sudo phc_ctl /dev/ptp0 caps
```


### using phc2sys utility ###
To synchronize the system clock to the PHC clock:
```console
# sudo phc2sys -s /dev/ptp0 -c CLOCK_REALTIME -r -O 0 -R 16 -m
phc2sys[7896.789]: CLOCK_REALTIME phc offset  -1259509 s0 freq  -31462 delay   1338
phc2sys[7896.852]: CLOCK_REALTIME phc offset  -1261498 s1 freq  -63144 delay   1328
phc2sys[7896.914]: CLOCK_REALTIME phc offset       -15 s2 freq  -63159 delay   1328
phc2sys[7896.977]: CLOCK_REALTIME phc offset       -19 s2 freq  -63167 delay   1327
phc2sys[7897.039]: CLOCK_REALTIME phc offset       -35 s2 freq  -63189 delay   1328
phc2sys[7897.102]: CLOCK_REALTIME phc offset       -37 s2 freq  -63201 delay   1331
phc2sys[7897.165]: CLOCK_REALTIME phc offset       -30 s2 freq  -63205 delay   1328
phc2sys[7897.227]: CLOCK_REALTIME phc offset       -50 s2 freq  -63234 delay   1331
phc2sys[7897.290]: CLOCK_REALTIME phc offset       -50 s2 freq  -63249 delay   1329
phc2sys[7897.353]: CLOCK_REALTIME phc offset       -62 s2 freq  -63276 delay   1334
phc2sys[7897.415]: CLOCK_REALTIME phc offset       -53 s2 freq  -63286 delay   1335
phc2sys[7897.478]: CLOCK_REALTIME phc offset       -46 s2 freq  -63295 delay   1325
phc2sys[7897.541]: CLOCK_REALTIME phc offset       -57 s2 freq  -63320 delay   1334
phc2sys[7897.603]: CLOCK_REALTIME phc offset       -39 s2 freq  -63319 delay   1327
phc2sys[7897.666]: CLOCK_REALTIME phc offset       -31 s2 freq  -63323 delay   1328
phc2sys[7897.728]: CLOCK_REALTIME phc offset       -48 s2 freq  -63349 delay   1327
phc2sys[7897.791]: CLOCK_REALTIME phc offset       -42 s2 freq  -63357 delay   1323
phc2sys[7897.854]: CLOCK_REALTIME phc offset       -41 s2 freq  -63369 delay   1324
phc2sys[7897.916]: CLOCK_REALTIME phc offset       -44 s2 freq  -63384 delay   1328
phc2sys[7897.979]: CLOCK_REALTIME phc offset       -13 s2 freq  -63366 delay   1327
phc2sys[7898.042]: CLOCK_REALTIME phc offset       -16 s2 freq  -63373 delay   1327
phc2sys[7898.104]: CLOCK_REALTIME phc offset       -19 s2 freq  -63381 delay   1328
phc2sys[7898.167]: CLOCK_REALTIME phc offset       -16 s2 freq  -63384 delay   1327
phc2sys[7898.229]: CLOCK_REALTIME phc offset         3 s2 freq  -63370 delay   1327
phc2sys[7898.292]: CLOCK_REALTIME phc offset        16 s2 freq  -63356 delay   1325
phc2sys[7898.355]: CLOCK_REALTIME phc offset        10 s2 freq  -63357 delay   1319
phc2sys[7898.417]: CLOCK_REALTIME phc offset        23 s2 freq  -63341 delay   1327
phc2sys[7898.480]: CLOCK_REALTIME phc offset        13 s2 freq  -63344 delay   1335
phc2sys[7898.543]: CLOCK_REALTIME phc offset        23 s2 freq  -63330 delay   1323
phc2sys[7898.605]: CLOCK_REALTIME phc offset        29 s2 freq  -63317 delay   1312
phc2sys[7898.668]: CLOCK_REALTIME phc offset        22 s2 freq  -63315 delay   1324
phc2sys[7898.730]: CLOCK_REALTIME phc offset        42 s2 freq  -63289 delay   1325
phc2sys[7898.793]: CLOCK_REALTIME phc offset        29 s2 freq  -63289 delay   1333
phc2sys[7898.856]: CLOCK_REALTIME phc offset        34 s2 freq  -63276 delay   1327
phc2sys[7898.918]: CLOCK_REALTIME phc offset        21 s2 freq  -63278 delay   1331
phc2sys[7898.981]: CLOCK_REALTIME phc offset        22 s2 freq  -63271 delay   1335
phc2sys[7899.044]: CLOCK_REALTIME phc offset        30 s2 freq  -63256 delay   1327
phc2sys[7899.106]: CLOCK_REALTIME phc offset        30 s2 freq  -63247 delay   1328
phc2sys[7899.169]: CLOCK_REALTIME phc offset        37 s2 freq  -63231 delay   1333
phc2sys[7899.232]: CLOCK_REALTIME phc offset        29 s2 freq  -63228 delay   1331
phc2sys[7899.294]: CLOCK_REALTIME phc offset        24 s2 freq  -63225 delay   1330
```
