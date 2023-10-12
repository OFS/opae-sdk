# CXL Host Exerciser #

## SYNOPSIS ##
```console
Usage: cxl_host_exerciser [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
  -l,--log-level TEXT:{trace, debug, info, warning, error, critical, off} [info]
                              stdout logging level
  -t,--timeout UINT [60000]   test timeout (msec)


Subcommands:
  cache                        run simple cxl he cache test
  lpbk                         run simple cxl he lpbk test

Usage subcommand cache: cxl_host_exerciser  cache

  -h,--help                   Print this help message and exit
  --test UINT:value in {fpgardcachehit->0,fpgardcachemiss->2,fpgawrcachehit->1,fpgawrcachemiss->3,
                       hostrdcachehit->4,hostrdcachemiss->6,hostwrcachehit->5,hostwrcachemiss->7} OR 
                       {0,2,1,3,4,6,5,7} [fpgardcachehit] host exerciser cache test
  --continuousmode BOOLEAN [false]
                              test rollover or test termination
  --contmodetime UINT [1]     Continuous mode time in seconds
  --target UINT:value in {fpga->1,host->0} OR {1,0} [host]
                              host exerciser run on host or fpga
  --bias UINT:value in {fpgamem_device_bias->3,fpgamem_host_bias->2,hostmem->0} OR {3,2,0} [hostmem]
                              host exerciser run on hostmem or fpgamem
  --device UINT:value in {/dev/dfl-cxl-cache.0->0,/dev/dfl-cxl-cache.1->1} OR {0,1} [/dev/dfl-cxl-cache.0]
                              run host exerciser device /dev/dfl-cxl-cache.0 (instance 0) 
                              or /dev/dfl-cxl-cache.1 (instance 1)
  --stride UINT:INT in [0 - 3] [0]
                              Set stride value
  --linerepcount UINT:INT in [1 - 256] [10]
                              Line repeat count
  --testall BOOLEAN [false]   Run all tests
```



## DESCRIPTION ##
The cxl host exerciser used to for generating traffic to create scenarios
like Cache Hit/Miss in Device or Host Caches with the intention of exercising
the path from AFU to the Host via CXL IP at full bandwidth.

### Performance and Latency measurement scenarios ###
CXL host exerciser measures performance and latency in below scenarios.

1. FPGA Read Cache Hit
2. FPGA Write Cache Hit
3. FPGA Read Cache Miss
4. FPGA Write Cache Miss
5. Host Read Cache Hit
6. Host Write Cache Hit
7. Host Read Cache Miss
8. Host Write Cache Miss


### CXL HOST EXERCISER SUB COMMANDS ###
`cache`

run clx host exerciser cache performance and latency test

`lpbk`

run cxl host exerciser lpbk test


## OPTIONAL ARGUMENTS ##
`--help, -h`

Prints help information and exit.


## COMMON ARGUMENTS / OPTIONS ##
The following arguments are common to all commands and are optional.

` -p,--pci-address`

PCIe domain, bus, device, function number of FPGA resource.

`-l,--log-level`

set host exerciser tool log level, trace, debug, info, warning, error, critical, off

`-t,--timeout`

host exerciser tool time out, by default time out 60000

## SUBCOMMAND CACHE ARGUMENTS / OPTIONS ##

`--help, -h`

Prints cache help information and exit.


`--test`

CXL scenarios type FPGA Read Cache Hit, FPGA Write Cache Hit, FPGA Read Cache Miss
FPGA write Cache Miss, Host  Read Cache Hit, Host  write Cache Hit, 
Host  Read Cache Miss, Host and  write Cache Miss


`--target`

CXL host exerciser allocates memory on numa node host or device FPGA.


`--device `

CXL host exerciser device instance  /dev/dfl-cxl-cache.0 or /dev/dfl-cxl-cache.1


`--bias `

CXL host exerciser bias mode 
1. Host BIAS mode targeting Host Address
2. Host BIAS mode targeting HDM(Device) Address
3. Device BIAS mode targeting HDM(Device) Address

`--stride`

Number of cache lines per request 1, 2, 3, 4.

`--linerepcount`

Number of times CXL host exerciser run test on cache line
 
`--continuousmode`

Configures test rollover or test termination mode.


`--contmodetime`

 Continuous mode time in seconds.
 
 
 `--testall `

Run all host exerciser tests.



## EXAMPLES ##

This command cxl host exerciser cache afu:
```console
cxl_host_exerciser cache
```

This command cxl host exerciser cache afu on pcie 000:38:00.0:
```console
cxl_host_exerciser --pci-address 000:38:00.0    cache
```

This command cxl host exerciser allocates memory on Host and runs 
FPGA Read Cache Hits scenario:
```console
cxl_host_exerciser cache --test fpgardcachehit --target host
```

This command cxl host exerciser allocates memory on FPGA and runs 
FPGA Read Cache Hits scenario :
```console
cxl_host_exerciser cache --test fpgardcachehit --target fpga
```

This command cxl host exerciser allocates memory on host, set bias mode to
Host BIAS mode targeting Host Address and run FPGA Read Cache Hits scenario :
```console
./bin/cxl_host_exerciser cache --test fpgardcachehit --target host --bias hostmem
```

This command cxl host exerciser allocates memory on FPGA, set bias mode to
Host BIAS mode targeting HDM(Device) Address and run FPGA Read Cache Hits scenario :
```console
./bin/cxl_host_exerciser cache --test fpgardcachehit --target fpga --bias fpgamem_host_bias
```

This command cxl host exerciser allocates memory on FPGA, set bias mode to
Device BIAS mode targeting HDM(Device) Address and run FPGA Read Cache Hits scenario :
```console
./bin/cxl_host_exerciser cache --test fpgardcachehit --target fpga --bias fpgamem_device_bias
```


This command cxl host exerciser allocates memory on Host, set stride to 3 and runs 
FPGA Read Cache Hits scenario:
```console
cxl_host_exerciser cache --test fpgardcachehit --target host --stride 3
```
