# alaska_fw_loader #

## NAME ##
_alaska_fw_loader_ - Alaska firmware images loader

## SYNOPOSIS ##

Application allows to load firmware for the device from embedded ROM or EEPROM.

```console
alaska_fw_loader --pci_device resource_path --sbusmaster rom_path --serdes rom_path
```

```eval_rst
.. important:: Tool requires `Alaska_v2` GBS to be loaded.
```

```eval_rst
.. note:: See Marvell "88X5111 Relase Notes - A1" document for details on rom file selection.
```

## COMMON OPTIONS ##
`--help`

    Print help information and exit.

--load method (default: rom)

    Supported load methods are:

    - rom    - load firmware from ROM to RAM
    - eeprom - update and load firmware from EEPROM to RAM

```eval_rst
.. note:: The firmware images have to be reloaded after every device power cycle or hardware reset, an alternative approach is to load the firmware images from an EEPROM.
```

`--pci_device resource_path`

    `path`: path to resource0 of fpga device eg. `/sys/devices/pci0000:5e/0000:5e:00.0/resource0`

    Use `lspci` to find path to the PCIe device.

```console
$ lspci -d 8086:bcc0
5e:00.0 Processing accelerators: Intel Corporation Device bcc0
be:00.0 Processing accelerators: Intel Corporation Device bcc0
$ readlink -e $(lspci -nd 8086:bcc0 | awk '{print "/sys/bus/pci/devices/*"$1"/resource0"}')
/sys/devices/pci0000:5e/0000:5e:00.0/resource0
/sys/devices/pci0000:be/0000:be:00.0/resource0
```

## ALASKA_FW_LOADER OPTIONS ##

`--sbusmaster rom_path`

    `rom_path`: path to sbusmaster rom

`--serdes rom_path`

    `rom_path`: path to serdes rom

## USAGE EXAMPLE ##

```console
# ./alaska_fw_loader --pci_device /sys/devices/pci0000:5e/0000:5e:00.0/resource0 --sbusmaster ./alaska/sbusmaster.rom --serdes ./alaska/serdes.rom --load eeprom
```
