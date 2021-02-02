# pci_device #

## SYNOPSIS ##

`pci_device [-h] [-E] device {remove,rescan,topology,unbind}`

## DESCRIPTION ##

pci_device is a tool to aid in common operations for managing PCIe devices and
drivers.

## OPTIONS ##

### POSITIONAL ARGUMENTS ###
    `device filter`

    PCIe address of a device or vendor/device ID pair.
    The PCIe address follows the format of [segment:]bus:device.function
    while the vendor/device ID pair follows the format [vendor ID]:[device ID]
    where at least one of these must be present.

    `{aer,remove,rescan,topology,unbind}`

    action to perform on device

    `aer`
    Perform AER (Advanced Error Reporting) operations.
    The aer action has its own sub-commands which are listed below:

    * `dump` sub-command will print out the AER error counters as reported
       by the sysfs files for the device.
    * `mask` can either print out the current AER mask bits or set them
      * If `show` or `print` (or nothing) is given after the `mask`
        command, it will show the current mask bits for AER.
	By default output will be written in stdout but can be written to an
	output file if `-o|--output FILENAME` argument is given.
      * If `all` is given after the `mask` command, it will mask all bits
        (by setting the values to 0xffffffff and 0xffffffff).
      * If `off` is given after the `mask` command, it will unmask all
        bits (by setting the values to 0x0 and 0x0).
      * If two numbers are present after the `mask` command, those two
        numbers will be used to set the mask bits.
	Values for setting the mask can also be read in from an input file if
	`-i|--input FILENAME` argument is given.

    _NOTE_: mask related operations require root privileges


    `remove`

    Remove the pci device from the pci bus

    `rescan`

    Rescan the bus as identified by the bus component of the PCIe device address

    'topology`

    Print the PCIe topology from the root port to the PCIe device.
    This shows the PCIe tree rooted at the PCIe root port.
    Each line shows the the PCIe address, vendor ID, and device ID along with
    the driver bound to the device. The indentnation is used to show
    parent/child relationship of devices.

    The line listing the target PCIe device as identified by the given PCIe
    address will be highlighted in green while the endpoints will be
    highlighted in cyan.

    The example below shows the topology of an N3000 device with eight virtual
    functions created from one of the Ethernet controllers:

    ```console
    [pci_address(0000:3a:00.0), pci_id(0x8086, 0x2030)] (pcieport)
	    [pci_address(0000:3b:00.0), pci_id(0x10b5, 0x8747)] (pcieport)
	        [pci_address(0000:3c:09.0), pci_id(0x10b5, 0x8747)] (pcieport)
	            [pci_address(0000:3f:00.0), pci_id(0x8086, 0x0b30)] (dfl-pci)
	        [pci_address(0000:3c:11.0), pci_id(0x10b5, 0x8747)] (pcieport)
	            [pci_address(0000:43:00.0), pci_id(0x8086, 0x0b32)] (no driver)
		[pci_address(0000:3c:08.0), pci_id(0x10b5, 0x8747)] (pcieport)
	            [pci_address(0000:3d:02.0), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:00.1), pci_id(0x8086, 0x0d58)] (i40e)
	            [pci_address(0000:3d:02.7), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:02.5), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:02.3), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:02.1), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:00.0), pci_id(0x8086, 0x0d58)] (i40e)
	            [pci_address(0000:3d:02.6), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:02.4), pci_id(0x8086, 0x154c)] (iavf)
	            [pci_address(0000:3d:02.2), pci_id(0x8086, 0x154c)] (iavf)
	        [pci_address(0000:3c:10.0), pci_id(0x10b5, 0x8747)] (pcieport)
	            [pci_address(0000:41:00.0), pci_id(0x8086, 0x0d58)] (i40e)
	            [pci_address(0000:41:00.1), pci_id(0x8086, 0x0d58)] (i40e)

    ```

    `unbind`

    Unbind the driver bound to the device



### OPTIONAL ARGUMENTS ###
    `-h, --help`

    show this help message and exit

    `-E, --other-endpoints`

    perform action on peer PCIe devices

## EXAMPLES ##
    pci_device 0000:3d:00.0 remove
    pci_device 0000:3d:00.0 rescan
    pci_device 3d:00.0 topology
    pci_device :0b30 topology
    pci_device :0b30 aer
    pci_device :0b30 aer mask
    pci_device :0b30 aer mask all
    pci_device :0b30 aer mask -o mask.dat
    pci_device :0b30 aer mask -i mask.dat


## Revision History ##
    Document Version | Intel Acceleration Stack Version | Changes
    -----------------|----------------------------------|--------
    2021.01.13 | IOFS EA | Initial release.
