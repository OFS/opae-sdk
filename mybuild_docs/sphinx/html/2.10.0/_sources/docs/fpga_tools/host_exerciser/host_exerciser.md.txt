# host_exerciser #

## SYNOPSIS ##
```console
Usage: host_exerciser [OPTIONS] SUBCOMMAND


Options:
  -h,--help                   Print this help message and exit
  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off}=warning
                              stdout logging level
  -s,--shared                 open in shared mode, default is off
  -t,--timeout UINT=60000     test timeout (msec)
  -m,--mode UINT:value in {lpbk->0,read->1,trput->3,write->2} OR {0,1,3,2}=lpbk
                              host exerciser mode {lpbk,read, write, trput}
  --cls UINT:value in {cl_1->0,cl_2->1,cl_4->2,cl_8->3} OR {0,1,2,3}=cl_1
                              number of CLs per request{cl_1, cl_2, cl_4, cl_8}
  --continuousmode BOOLEAN=false
                              test rollover or test termination
  --atomic UINT:value in {cas_4->9,cas_8->11,fadd_4->1,fadd_8->3,off->0,swap_4->5,swap_8->7} OR {9,11,1,3,0,5,7}=off
                              atomic requests (only permitted in combination with lpbk/cl_1)
  --encoding UINT:value in {default->0,dm->1,pu->2,random->3} OR {0,1,2,3}=default
                              data mover or power user encoding -- random interleaves both in the same stream
  -d,--delay BOOLEAN=false    Enables random delay insertion between requests
  --interleave UINT=0         Interleave requests pattern to use in throughput mode {0, 1, 2}
                              indicating one of the following series of read/write requests:
                              0: rd-wr-rd-wr
                              1: rd-rd-wr-wr
                              2: rd-rd-rd-rd-wr-wr-wr-wr
  --interrupt UINT:INT in [0 - 3]
                              The Interrupt Vector Number for the device
  --contmodetime UINT=1       Continuous mode time in seconds
  --testall BOOLEAN=false     Run all tests
  --clock-mhz UINT=0          Clock frequency (MHz) -- when zero, read the frequency from the AFU

Subcommands:
  lpbk                        run simple loopback test
  mem                         run simple mem test

```



## DESCRIPTION ##
The host exerciser used to exercise and characterize the various host-FPGA
interactions eg. MMIO, Data transfer from host to FPGA , PR, host to FPGA memory etc.

### Host Exerciser Loopback (HE-LBK) ###
HE-LB is responsible for generating traffic with the intention of exercising the
path from the AFU to the Host at full bandwidth. 
Host Exerciser Loopback (HE-LBK) AFU can move data between host memory and FPGA.

HE-LBK supports:
1. Latency (AFU to Host memory read)
2. MMIO latency (Write+Read)
3. MMIO BW (64B MMIO writes)
4. BW (Read/Write, Read only, Wr only)

### Host Exerciser Memory (HE-MEM) ###
HE-MEM is used to exercise use of FPGA connected DDR; data read from the host is 
written to DDR, and the same data is read from DDR before sending it back to the 
host. HE-MEM uses external DDR memory (i.e. EMIF) to store data. It has a customized
version of the AVMM interface to communicate with the EMIF memory controller.

Execution of these exercisors requires the user to bind specific VF endpoint to vfio-pci
Bind the correct endpoint for a device with B/D/F 0000:b1:00.0

`[user@localhost]: sudo opae.io init -d 0000:b1:00.2 user:user`

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

host exerciser test modes are lpbk, read, write, trput


`--cls`

Number of cachelines per request 1, 2, 3, 4.


`--continuousmode`

Configures test rollover or test termination mode.


`--atomic`

atomic requests.


`--encoding`

select data mover mode or power user mode or random.


`-d,--delay`

Enables random delay insertion between requests.


`--interleave`

Enables interleave requests in throughput mode.
Value:3'b000-Rd,Wr,Rd,Wr
Value:3'b001-Rd,Rd,Wr,Wr
Value:3'b010-Rd,Rd,Rd,Rd,Wr,Wr,Wr,Wr
Value:3'b011-Not supported


`--interrupt`

Accelerator interrupt vector Number.


`--contmodetime`

 Continuous mode time in seconds.
 
 
 `--testall `

Run all host exerciser tests.


 `--clock-mhz`

pcie clock frequency, default value 350Mhz.



## EXAMPLES ##
This command exerciser Loopback afu:
```console
host_exerciser lpbk
```

This command exerciser memory afu:
```console
host_exerciser mem
```

This command exerciser Loopback afu on pcie 000:3b:00.0:
```console
host_exerciser --pci-address 000:3b:00.0    lpbk
```

This command exerciser Loopback afu on pcie 000:3b:00.0 and run in write mode:
```console
host_exerciser --pci-address 000:3b:00.0   --mode write lpbl
```

This command exerciser Loopback afu on pcie 000:3b:00.0 and run 2 cache lines per request:
```console
host_exerciser --pci-address 000:3b:00.0   --cls cl_2  lpbk
```

This command exerciser Loopback afu on pcie 000:3b:00.0 and run continuous mode for 10 seconds:
```console
host_exerciser --pci-address 000:3b:00.0   -cls cl_1   -m 0 --continuousmode true --contmodetime 10 lpbk
```

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | Updated description of the `fme` command | 
