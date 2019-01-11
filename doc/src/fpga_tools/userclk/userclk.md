# userclk #

## SYNOPSIS  ##

`userclk [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <Port id>] [-H <User clock high frequency>] -L <User clock low frequency>]`


## DESCRIPTION ##

userclk sets the frequency range for an AFU. 

## EXAMPLES  ##

`./userclk  -B 0x5e -H 400 -L 200`

 Sets AFU frequency.

## OPTIONS ##

`-B,--bus` 

FPGA Bus number.

`-D,--device` 

FPGA Device number.

`-F,--function` 

FPGA function number.

`-S,--socket` 

FPGA socket number.

`-P,--port` 

Port ID.

`-H,--freq-high ` 

User clock high frequency. 

`-L,--freq-low ` 

User clock low frequency. 

| Date | Intel Acceleration Stack Version | Changes Made |
|:------|----------------------------|:--------------|
|2018.05.21| DCP 1.1 Beta (works with Quartus Prime Pro 17.1.1) |  Fixed typos. |

