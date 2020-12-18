# host_exerciser #

## SYNOPSIS ##
```console
Usage: host_exerciser [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off}=info
                              stdout logging level
  -s,--shared                 open in shared mode, default is off
  -t,--timeout UINT=60000     test timeout (msec)
  -m,--mode UINT:value in {lpbk->0,read->1,trput->3,write->2} OR {0,1,3,2}=lpbk
                              host exerciser mode {lpbk,read, write, trput}
  --cl UINT:value in {cl_1->0,cl_2->1,cl_3->2,cl_4->2} OR {0,1,2,2}=cl_1
                              number of CLs per request{cl_1, cl_2, cl_3, cl_4}
  --ccont BOOLEAN=0           Configures test rollover or test termination
  -d,--delay BOOLEAN=0        Enables random delay insertion between requests

Subcommands:
  lpbk                        run simple loopback test
  mem                         run simple mem test


```



## DESCRIPTION ##
The host exerciser used to exercise and characterize the various host-FPGA
interactions eg. MMIO, Data transfer from host to FPGA , PR, host to FPGA memory etc.

### Host Exerciser Loopback (HE-LBK) ###
Host Exerciser Loopback (HE-LBK) AFU can move data between host memory and FPGA.

HE-LBK supports:
1. Latency (AFU to Host memory read)
2. MMIO latency (Write+Read)
3. MMIO BW (64B MMIO writes)
4. BW (Read/Write, Read only, Wr only)

### Host Exerciser Loopback Memory (HE-Mem) ###
The host exerciser used to exercise use of FPGA connected DDR, data read
from the host is written to DDR, and the same data is read from DDR before sending it back to the host


### HOST EXERCISER SUB COMMANDS ###
`lpbk`

run host exerciser loopback test

`mem`

run host exerciser memory test


## OPTIONAL ARGUMENTS ##
`--help, -h`

Prints help information and exit.


## COMMON ARGUMENTS / OPTIONS ##
The following arguments are common to all commands and are optional.

` -p,--pci-address`

PCIe domain, bus, device, function number of fpga resource.

`-l,--log-level`

set host exerciser tool log level, trace, debug, info, warning, error, critical, off

`-s,--shared `

open FPGA PCIe resource in shared mode

`-t,--timeout`

host exerciser tool time out, by default time out 60000

`-m,--mode`

host exerciser modes are supported lpbk,read, write, trput


`--cl`

Number of CLs per request 1, 2, 3, 4.


`--ccont`

Configures test rollover or test termination.


`-d,--delay`

Enables random delay insertion between requests.



## EXAMPLES ##
This command exerciser Loopback afu:
```console
./host_exerciser lpbk
```

This command exerciser memory afu:
```console
./host_exerciser mem
```

This command exerciser Loopback afu on pcie 000:3b:00.0:
```console
./host_exerciser --pci-address 000:3b:00.0    lpbk


This command exerciser Loopback afu on pcie 000:3b:00.0 and run in write mode:
```console
./host_exerciser --pci-address 000:3b:00.0   --mode write lpbl



## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Updated description of the `fme` command | 
