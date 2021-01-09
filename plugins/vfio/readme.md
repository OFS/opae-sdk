# VFIO Plugin

The OPAE vfio plugin, v-opae, is an OPAE plugin designed to discover and
interface with accelerator resources on PCIe devices bound to the vfio-pci
kernel driver. Given that the OPAE object model currently differentiates
between management (`FPGA_DEVICE`) and accelerator (`FPGA_ACCELERATOR`),
the vfio plugin focuses on enabling developers to create user-mode drivers
for accelerators using the OPAE API. This means that even when a PCIe device
with management IP may be bound to the vfio-pci driver, the OPAE operations
allowed for those resources are limited mostly to those relating to discovery
and identification.

### Sample Use Case
As mentioned before, this plugin is best used for runninng user-space
drivers with vfio-pci kernel driver. An example hardware configuration
could expose managment functions via a PF (physical fucntion) endpoint
and one or more accelerator functions via VF (virtual function) endpoints.
The image below depicts such an example and shows accelerator data flow
between a software application and the accelerator functional unit (AFU).

![sample use case](vfio-plugin-example.svg)

#### Binding vfio-pci Driver
Using the example above, the folowing shows how one may bind the vfio-pci
driver to the VF for the AFU. For this example, the PCIe vendor ID is 8086
and the device IDs for the PF and VF are aba0 and aba1 respectively.

Load the vfio-pci driver and create one VF.
```shell
> sudo modprobe vfio-pci
> echo 1 | sudo tee /sys/bus/pci/devices/0000\:ab\:00.0/sriov_numvfs
```

At this point, if this is the first VF created for the device, a
sysfs node will be created for using function 1. The new PCIe
address will be `0000:ab:00.1`. If a driver claims this then it
must be unbound from the device.
```shell
> echo 0000:ab:00.1 | sudo tee /sys/bus/pci/drivers/<vf-driver>/unbind
```

Write the vendor and device id (in hex) to the new_id sysfs attribute
for the vfio-pci driver.
```shell
> echo 8086 a0a1 > /sys/bus/pci/drivers/vfio-pci/new_id
```

More information about vfio can be found
[here](https://www.kernel.org/doc/Documentation/vfio.txt).

#### Enabling the Plugin
Currently, the vfio plugin will not automatically load. One must use the OPAE
configuration file in order to enable it. The example below shows how to
enable it by adding a configuration item into the configurations dictionary
(as identified by the `vfio` name) and by adding the idendtifier to the list
of plugins.

```json
{
    "configurations": {
        "vfio": {
            "configuration": {
            },
        "enabled": true,
        "plugin": "libv-opae.so"
        }
    },
    "plugins": [
        "vfio"
    ]
}
```

### OPAE Operations
