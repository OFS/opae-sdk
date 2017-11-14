# userclk #

## SYNOPSIS  ##

`userclk [-B <bus>] [-D <device>] [-F <function>] [-S <socket>] [-P <Port id>] [-H <User clock high frequency>] -L <User clock low frequency>]`


## DESCRIPTION ##

userclk tool used to set high and low clock frequency to acceleration function unit (AFU).

## EXAMPLES  ##

`./userclk  -B 0x5e -H 400 -L 200`

 Sets AFU frequency.

## OPTIONS ##

`-B,--bus` FPGA Bus number.

`-D,--device` FPGA Device number.

`-F,--functio` FPGA function number.

`-S,--socket` FPGA socket number.

`-P,--port` Port id.

`-H,--freq-high ` User clock high frequency. 

`-L,--freq-low ` User clock low frequency. 



