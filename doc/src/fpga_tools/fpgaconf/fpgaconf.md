# fpgaconf #

## SYNOPSIS ##

`fpgaconf [-hvVn] [-b <bus>] [-d <device>] [-f <function>] [-s <socket>] <gbs>`

## DESCRIPTION ##

```fpgaconf``` configures the FPGA with the accelerator function (AF). It also checks the AF for compatibility with 
the targeted FPGA and the FPGA Interface Manager (FIM). ```fpgaconf``` takes the following arguments: 

`-h, --help`

	Prints usage information.

`-v, --version`

	Prints version information and exits.

`-V, --verbose`

	Prints more verbose messages while enumerating and configuring. Can be
	requested more than once.

`-n, --dry-run`

	Performs enumeration. Skips any operations with side-effects such as the
	actual AF configuration. 

`-B, --bus`

	PCIe bus number of the target FPGA.

`-D, --device`

	PCIe device number of the target FPGA. 

`-F, --function`

	PCIe function number of the target FPGA.

`-S, --socket`

	Socket number of the target FPGA.

```fpgaconf``` enumerates available FPGA devices in the system and selects
compatible FPGAs for configuration. If more than one FPGA is
compatible with the AF, ```fpgaconf``` exits and asks you to be
more specific in selecting the target FPGAs by specifying a
socket number or a PCIe BDF.

## EXAMPLES ##

`fpgaconf my_af.gbs`

	Program "my_af.gbs" to a compatible FPGA.

`fpgaconf -v -s 0 my_af.gbs`

	Program "my_af.gbs" to the FPGA in socket 0, if compatible,
	while printing out slightly more verbose information.
	
	## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1.) | Corrected typos. |
