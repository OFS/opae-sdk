# pci_device #

## SYNOPSIS ##

`pci_device [-h] [-E] device {remove,rescan,topology,unbind}`

## DESCRIPTION ##

pci_device is a tool to aid in common operations for managing PCIe devices and
drivers.

## OPTIONS ##

### POSITIONAL ARGUMENTS ###
    `device`

    pcie address of device ([segment:]bus:device.function)

    `{remove,rescan,topology,unbind}`

    action to perform on device

    `remove`

    Remove the pci device from the pci bus

    `rescan`

    Rescan the bus as identified by the bus component of the pcie device address

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

    perform action on peer pcie devices

## EXAMPLES ##
    pci_device 0000:3d:00.0 remove
    pci_device 0000:3d:00.0 rescan
    pci_device 3d:00.0 topology

## Revision History ##
    Document Version | Intel Acceleration Stack Version | Changes
    -----------------|----------------------------------|--------
    2021.01.13 | IOFS EA | Initial release.
