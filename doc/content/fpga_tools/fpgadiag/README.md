# fpgadiag #

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

    AFU ID to enumerate. Default=D8424DC4-A4A3-C413-F89E-433683F9040B 

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

    AFU ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

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

    AFU ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

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

    AFU ID to enumerate. Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

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

    AFU ID to enumerate. Default=7BAF4DEA-A57C-E91E-168A-455D9BDA88A3 

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
for hugepage configuration steps. In particular, `fpgadiag` requires a few 1GB
pages to be available. 
* Is the required bitstream loaded? See [DESCRIPTION](#description) for
information about what bitstream is required by which test.
* Are `--begin` and `--end` values set properly? `--end` must be no
smaller than the `--begin`. Also, `--begin` must be a multiple of the
`--multi-cl` value.
* The `--warm-fpga-cache` and `--cool-fpga-cache` options in the `read`
and `write` tests are mutually exclusive.
* The timeout options are only meaningful for the continuous mode 
(with the `--cont` option).

