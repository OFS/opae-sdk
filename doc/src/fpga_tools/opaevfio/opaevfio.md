# opaevfio #

## SYNOPSIS ##

`opaevfio [-h] [-i] [-r] [-d DRIVER] [-u USER] [-g GROUP] [-n] [-v] [addr]`

## DESCRIPTION ##

The ```opaevfio``` command enables the binding/unbinding of a PCIe device
to/from the vfio-pci device driver. See https://kernel.org/doc/Documentation/vfio.txt
for a description of vfio-pci.

## OPTIONS ##

`addr`
    The PCIe address of the device in ssss:bb:dd.f format, eg 0000:7f:00.0

`-h, --help`

    Display command-line help and exit.

`-i, --init`

    Specifies binding mode operation - initialize the given addr for vfio.
    Used in conjunction with -u, -g, and -n.

`-r, --release`

    Specifies unbinding mode operation - release the given addr from vfio.
    Used in conjunction with -d.

`-d DRIVER, --driver DRIVER`

    Specifies the device driver to bind to when releasing from vfio.
    When omitted, the device is not rebound to a driver (default).

`-u USER, --user USER`

    The user ID to assign when binding to vfio. A new device node is created in
    /dev/vfio when the device is bound to vfio-pci. Use this option to specify
    the new device owner.

`-g GROUP, --group GROUP`

    The group ID to assign when binding to vfio. Use this option to specify the
    new device group for the device created in /dev/vfio.

`-n, --no-sriov`

    Do not enable SR-IOV when binding to vfio. The default value for this option
    is FALSE, ie the script should specify SR-IOV functionality when binding to
    the vfio-pci driver. When omitted, the modprobe command which loads the vfio-pci
    driver will contain the `enable_sriov=1` option. When given, it will not.

`-v, --version`

    Display script version information and exit.

## EXAMPLES ##

`opaevfio -h`<br>
`opaevfio -v`<br>
`sudo opaevfio -i -u lab -g labusers 0000:7f:00.0`<br>
`sudo opaevfio -r 0000:7f:00.0`

## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2021.01.07 | IOFS EA | Initial release.
