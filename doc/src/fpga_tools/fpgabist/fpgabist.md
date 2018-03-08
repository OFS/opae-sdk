# fpgabist #

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
   1. nlb_mode_3 = host memory interface checking
   2. dma = local memory interface checking

The platform includes the AF files, but you can also compile the AFs from the source. 

If there are multiple devices, use -b, -d, -f to specify the BDF for the specific device.

## POSITIONAL ARGUMENTS ##
`[path_to_gbs1 path_to_gbs2 ...]`

   Paths to GBS files of AFs being used.

### OPTIONAL ARGUMENTS ##
`-h, --help`

   Prints usage information

`-i device_id, --device-id device_id`

   Device ID for Intel FPGA. Deafult is: 0x09c4

`-b bus, --bus bus`

   Bus number for specific FPGA

`-d device, --device device`

   Device number for specific FPGA

`-f function, --function function`

   Function number for specific FPGA

## EXAMPLES ##

`fpgabist <path_to_gbs_files>/dma_afu.gbs <path_to_gbs_files>/nlb_3.gbs`

 Runs ```fpgabist``` on any platforms in the system that match the default device ID. This command runs both the DMA and NLB_MODE_3 tests.

   
