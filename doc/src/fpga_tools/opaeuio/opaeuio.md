# opaeuio #

## SYNOPSIS ##

`opaeuio [-h] [-i] [-r] [-d DRIVER] [-u USER] [-g GROUP] [-v] [device]`

## DESCRIPTION ##

The ```opaeuio``` command enables the binding/unbinding of a DFL device
to/from the dfl-uio-pdev device driver.
See https://kernel.org/doc/html/v4.14/driver-api/uio-howto.html for a
description of uio.

## OPTIONS ##

`device`
    The DFL device name, eg dfl_dev.10

`-h, --help`

    Display command-line help and exit.

`-i, --init`

    Specifies binding mode operation - initialize the given device for uio.
    Used in conjunction with -u, -g, and -d.

`-r, --release`

    Specifies unbinding mode operation - release the given device from uio.

`-d DRIVER, --driver DRIVER`

    Specifies the device driver to bind to when binding to uio.
    The default value is dfl-uio-pdev.

`-u USER, --user USER`

    The user ID to assign when binding to uio. A new device node is created in
    /dev when the device is bound to uio. Use this option to specify
    the new device owner.

`-g GROUP, --group GROUP`

    The group ID to assign when binding to uio. Use this option to specify the
    new device group for the device created in /dev.

`-v, --version`

    Display script version information and exit.

## EXAMPLES ##

`opaeuio -h`<br>
`opaeuio -v`<br>
`sudo opaeuio -i -u lab -g labusers dfl_dev.10`<br>
`sudo opaeuio -r dfl_dev.10`

## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2021.01.07 | IOFS EA | Initial release.
