# fpgasupdate #

## SYNOPSIS ##
`fpgasupdate [--log-level=<level>] file [bdf]`

## DESCRIPTION ##
```fpgasupdate``` updates the various firmware files for the PCIe&reg; Accelerator Card (PAC), in a secure fashion.

`--log-level <level>`

Sets the log level (the level of screen output) to `<level>`. The accepted levels
are `state`, `ioctl`, `debug`, `info`, `warning`, `error`, `critical`. If omitted, the default
level is `state`.

`file`

The secure update file to be programmed to the PAC. This may be the file to program a
static region (SR), programmable region (PR), root key hash, key cancellation, or other
device-specific firmware.

`bdf`

The PCIe address of the PAC to program. `bdf` is of the form `[ssss:]bb:dd:f`, corresponding
to PCIe segment, bus, device, function. The segment is optional; if omitted, a segment
of `0000` is assumed. If there is only one PAC in the system, then `bdf` may be omitted. In this
case, `fpgasupdate` will determine the address automatically.

## TROUBLESHOOTING ##

To gather more debug output, decrease the `--log-level` parameter. ie, `--log-level=state` provides
the most verbose output, followed by `--log-level=ioctl`, etc. The default level is `state`.

## EXAMPLES ##

`fpgasupdate firmware.bin`<br>
`fpgasupdate firmware.bin 05:00.0`<br>
`fpgasupdate firmware.bin 0001:04:02.0 --log-level=ioctl`

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2019.07.26 | ??? | Initial version. |
