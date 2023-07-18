# mem_tg

## SYNOPSIS ##
```console
Usage: mem_tg [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -g,--guid TEXT=4DADEA34-2C78-48CB-A3DC-5B831F5CECBB
                              GUID
  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off}=info
                              stdout logging level
  -s,--shared                 open in shared mode, default is off
  -t,--timeout UINT=60000     test timeout (msec)
  -m,--mem-channel UINT=0     Target memory bank for test to run on (0 indexed)
  --loops UINT=1              Number of read/write loops to be run
  -w,--writes UINT=1          Number of unique write transactions per loop
  -r,--reads UINT=1           Number of unique read transactions per loop
  -b,--bls UINT=1             Burst length of each request
  --stride UINT=1             Address stride for each sequential transaction
  --data UINT:value in {fixed->0,prbs15->2,prbs31->3,prbs7->1,rot1->3} OR {0,2,3,1,3}=fixed
                              Memory traffic data pattern: fixed, prbs7, prbs15, prbs31, rot1
  -f,--mem-frequency UINT=0   Memory traffic clock frequency in MHz

Subcommands:
  tg_test                     configure & run mem traffic generator test

```



## DESCRIPTION ##
The memory traffic generator (TG) used to exercise and test available memory channels
with a configurable traffic pattern.

Execution of this application requires the user to bind the specific VF endpoint containing the mem_tg AFU id to vfio-pci

In the TG, read responses are checked against a specified pattern. If the application is configured to perform a read only test on a region of memory that has not previously been initialized to contain that pattern it will flag a test failure.

## OPTIONAL ARGUMENTS ##
`--help, -h`

Prints help information and exit.


## COMMON ARGUMENTS / OPTIONS ##
The following arguments are common to all commands and are optional.

` -p,--pci-address`

PCIe domain, bus, device, function number of fpga resource.

`-l,--log-level`

set application log level, trace, debug, info, warning, error, critical, off

`-s,--shared `

open FPGA PCIe resource in shared mode

`-t,--timeout`

mem_tg application time out, by default time out 60000

`-m,--mem-channel`

Target memory banks for test to run on (0 indexed). Multiple banks seperated by ', '. '-m all' will use every channel enumerated in MEM_TG_CTRL
Example: to run on channels 1 and 2:            -m 0, 1
Example: to run on all available channels:      -m all

default: 0

`--loops`

Number of read/write loops to be run 
default: 1

`-w,--writes`

Number of unique write transactions per loop. 
default: 1

`-r,--reads`

Number of unique read transactions per loop 
default: 1

`-b,--bls`

AXI4 burst length of each request.  Supports 1-256 transfers beginning from 0.
default: 0


`--stride`

Address stride for each sequential transaction (>= burst length) 
default: 1


`--data`

Memory traffic data pattern.
0 = fixed {0xFF, 0x00}
1 = prbs7
2 = prbs15
3 = prbs31
4 = rot1

default: fixed

`-f, --mem-frequency`

Memory traffic clock frequency in MHz 
default: 300 MHz

## EXAMPLES ##
This command will run a basic read/write test on the channel 0 traffic generator:
```console
mem_tg tg_test
```

This command will run the application for an afu on pcie 000:b1:00.7:
```console
mem_tg --pci-address 000:b1:00.7 tg_test
```

This command will test channel 2 write bandwidth:
```console
mem_tg -loops 1000 -w 1000 -r 0 -m 2 tg_test
```

This command will perform a read bandwidth test with a burst of 16 on channel 1 and perform a data comparison with the prbs7 pattern:
```console
mem_tg -loops 1000 -w 0 -r 1000 -b 0xF --data prbs7 -m 1 tg_test
```

This command will perform a read/write test with 1 MB strided access to channel 0 memory:
```console
mem_tg -loops 10000 --stride 0x100000 tg_test
```


## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2022.07.21 | 2.0.11  <br>(Supported with Intel Quartus Prime Pro Edition 22.1.) | Added description of the `mem_tg` command |