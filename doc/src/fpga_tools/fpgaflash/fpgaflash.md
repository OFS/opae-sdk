# fpgaflash #

## SYNOPSIS ##
```console
fpgaflash [-h] {user,factory} file [bdf]
```

## DESCRIPTION ##
fpgaflash updates the static FIM image loaded from flash at power-on.

If there are multiple devices in the system, fpgaflash must specify a BDF to select the correct device. If no BDF is specified, fpgaflash prints out the BDFs of any compatible devices.

## POSITIONAL ARGUMENTS ##
`{user, factory}`

   type of flash programming
   
   A user update only reprograms the user image in flash.  
   
   A factory update reprograms the entire flash.  A catastrophic failure during a factory update such as a power outage requires a USB cable and quartus_pgm to recover.

`file`

   rpd file to program into flash.

`bdf`

   BDF of device to program such as 04:00.0 or 0000:04:00.0. This flag is optional when there is a single device in the system.


## OPTIONAL ARGUMENTS ##
`-h, --help`

   Print usage information.

## EXAMPLE ##

`fpgaflash user new_image.rpd 0000:04:00.0`

   Programs new_image.rpd to flash of device with BDF 0000:04:00.0.
