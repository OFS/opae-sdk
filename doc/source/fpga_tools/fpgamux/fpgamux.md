# fpgamux #

## SYNOPSIS ##
```console
fpgamux [-h] [-S|--socket-id SOCKET_ID] [-B|--bus-number BUS] [-D|--device DEVICE] [-F|--function FUNCTION]
        [-G|--guid GUID] -m|--muxfile <filepath.json>
```

## DESCRIPTION ##
fpgamux is a testing tool to interact with multiple AFUs that have been synthesized into one GBS along with
the CCIP-MUX BBB (basic building block). The CCIP-MUX uses upper bits in the MMIO addresses to route MMIO
reads/writes to the AFU running on the corresponding CCIP-MUX port. fpgamux uses a configuration file that
lists the software components and configuration to use.

.. note::

```
  Only one (the first) AFU is discoverable by the OPAE driver. Enumerating AFCs on an FPGA will find
  the AFC associated with the first AFU only. The first software component in the configuration will
  be used to determine the GUID to use for enumeration. This can be overridden with the -G|--guid option.
```


## OPTIONS ##
`-S SOCKET_ID, --socket-id SOCKET_ID`
   socket id of FPGA resource

`-B BUS, --bus BUS`
   bus id of FPGA resource

`-D DEVICE, --device DEVICE`
   device id of FPGA resource

`-F FUNCTION, --function FUNCTION`
   function id of FPGA resource

`-G, --guid`
   specify what guid to use for the AFC enumeration

`-m, --muxfile <filepath.json>`
    The path to the fpgamux configuration file. This file must be in JSON format following the
    schema described below

## CONFIGURATION ##
fpgamux uses a configuration file (in JSON format) to determine what software components to instantiate and
how to configure them for interacting with the AFUs in the GBS. This schema for this is listed below:

```
    [
        {
            "app" : "fpga_app",
            "name" : "String",
            "config" : "Object"
        }
    ]
```

## EXAMPLES ##
An example configuration with two components is listed below:
```
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
```
