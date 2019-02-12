# fpgamux #

## SYNOPSIS ##
```console
fpgamux [-h] [-S|--socket-id SOCKET_ID] [-B|--bus-number BUS] [-D|--device DEVICE] [-F|--function FUNCTION]
        [-G|--guid GUID] -m|--muxfile <filepath.json>
```

## DESCRIPTION ##
```fpgamux``` tests multiple AFUs that are synthesized into a single AFU along with
the CCIP-MUX basic building block (BBB). The CCIP-MUX uses the upper bits in the MMIO addresses to route MMIO
reads and writes to the AFU running on the corresponding CCIP-MUX port. ```fpgamux``` uses a configuration file that
lists the software components and correct configuration. ```fpgamux``` only runs on the Integrated FPGA Platform. 
You cannot run it on the PCIe accelerator card (PAC).

.. note::

```
  The OPAE driver discovers only the first AFU. The first software component in the configuration 
  determines the GUID to use for enumeration. Use the -G|--guid option to override the GUID
  for the first software component.
```


## OPTIONS ##
`-S SOCKET_ID, --socket-id SOCKET_ID`

   socket id of FPGA resource.

`-B BUS, --bus BUS`

   bus id of FPGA resource.

`-D DEVICE, --device DEVICE`

   The device id of FPGA resource.

`-F FUNCTION, --function FUNCTION`

   The function id of FPGA resource.

`-G, --guid`

   Specifies the GUID to use for the AFC enumeration.

`-m, --muxfile <filepath.json>`

The path to the ```fpgamux``` configuration file. This file must be in JSON format following the
schema described below.

## CONFIGURATION ##
```fpgamux``` uses a configuration file (in JSON format) to determine what software components to instantiate and
how to configure them to work with the AFUs. The schema includes the following elements:

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
The following example shows a configuration with two components:
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

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 | 2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.) | No changes from previous release.  | 
