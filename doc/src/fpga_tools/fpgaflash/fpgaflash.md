# fpgaflash #

## SYNOPSIS ##
```console
fpgaflash [-h] {user, factory, bmc_bl, bmc_app} file [bdf]
```

## DESCRIPTION ##
`fpgaflash` updates the various flash devices on an Intel Programmable Acceleration Card (PAC).

If there are multiple devices in the system, `fpgaflash` must specify a bus, device, and function (BDF) to select the correct device.
If you do not specify a BDF, `fpgaflash` prints out the BDFs of any compatible devices.

## POSITIONAL ARGUMENTS ##
There are three positional arguments: flash type, the flash image file, and the BDF. 

Flash type: `user, factory, bmc_bl, or bmc_app`

The first positional argument specifies the type of flash programming. The following table defines the four flash types.

| Flash Type | Description|
|---------------- |------------------------------------|
|`user` | Only reprograms the user FPGA image in flash memory. |
|`factory` |  Reprograms the entire FPGA flash. A catastrophic failure during a factory update such as a power outage requires a USB cable and `quartus_pgm` to recover. |
|`bmc_bl` | Only reprograms the Board Management Controller (BMC) bootloader image. |
|`bmc_app` | Only reprograms the (BMC) application firmware image. |
 

`file`

Specifies the Raw Programming Data (`.rpd`) file to program into the FPGA flash or the Hexadecimal (Intel-Format) File (`.hex`) file for the BMC bootloader or application.

`bdf`

Specifies the BDF of device to program such as 04:00.0 or 0000:04:00.0. This flag
is optional when there is a single device in the system.


## OPTIONAL ARGUMENTS ##
`-h, --help`

   Print usage information.

## EXAMPLE ##

`fpgaflash user new_image.rpd 0000:04:00.0`

Programs new_image.rpd to flash of device with BDF 0000:04:00.0.

  ## Revision History ##
                                
 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2018.10.02 | 1.2 Alpha. <br> (Supported with Intel Quartus Prime Pro Edition 17.1.1.) | Added  `bmc_bl` and `bmc_app` to supported flash types. |
 |2018.05.21 | 1.1 Beta. <br>(Supported with Intel Quartus Prime Pro Edition 17.1.1.) | No changes from previous release. |
