# userclk #

## SYNOPSIS  ##

`userclk [-hv] [-S <segment>] [-B <bus>] [-D <device>] [-F <function>] [PCI_ADDR] [-H <User clock high frequency>] -L <User clock low frequency>]`


## DESCRIPTION ##

userclk sets the frequency range for an AFU. 

## EXAMPLES  ##

`./userclk -B 0x5e -H 400 -L 200`

 Sets AFU frequency.

## OPTIONS ##

`-v,--version`

Prints version information and exits.

`-S,--segment` 

FPGA segment number.

`-B,--bus` 

FPGA Bus number.

`-D,--device` 

FPGA Device number.

`-F,--function` 

FPGA function number.

`-H,--freq-high ` 

User clock high frequency. 

`-L,--freq-low ` 

User clock low frequency. 

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) |  Fixed typos. |

