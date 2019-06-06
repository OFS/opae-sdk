# fpgadiag #

## SYNOPSIS ##
```console
fpgadiag [-m | --mode=] <mode> [-t | --target=] <target> [options]
```


## DESCRIPTION ##
Includes several tests to diagnose, test, and report on the FPGA hardware.

```<mode>``` chooses which test to run. 
```<target>``` specifies the platform that runs the test.
```<target>``` can be either ```fpga``` or ```ase``` where ```ase```. 
```<ase>``` is the abbreviation for Accelerator Simulation Environment.

The ```<mode>``` selects from the  following tests:

**lpbk1**

This test runs a loopback test on the number of cachelines specified with 
the ```BEGIN``` option. ```fpgadiag``` sets up source and  destination buffers in 
main memory. The FPGA then performs a ```memcpy``` from a source buffer to the 
destination buffer, one cacheline at a time. 

A cacheline is 64 bytes. When `BEGIN = END`, the test performs one iteration. When 
`BEGIN = END + x`, the test performs `x` iterations. The first iteration consists 
of copying `BEGIN` cachelines; the second iteration consists of copying 
`BEGIN+1` cache lines. The third iteration consists of copying `BEGIN+2` 
cache lines, and so on. 
    
The latency is shown as the number of clock cycles. 
    
When you specify `MULTI-CL`, you copy `MULTI-CL` cache lines at a time.
The WR-FENCE chooses on which virtual channel the WrFence occurs.
     
    
If you specify continuous mode with `--cont`, the program iterates
until the timeout specified in `TIMEOUT` completes.


**read**

This test performs reads. Use this test to measure read bandwidth. 
    


**write** 

This test performs writes. Use it to measure write bandwidth. 


**trput**

This test measures both read and write bandwidth by performing 50% read and 
50% write tests.


**sw**

This is a send-and-respond (ping-pong) test. One side sends data and 
waits for response.

Each test requires a particular AF. Before running a test,
make sure the required AF is properly configured
on the platform. 

* The lpbk1 test requires the nlb mode 0 AF.
* The trput test requires the nlb mode 3 AF. 
* The sw test requires the nlb mode 7 AF. This AF is only available for the integrated FPGA platform.
     You cannot run it on the PCIe accelerator card (PAC).


**fpgalpbk**

This enable/disable FPGA loopback.


**fpgastats**

This get fpga mac statistics.


**mactest**

This compare mac addresses that read from MAC ROM with mac addresses read from Host side.


## OPTIONS ##
### Common options ###
`--help, -h`

    Print help information and exit.

`--target=, -t`

    This switch specifies fpga (hardware) or ase (simulation). The default=fpga.

`--mode=, -m`

    The test to run. The valid values are `lpbk1`, `read`,
    `write`, `trput`, and `sw`.

`--config=, -c`

    A configuration file in the JSON format that specifies options for a test.
    If an option is specified both in the configuration file and on the command 
    line, the value in the configuration file takes precedence.

`--dsm-timeout-usec`

    Timeout in microseconds for test completion. The test fails if not completed by 
    specified timeout. The default=1000000.

`--socket-id=, -s`

    Socket ID encoded in FPGA Interface Manager (FIM). The default=0. 

`--bus=, -B`

    Bus number of the PCIe device. The default=0. 

`--device=, -D`

    Device number of the PCIe device. The default=0. 

`--function=, -F`

    Function number of the PCIe device. The default=0. 

`--freq=, -T`

    Clock frequency (in Hz) used for bandwidth calculation. The default=400000000 Hz (400 MHz). 
```eval_rst
.. note::
    This frequency is used only when the software cannot infer the frequency from the accelerator.
```

`--suppress-hdr, -S`

    Suppress column headers for text output. The default=off.

`--csv, -V`

    Comma separated value format. The default=off. 

`--suppress-stats`

    Suppress statistics output at the end of test. The default=off.

### **lpbk1** test options ###
`--guid=, -g`

    AFU ID to enumerate. The default=D8424DC4-A4A3-C413-F89E-433683F9040B. 

`--begin=B, -b`

    1 <= B <= 65535. The default=1, B = number of cache lines. 

`--end=E, -e`

    1 <= E <= 65535. The default=B, B and E designate number of cache lines. 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. The default=1. 

`--cont, -L`

    Continuous mode. The default=off. 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode. The default for all options is 0. 

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I The default=wrline-M.

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. The default=rdline-I.

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. The default=auto. 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. The default=auto. 

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1. The default=auto. 


### **read** test options ###
`--guid=, -g`

    AFU ID to enumerate. The default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18. 

`--begin=B, -b`

    1 <= B <= 65535. The default=1, B = number of cache lines. 

`--end=E, -e`

    1 <= E <= 65535. The default=B, B and E designate number of cache lines. 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. The default=1. 

`--strided-access=S, -a`

    1<= S <= 64. The default=1. 

`--cont, -L`

    Continuous mode. The default=off. 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode. The default for all options is 0.

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. The default=rdline-I. 

`--warm-fpga-cache -H; --cool-fpga-cache -M`

    Try to prime the cache with hits. The default=off. Try to prime the 
    cache with misses. The default=off.

`--cool-cpu-cache, -C`

    Try to prime the cpu cache with misses. The default=off. 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. The default=auto 


### **write** test options ###
`--guid=, -g`

    AFU ID to enumerate. The default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18 

`--begin=B, -b`

    1 <= E <= 65535. The default=B, B and E designate number of cache lines. 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. The default=1.

`--strided-access=S, -a`

    1<= S <= 64. The default=1.

`--cont, -L`

    Continuous mode. The default=off.

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode. The default for all options is 0.

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I The default=wrline-M 

`--warm-fpga-cache -H; --cool-fpga-cache -M`

    Try to prime the cache with hits. The default=off. Try to prime the 
    cache with misses. The default=off. 

`--cool-cpu-cache, -C`

    Try to prime the cpu cache with misses. The default=off. 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. The default=auto. 

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1, random. The default=`WRITE-VC`.

`--alt-wr-pattern, -l`

    Alternate Write Pattern. The default=off. 


### **trput** test options ###
`--guid=, -g`

    AFU ID to enumerate. The default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18.

`--begin=B, -b`

    1 <= B <= 65535. The default=1, B = number of cache lines. 

`--end=E, -e`

    1 <= E <= 65535. The default=B, B and E designate number of cache lines. 

`--multi-cl=M, -u`

    M can equal 1, 2, or 4. The default=1. 

`--strided-access=S, -a`

    1<= S <= 64. The default=1 

`--cont, -L`

    Continuous mode. The default=off. 

`--timeout-usec=, --timeout-msec=, --timeout-sec=, --timeout-min=, --timeout-hour=`

    timeout for --cont mode. The default for all options is 0.

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I The default=wrline-M. 

`--cache-hint=, -i`

    Can be rdline-I or rdline-S. The default=rdline-I. 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random. The default=auto. 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random. The default=auto. 

`--wrfence-vc=, -f`

    Can be  auto, vl0, vh0, vh1. The default=`WRITE-VC`.


### **sw** test options ###
`--guid=, -g`

    AFU ID to enumerate. The default=7BAF4DEA-A57C-E91E-168A-455D9BDA88A3. 

`--begin=B, -b`

    1 <= B <= 65535. The default=1, B = number of cache lines. 

`--end=E, -e`

    1 <= E <= 65535. The default=B, B and E designate number of cache lines. 

`--cache-policy=, -p`

    Can be wrline-I, wrline-M, or wrpush-I. The default=wrline-M. 

`--cache-hint= -i`

    Can be rdline-I or rdline-S. The default=rdline-I. 

`--read-vc=, -r`

    Can be auto, vl0, vh0, vh1, random The default=auto. 

`--write-vc=, -w`

    Can be auto, vl0, vh0, vh1, random The default=auto.

`--wrfence-vc=, -f`

    Can be auto, vl0, vh0, vh1. The default=`WRITE-VC`.

`--notice=, -N`

    Can be poll or csr-write. The default=poll. 


### **fpgalpbk** test options ###
`--enable`

    Enable fpga phy loopback.

`--disable`

    Disable fpga phy loopback.

`--direction`

    Can be local, remote.

`--type`

    Can be serial, precdr, postcdr.

`--side`

    Can be line, host.

`--port`

    0 <= port <= 7, the default is all.


### **mactest** test options ###
`--offset`

    Read mac addresses from an offset, The default=0.


## EXAMPLES ##
This command starts a `lpbk1` test for the FPGA on bus `0x5e`. The test 
copies 57535, 57536, 57537 ... up to 65535 cache lines, one line at a time.
The test prints output in the comma separated values (CSV) format with the
header suppressed.
```console
./fpgadiag --mode=lpbk1 --target=fpga -V --bus=0x5e --begin=57535
--end=65535 --cache-hint=rdline-I --cache-policy=wrpush-I --multi-cl=1
--write-vc=vl0 --read-vc=vh1 --wrfence-vc=auto
```

This command starts a `read` test on the FPGA located on bus `0xbe`. The test
reads 2045 cache lines in the continuous mode with a 15-second timeout period. 
The reads use a strided pattern with a 10-byte stride length.
```console
./fpgadiag --mode=read --target=fpga -V --bus=0xbe --begin=2045 --cont
--timeout-sec=15 --cache-hint=rdline-I --multi-cl=1 -a=10 
--read-vc=auto --wrfence-vc=auto
```

This command starts a `sw` test on the FPGA located on bus `0xbe`. The test
signals completion using a CSR write.
```console
./fpgadiag --mode=sw --target=fpga -V --bus=0xbe --begin=4 --end=8192
--cache-hint=rdline-I --cache-policy=wrline-I --notice=csr-write --write-vc=vl0
--wrfence-vc=auto --read-vc=random 
```


This command enable a `fpgalpbk` on the FPGA located on bus `0xbe`.
```console
./fpgadiag -m fpgalpbk --bus 0xbe --enable --direction local --type postcdr
--side host
```


This command show `fpgastats` on the FPGA located on bus `0xbe`.
```console
./fpgadiag -m fpgastats --bus 0xbe
```


## TROUBLESHOOTING ##
When a test fails to run or gives errors, check the following:

* Is the Intel FPGA driver properly installed? 
See [Installation Guide](/fpga-doc/docs/install_guide/installation_guide.html) 
for driver installation instructions.
* Are FPGA port permissions set properly? Check the permission bits of the
port, for example, `/dev/intel-fpga-port-0`. You need READ and WRITE
permissions to run `fpgadiag` tests.
* Is hugepage properly configured on the system? 
See [Installation Guide](/fpga-doc/docs/install_guide/installation_guide.html)
for hugepage configuration steps. In particular, `fpgadiag` requires a few 1 GB
pages. 
* Is the required AFU loaded? See [DESCRIPTION](#description) for
information about what AFU the test requires.
* Are `--begin` and `--end` values set properly? `--end` must be larger
than the `--begin`. Also, `--begin` must be a multiple of the
`--multi-cl` value.
* The `--warm-fpga-cache` and `--cool-fpga-cache` options in the `read`
and `write` tests are mutually exclusive.
* The timeout options are only meaningful for the continuous mode 
(with the `--cont` option).

## Revision History ##

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) | fpgadiag now reports the correct values for bandwidth. |
