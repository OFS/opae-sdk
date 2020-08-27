# fecmode (N3000 specific tool) #

## SYNOPSIS ##
```console
fecmode [<mode>][<args>]
```

## DESCRIPTION ##

Fecmode changes FEC mode of external ethernet PHY, this tool only support on N3000 Card.

## POSITIONAL ARGUMENTS ##
`--segment, -S`

 segment number of the PCIe device.
   
 `--bus, -B` 
 
 bus number of the PCIe device. 
   
 `--device, -D` 
 
device number of the PCIe device.

`--function, -F`
function number of the PCIe device

`--rsu, -r`
reboot card only if mode is not configured

`--debug, -d`
output debug information 



## FEC Mode ##
`no`
no FEC.

`kr`
BaseR FEC (Fire-Code) correction – 4 orders

`rs`
Reed-Solomon FEC correction – 7 orders

## EXAMPLE ##

This command change FEC mode to “kr”:
```console
# fecmode -B 0x25 kr
```

This command reboot card (no need to specify bus number if there is only one card):
```console
# fecmode -r
```

This command display the current FEC mode:
```console
# fecmode
```

