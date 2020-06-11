## NAME ##
fpgabist - FPGA built-in self test

## SYNOPSIS ##
```console
fpgabist [-h] [-i device_id] [-b bus] [-d device] [-f function] [path_to_gbs1 path_to_gbs2 ...]
```

## DESCRIPTION ##
The ```fpgabist``` tool performs self-diagnostic tests on supported FPGA platforms.

The tool accepts one or more Accelerator Function (AF) binaries from a predetermined set of AFs. Depending on the available binaries, 
the tool runs appropriate tests and reports hardware issues.

```fpgabist``` always uses ```fpgainfo``` to report system information before running any hardware tests.

Currently, ```fpgabist``` accepts the following AFs:
   1. nlb_mode_3: The native loopback (NLB) test implements a loopback from TX to RX. Use it to verify basic functionality
   and to measure bandwidth.
   2. dma_afu: The direct memory access (DMA) AFU test transfers data from host memory to FPGA-attached local memory. 

The installation includes the AF files, but you can also compile the AFs from the source. 

If there are multiple PCIe&reg; devices, use -b, -d, -f to specify the BDF for the specific PCIe&reg; device.

## POSITIONAL ARGUMENTS ##
`[path_to_gbs1 path_to_gbs2 ...]`

   Paths to Accelerator Function (AF) files.

### OPTIONAL ARGUMENTS ##

You can use the single letter or the full parameter name for the command line arguments.

`-h, --help`

   Prints usage information

`-i device_id, --device-id device_id`

   Device ID for Intel FPGA. Default is: 0x09c4

`-B bus, --bus bus`

   Bus number for specific FPGA

`-D device, --device device`

   Device number for specific FPGA

`-F function, --function function`

   Function number for specific FPGA

## EXAMPLES ##

`fpgabist <path_to_gbs_files>/dma_afu.gbs <path_to_gbs_files>/nlb_3.gbs`

 Runs ```fpgabist``` on any platform in the system that matches the default device ID. This command runs both the DMA and 
 NLB_MODE_3 tests.
 
 `fpgabist -i 09c4 -b 5 <path to gbs>/dma_afu.gbs`
 
 Runs `fpgabist` the DMA test on the PCIe&reg;  Endpoint with `device_id` 09c4 on bus 5. 

## Revision History ##

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) | Made the following changes: <br>Expanded descriptions of `nlb_mode_3` and`dma_afu` tests. <br> Added a second example command. |


