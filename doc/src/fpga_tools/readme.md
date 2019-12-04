# fpga_tools #

# fpgainfo #

## NAME ##
_fpgainfo_ - FPGA information tool


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
# fpgaconf #

## NAME ##

fpgadiag - Configure a green bitstream to an FPGA

## SYNOPSIS ##

`fpgaconf [-hvn] [-b <bus>] [-d <device>] [-f <function>] [-s <socket>] <gbs>`

## DESCRIPTION ##

fpgaconf writes accelerator configuration bitstreams (also referred to as "green
bitstreams" to an FPGA device recognized by OPAE. In the process, it also checks
the green bitstream file for compatibility with the targeted FPGA and its
current infrastructure bitstream (the "blue bistream"). fpgaconf takes the
following arguments:

`-h, --help`

	Print usage information

`-v, --verbose`

	Print more verbose messages while enumerating and configuring. Can be
	given more than once

`-n, --dry-run`

	Perform enumeration, but skip any operations with side-effects (like the
	actual configuration of the bitstream

`-b, --bus`

	PCI bus number of the FPGA to target

`-d, --device`

	PCI device number of the FPGA to target

`-f, --function`

	PCI function number of the FPGA to target

`-s, --socket`

	Socket number of the FPGA to target

fpgaconf will enumerate available FPGA devices in the system and select
compatible FPGAs for configuration. If there are more than one candidate FPGAs
that are compatible with the given green bitstream, fpgaconf will exit and ask
you to be more specific in selecting the target FPGAs (e.g. by specifying a
socket number, or a PCIe bus/device/function).

## EXAMPLES ##

`fpgaconf my_green_bitstream.gbs`

	Program "my_green_bitstream.gbs" to a compatible FPGA

`fpgaconf -v -s 0 my_green_bitstream.gbs`

	Program "my_green_bitstream.gbs" to the FPGA in socket 0, if compatible,
	while printing out slightly more verbose information
# fpgad #

## NAME ##
fpgad - log errors and generate events

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

## SEE ALSO ##

umask

# fpgadiag #

## NAME ##
_fpgadiag_ - FPGA diagnosis and testing tool.


## SYNOPSIS ##
```console
fpgadiag [-m | --mode=] <mode> [-t | --target=] <target> [options]
```


## DESCRIPTION ##
_fpgadiag_ includes several tests to diagnose, test and report on the FPGA hardware.

`<mode>` chooses which test to run. `<target>` specifies on what platform to
run the test. `<target>` can be either `fpga` or `ase`, where `ase` stands for
"AFU Simulation Environment".

The tests that can be selected by `<mode>` include:

**lpbk1**

    The test performs loopback test on the number of cachelines specified with 
    the `BEGIN` option. _fpgadiag_ sets up source and  destination buffers in 
    main memory. The FPGA then performs a memcpy from a source buffer to the 
    destination buffer, one cacheline at a time. 

    A cacheline is 64 bytes. When `BEGIN = END`, you perform one iteration. When 
    `BEGIN = END + x`, you perform `x` iterations. The first iteration consists 
    of copying `BEGIN` cachelines; the second iteration consists of copying 
    `BEGIN+1` cache lines; the third iteration consists of copying `BEGIN+3` 
    cache lines, etc. 
    
    The latency is shown as the number of clock ticks. 
    
    When you specify `MULTI-CL`, you copy `MULTI-CL` cache lines at a time.
    There is always a WrFence. `WR-FENCE` chooses what virtual channel the 
    WrFence occurs on. 
    
    If you specify continuous mode with `--cont`, the program runs an iteration
    until the timeout specified in `TIMEOUT` completes.


**read**

    This test performs only a read, not a memcpy. It is used to measure read 
    bandwidth. 


**write** 

    This test is used to measure write bandwidth. 


**trput**

    This test measures both read and write bandwidth by performing 50% read and 
    50% write tests.


**sw**

    This is a send-and-respond (ping-pong) test where one side sends data and 
    waits for answer.

Each test requires presence of one of these bitstreams, as documented below.
Before running a test, make sure its required bitstream is properly configured
on the platform.

* **nlb mode 0** for the `lpbk1` test.
* **nlb mode 3** for the `trput`, `read`, and `write` tests.
* **nlb mode 7** for the `sw` test.


## OPTIONS ##
### Common options ###
`--help, -h`

    Print help information and exit.

`--target=, -t`

    Values accepted for this switch are fpga or ase. Default=fpga

`--mode=, -m`

    The test to run. Values accepted for this switch are `lpbk1`, `read`,
    `write`, `trput`, `sw`

`--config=, -c`

    A configuration file in the JSON format that specifies options for a test.
    If an option is specified both in the configuration file and on the command 
    line, the value in the configuration file prevails

`--socket-id=, -s`

    Socket ID encoded in BBS. Default=0 

`--bus-number=, -B`

    Bus number of the PCIe device. Default=0 

`--device=, -D`

    Device number of the PCIe device. Default=0 

`--function=, -F`

    Function number of the PCIe device. Default=0 

`--freq=, -T`

    Clock frequency in Hz. Default=400 MHz 

`--suppress-hdr, -S`

    Suppress column headers for text output. Default=off

`--csv, -V`

    Comma separated value format. Default=off 


### **lpbk1** test options ###
`--guid=, -g`

    Accelerator ID to enumerate. Default=D8424DC4-A4A3-C413-F89E-433683F9040B 

`--begin=B, -b`

    1 <= B <= 65535. Default=1, B = number of cache lines 

`--end=E, -e`

    1 <= E <= 65535. Default=B, B and E designate number of cache lines 

`--multi-cl=M, -U`

    M can equal 1, 2, or 4. Default=1 

`--cont, -L`

    Continuous mode. Default=off 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode (microseconds portion default=0; milliseconds 
    portion default=0; seconds portion default=1; minutes portion default=0;
    hours portion default=0)

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I Default=wrline-M 

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. Default=rdline-I 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. Default=auto 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. Default=auto 

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1. Default=auto 


### **read** test options ###
`--guid=, -g`

    Accelerator ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

`--begin=B, -b`

    1 <= B <= 65535. Default=1, B = number of cache lines 

`--end=E, -e`

    1 <= E <= 65535. Default=B, B and E designate number of cache lines 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. Default=1 

`--strided-access=S, -a`

    1<= S <= 64. Default=1 

`--cont, -L`

    Continuous mode. Default=off 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode (microseconds portion default=0; milliseconds 
    portion default=0; seconds portion default=1; minutes portion default=0;
    hours portion default=0)

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. Default=rdline-I 

`--warm-fpga-cache -H; --cool-fpga-cache -M`

    Attempt to prime the cache with hits. Default=off, Attempt to prime the 
    cache with misses. Default=off 

`--cool-cpu-cache, -C`

    Attempt to prime the cpu cache with misses. Default=off 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. Default=auto 


### **write** test options ###
`--guid=, -g`

    Accelerator ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

`--begin=B, -b`

    1 <= E <= 65535. Default=B, B and E designate number of cache lines 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. Default=1 

`--strided-access=S, -a`

    1<= S <= 64. Default=1 

`--cont, -L`

    Continuous mode. Default=off 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode (microseconds portion default=0; milliseconds 
    portion default=0; seconds portion default=1; minutes portion default=0;
    hours portion default=0)

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I Default=wrline-M 

`--warm-fpga-cache -H; --cool-fpga-cache -M`

    Attempt to prime the cache with hits. Default=off, Attempt to prime the 
    cache with misses. Default=off 

`--cool-cpu-cache, -C`

    Attempt to prime the cpu cache with misses. Default=off 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. Default=auto 

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1, random. Default=`WRITE-VC`

`--alt-wr-pattern, -l`

    Alternate Write Pattern. Default=off 


### **trput** test options ###
`--guid=, -g`

    Accelerator ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

`--begin=B, -b`

    1 <= B <= 65535. Default=1, B = number of cache lines 

`--end=E, -e`

    1 <= E <= 65535. Default=B, B and E designate number of cache lines 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. Default=1 

`--strided-access=S, -a`

    1<= S <= 64. Default=1 

`--cont, -L`

    Continuous mode. Default=off 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode (microseconds portion default=0; milliseconds 
    portion default=0; seconds portion default=1; minutes portion default=0;
    hours portion default=0)

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I Default=wrline-M 

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. Default=rdline-I 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. Default=auto 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. Default=auto 

`--wrfence-vc=, -f`

    Can be  auto, vl0, vh0, vh1. Default=`WRITE-VC`


### **sw** test options ###
`--guid=, -g`

    Accelerator ID to enumerate. Default=7BAF4DEA-A57C-E91E-168A-455D9BDA88A3 

`--begin=B, -b`

    1 <= B <= 65535. Default=1, B = number of cache lines 

`--end=E, -e`

    1 <= E <= 65535. Default=B, B and E designate number of cache lines 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. Default=1 

`--strided-access=S, -a`

    1<= S <= 64. Default=1 

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I. Default=wrline-M 

`--cache-hint= -i`

    Can be rdline-I or rdline-S. Default=rdline-I 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random Default=auto 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random Default=auto 

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1. Default=`WRITE-VC`

`--notice=, -N`

    Can be poll, csr-write, umsg-data, or umsg-hint. Default=poll 


## EXAMPLES ##
This command starts an `lpbk1` test on the FPGA on bus `0x5e`. The test 
copies 57535, 57536, 57537, ..., up to 65535 cache lines, one line at a time.
The test output is printed in the CSV format with header suppressed.
```console
./fpgadiag --mode=lpbk1 --target=fpga -SV --bus-number=0x5e --begin=57535
--end=65535 --cache-hint=rdline-I --cache-policy=wrpush-I --multi-cl=1
--write-vc=vl0 --read-vc=vh1 --wrfence-vc=auto
```

This command starts a `read` test on the FPGA located on bus `0xbe`. The test
reads 2045 cache lines in the continuous mode with a 15-second timeout period. 
Data is accessed with a strided pattern with a 10-byte stride length.
```console
./fpgadiag --mode=read --target=fpga -SV --bus-number=0xbe --begin=2045 --cont
--timeout-sec=15   --cache-hint=rdline-I --multi-cl=1 -a=10 --write-vc=vh1
--read-vc=auto --wrfence-vc=auto
```

This command starts an `sw` test on the FPGA located on bus `0xbe`. The test
notifies completion using a CSR write.
```console
./fpgadiag --mode=sw --target=fpga -SV --bus-number=0xbe --begin=4 --end=8192
--cache-hint=rdline-I --cache-policy=wrline-I --notice=csr-write --write-vc=vl0
--wrfence-vc=auto --read-vc=random 
```


## TROUBLESHOOTING ##
When a test fails to run or gives errors, check the following:

* Is Intel FPGA driver properly installed? 
See [Installation Guide](/fpga-doc/docs/install_guide/installation_guide.html) 
for driver installation instructions.
* Are FPGA port permissions set properly? Check the permission bits of the
port, for example, `/dev/intel-fpga-port-0`. Users need READ and WRITE
permissions to run `fpgadiag` tests.
* Is hugepage properly configured on the system? 
See [Installation Guide](/fpga-doc/docs/install_guide/installation_guide.html)
for hugepage configuration steps.
* Is the required bitstream loaded? See [DESCRIPTION](#description) for
information about what bitstream is required by which test.
* Are `--begin` and `--end` values set properly? `--end` must be no
smaller than the `--begin`. Also, `--begin` must be a multiple of the
`--multi-cl` value.
* The `--warm-fpga-cache` and `--cool-fpga-cache` options in the `read`
and `write` tests are mutually exclusive.
* The timeout options are only meaningful for the continuous mode 
(with the `--cont` option).

# mmlink #

## NAME ##
 MMLink - Debugging RTL.

## SYNOPSIS  ##

`mmlink [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <TCP port>] [-I <IP Address>]`


## DESCRIPTION ##

Remote signaltap is software  tool used for debug RTL (AFU), effectively a signal trace capability that Quartus places into a green bitstream.
Remote Signal Tap provides  access the RST part of the Port MMIO space, and then runs the remote protocol on top.

## EXAMPLES  ##

`./mmlink  -B 0x5e -P 3333`

  MMLink app starts and listens for connection.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-P,--port` TCP port number.

`-I,--ip ` IP address of FPGA system. 


## NOTES ##

Driver privilege:

Change AFU driver privilege to user .

command: chmod 777 /dev/intel-fpga-port.0

set memlock:    

command: ulimit -l 10000

# coreidle #

## NAME ##
 coreidle - idles cores for shared TDP sockets to run online cores at maximum capacity.

## SYNOPSIS  ##

`coreidle [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-G <GBS path>] `


## DESCRIPTION ##

This tools parses input GBS, extracts power from metadata ,calculates fpga power, number of online and idle cores.
It moves threads from idle cores to online cores.

## EXAMPLES  ##

`./coreidle  -B 0x5e -G /home/lab/gbs/mode0.gbs`

  Idle cores to run online cores at maximum capacity.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-G,--gbs` Green bitstream file path.
 
# fpgamux #

## NAME ##
    fpgamux - Software MUX for running multiple AFU (accelerator functional unit) tests in one GBS (green bitsream)

## SYNOPSIS ##
```console
fpgamux [-h] [-S|--socket-id SOCKET_ID] [-B|--bus-number BUS] [-D|--device DEVICE] [-F|--function FUNCTION]
          [-G|--guid GUID] -m|--muxfile MUXFILE.json
```

## DESCRIPTION ##
fpgamux is a testing tool to interact with multiple AFUs that have been synthesized into one GBS along with
the CCIP-MUX BBB (basic building block). The CCIP-MUX uses upper bits in the MMIO addresses to route MMIO
reads/writes to the AFU running on the corresponding CCIP-MUX port. fpgamux uses a configuration file that
lists the software components and configuration to use.

.. note::

```
  Only one (the first) AFU is discoverable by the OPAE driver. Enumerating acceleration on an FPGA will find
  the accelerator associated with the first AFU only. The first software component in the configuration will
  be used to determine the GUID to use for enumeration. This can be overridden with the -G|--guid option.
```


## OPTIONS ##
    -S SOCKET_ID, --socket-id SOCKET_ID
       socket id of FPGA resource

    -B BUS, --bus BUS
       bus id of FPGA resource

    -D DEVICE, --device DEVICE
       device id of FPGA resource


    -F FUNCTION, --function FUNCTION
       function id of FPGA resource

    -G, --guid
       specify what guid to use for the accelerator enumeration

## CONFIGURATION ##
fpgamux uses a configuration file (in JSON format) to determine what software components to instantiate and
how to configure them for interacting with the AFUs in the GBS. This schema for this is listed below:


    [
        {
            "app" : "fpga_app",
            "name" : "String",
            "config" : "Object"
        }
    ]


## EXAMPLES ##
An example configuration with two components is listed below:

    [
        {
            "app" : "nlb0",
            "name" : "nlb0",
            "config" :
            {
                "begin" : 1,
                "end" : 1,
                "multi-cl" : 1,
                "cont" : false,
                "cache-policy" : "wrline-M",
                "cache-hint" : "rdline-I",
                "read-vc" : "vh0",
                "write-vc" : "vh1",
                "wrfence-vc" : "write-vc",
                "timeout-usec" : 0,
                "timeout-msec" : 0,
                "timeout-sec" : 1,
                "timeout-min" : 0,
                "timeout-hour" : 0,
                "freq" : 400000000
            }
        },
        {
            "app" : "nlb3",
            "name" : "nlb3",
            "config" :
            {
                "mode" : "read",
                "begin" : 1,
                "end" : 1,
                "multi-cl" : 1,
                "strided-access" : 1,
                "cont" : false,
                "warm-fpga-cache" : false,
                "cool-fpga-cache" : false,
                "cool-cpu-cache" : false,
                "cache-policy" : "wrline-M",
                "cache-hint" : "rdline-I",
                "read-vc" : "vh0",
                "write-vc" : "vh1",
                "wrfence-vc" : "write-vc",
                "alt-wr-pattern" : false,
                "timeout-usec" : 0,
                "timeout-msec" : 0,
                "timeout-sec" : 1,
                "timeout-min" : 0,
                "timeout-hour" : 0,
                "freq" : 400000000
            }
        }
    ]

# userclk #

## NAME ##
 userclk - to set afu high and low clock frequency.

## SYNOPSIS  ##

`userclk [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <Port id>] [-H <User clock high frequency>] -L <User clock low frequency>]`


## DESCRIPTION ##

userclk tool used to set high and low clock frequency to acceleration function unit.

## EXAMPLES  ##

`./userclk  -B 0x5e -H 400 -L 200`

 Sets AFU frequency.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-P,--port` Port id.

`-H,--freq-high ` User clock high frequency. 

`-L,--freq-low ` User clock low frequency. 




