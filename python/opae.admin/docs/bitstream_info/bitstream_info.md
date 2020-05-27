## NAME ##
bitstreaminfo - PAC bitstream information

## SYNOPSIS ##
`bitstreaminfo [--verbose | -v] file {file}`

## DESCRIPTION ##
The ```bitstreaminfo``` command displays authentication information contained within each
provided ```file``` on the command line. This includes any JSON header strings, authentication
header block information, and a small portion of the payload.

`--verbose`

Print verbose (debugging) messages along with bitstream security content.

`file {file}`

A list of one or more bitstream files which have been passed through the ```PACSign``` command and contain
authentication headers.

## TROUBLESHOOTING ##

To gather more debug output, use the `--verbose` option. 

## EXAMPLES ##

`bitstreaminfo firmware.bin`<br>
`bitstreaminfo -v firmware.bin`<br>

## Revision History ##

 | Document Version |  Intel Acceleration Stack Version  | Changes  |
 | ---------------- |------------------------------------|----------|
 |2019.12.20 | 1.2.1 Beta | Initial release. |
