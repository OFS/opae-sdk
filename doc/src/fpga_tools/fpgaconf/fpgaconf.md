# fpgaconf #

## SYNOPSIS ##

`fpgaconf [-hvn] [-b <bus>] [-d <device>] [-f <function>] [-s <socket>] <gbs>`

## DESCRIPTION ##

fpgaconf writes accelerator configuration bitstreams (also referred to as "green
bitstreams" to an FPGA device recognized by OPAE. In the process, it also checks
the green bitstream file for compatibility with the targeted FPGA and its
current infrastructure bitstream (the "blue bistream"). fpgaconf takes the
following arguments:

`-h, --help`

	Print usage information

`-v, --verbose`

	Print more verbose messages while enumerating and configuring. Can be
	given more than once

`-n, --dry-run`

	Perform enumeration, but skip any operations with side-effects (like the
	actual configuration of the bitstream

`-b, --bus`

	PCI bus number of the FPGA to target

`-d, --device`

	PCI device number of the FPGA to target

`-f, --function`

	PCI function number of the FPGA to target

`-s, --socket`

	Socket number of the FPGA to target

fpgaconf will enumerate available FPGA devices in the system and select
compatible FPGAs for configuration. If there are more than one candidate FPGAs
that are compatible with the given green bitstream, fpgaconf will exit and ask
you to be more specific in selecting the target FPGAs (e.g. by specifying a
socket number, or a PCIe bus/device/function).

## EXAMPLES ##

`fpgaconf my_green_bitstream.gbs`

	Program "my_green_bitstream.gbs" to a compatible FPGA

`fpgaconf -v -s 0 my_green_bitstream.gbs`

	Program "my_green_bitstream.gbs" to the FPGA in socket 0, if compatible,
	while printing out slightly more verbose information
