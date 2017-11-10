# coreidle #

## SYNOPSIS  ##

`coreidle [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-G <GBS path>] `


## DESCRIPTION ##

This tools parses input GBS, extracts power from metadata ,calculates fpga power, number of online and idle cores.
It moves threads from idle cores to online cores.

## EXAMPLES  ##

`./coreidle  -B 0x5e -G /home/lab/gbs/mode0.gbs`

  Idle cores to run online cores at maximum capacity.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-G,--gbs` Green bitstream file path.
 


