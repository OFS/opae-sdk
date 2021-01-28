# opae.io #

## SYNOPSIS ##

`opae.io ls [-v,--viddid VID:DID]`<br>
`opae.io init [-d PCI_ADDR USER[:GROUP]]`<br>
`opae.io release [-d PCI_ADDR]`<br>
`opae.io [-d PCI_ADDR] [-r REGION] walk OFFSET [-u,--show-uuid]`<br>
`opae.io [-d PCI_ADDR] [-r REGION] peek OFFSET`<br>
`opae.io [-d PCI_ADDR] [-r REGION] poke OFFSET VALUE`<br>
`opae.io [-d PCI_ADDR] [-r REGION] SCRIPT ARG1 ARG2 ... ARGN`<br>
`opae.io [-d PCI_ADDR] [-r REGION]`

## DESCRIPTION ##

```opae.io``` is an interactive Python environment packaged on top of
```libopaevfio.so```, which provides user space access to PCIe devices
via the vfio-pci driver. The main feature of opae.io is its built-in
Python command interpreter, along with some Python bindings that provide
a means to access Configuration and Status Registers (CSRs) that reside
on the PCIe device.

```opae.io``` has two operating modes: command line mode and interactive
mode.

## COMMAND LINE MODE ##

To view the accelerator devices that are present on the system, ```opae.io```
provides the ```ls``` command option.

`opae.io ls [-v,--viddid VID:DID]`

Each accelerator device is listed along with the PCIe address, the
PCIe vendor/device ID, a brief description of the device, and the
driver to which the device is currently bound.

```opae.io``` provide an option to initialize a PCIe device for use with
the vfio-pci driver. In order for the device CSRs to be accessed from
user space, the device must first be bound to the vfio-pci driver. This
is the job of the ```init``` command option.

`opae.io init [-d PCI_ADDR USER:[GROUP]]`

The ```init``` command unbinds the specified device from its current
driver and binds it to vfio-pci. This creates a new vfio group under
/dev/vfio. This group path is then used by the ```libopaevfio.so```
library to interact with the device.

To release the PCIe device from vfio-pci and return it to use with its
previous driver, the ```release``` command option is used.

`opae.io release [-d PCI_ADDR]`

The ```release``` command option reverses the actions of the last
```init``` command, releasing the device from vfio-pci and binding
it to the driver which was bound at the time the ```init``` command
was issued.

The ```walk``` command option traverses and displays the Device
Feature List of the given region.

`opae.io walk [-d PCI_ADDR] [-r REGION] [OFFSET] [-u,--show-uuid]`

The various fields of each Device Feature Header are displayed. The
`--show-uuid` option additionally displays the GUID for each feature.
OFFSET can be used to specify the beginning of the DFL in the MMIO
region.

The ```peek``` command option reads and displays a CSR value.

`opae.io peek [-d PCI_ADDR] [-r REGION] OFFSET`

The ```poke``` command option writes a given value to a CSR.

`opae.io poke [-d PCI_ADDR] [-r REGION] OFFSET VALUE`

```opae.io``` can also execute Python scripts from the command line.
These Python scripts may contain calls to the device built-in
functions that are available during an interactive session. Refer
to the description of interactive mode for details.

`opae.io [-d PCI_ADDR] [-r REGION] myscript.py a b c`

In order to enter the interactive mode of ```opae.io```, simply
invoke it and optionally pass the desired device address and
MMIO region options.

`opae.io [-d PCI_ADDR] [-r REGION]`

## INTERACTIVE MODE ##

Upon entering interactive mode, ```opae.io``` begins a Python
interpreter session and displays the command prompt shown below:

0000:3f:00.0[0]>>

The first portion of the prompt shows the address of the active
PCIe device, here 0000:3f:00.0. The part in square brackets shows
the active MMIO region, here [0].

The interpreter waits for a valid Python command, then attempts
to execute the given command in the usual way. The only
differences between the traditional Python command intepreter and
```opae.io``` are that opae.io provides 1) the notion of an active
PCIe device and MMIO region and 2) several built-in functions and
objects that allow manipulating the active device and MMIO region.

### BUILT-IN FUNCTIONS ###

The ```opae.io``` built-in functions assume an active device and
MMIO region. Attempting to use the built-in functions without first
opening a device and region will result in errors.

`peek(OFFSET)`

The `peek` built-in function reads and displays a CSR value from
the active device and region, at the offset supplied by its argument.

0000:3f:00.0[0]>> peek(0x28)<br>
0xdeadbeef

`poke(OFFSET, VALUE)`

The `poke` built-in function writes the given VALUE to the current
MMIO region at the given OFFSET.

0000:3f:00.0[0]>> poke(0x28, 0xdeadbeef)

`read_csr(OFFSET)`

The `read_csr` built-in function returns the value of the CSR at
the active MMIO region and the given OFFSET.

0000:3f:00.0[0]>> print('0x{:0x}'.format(read_csr(0x28)))<br>
0xdeadbeef

`write_csr(OFFSET, VALUE)`

The `write_csr` built-in function writes the given VALUE to
the current MMIO region at the given OFFSET.

0000:3f:00.0[0]>> write_csr(0x28, 0xdeadbeef)

`device(PCI_ADDR)`

The `device` built-in function allows changing the active
PCIe device.

0000:3f:00.0[0]>> device('0000:2b:00.0')<br>
0000:2b:00.0>>

`region(REGION)`

The `region` built-in function allows changing the active
MMIO region.

0000:2b:00.0>> region(0)<br>
0000:2b:00.0[0]>>

`allocate_buffer(SIZE)`

The `allocate_buffer` built-in function creates and returns
a DMA buffer object. The underlying buffer will be SIZE bytes
in length.

0000:2b:00.0[0]>> b1 = allocate_buffer(4096)<br>
0000:2b:00.0[0]>> print(b1.size, '0x{:0x}'.format(b1.address), b1.io_address)<br>
4096 0x7f9361c66000 0

`version()`

The `version` built-in function returns a tuple containing
the four components used to identify the opae.io version:

0000:2b:00.0[0]>> print(version())<br>
('opae.io', 0, 2, 0)

### BUILT-IN OBJECTS ###

```opae.io``` interactive mode provides two global objects
corresponding to the current device and that device's current
MMIO region. These objects are referred to by global variables
`the_device` and `the_region`, respectively.

The `device` class:

the_device.descriptor() : method that returns the integer file
descriptor of the `VFIO container`.

0000:2b:00.0[0]>> print(the_device.descriptor())<br>
5

the_device.__repr__() : method that is invoked when a `device`
object is printed.

0000:2b:00.0[0]>> print(the_device)<br>
0000:2b:00.0

the_device.allocate(SIZE) : method that allocates and returns a
`system_buffer` object. The buffer will be mapped into the
DMA space of `the_device`.

0000:2b:00.0[0]>> b1 = the_device.allocate(4096)

the_device.pci_address() : read-only property that returns the
PCIe address of `the_device`.

0000:2b:00.0[0]>> print(the_device.pci_address)<br>
0000:2b:00.0

the_device.num_regions : read-only property that returns the
number of MMIO regions in `the_device`.

0000:2b:00.0[0]>> print(the_device.num_regions)<br>
2

the_device.regions : read-only property that returns a list
of the active MMIO regions of `the_device`:

0000:2b:00.0[0]>> print(the_device.regions)<br>
[0, 2]

The `region` class:

the_region.write32(OFFSET, VALUE) : method that writes a
32-bit VALUE to the CSR at OFFSET.

the_region.read32(OFFSET) : method that returns a 32-bit
CSR at the given OFFSET.

0000:2b:00.0[0]>> the_region.write32(0x28, 0xdeadbeef)<br>
0000:2b:00.0[0]>> print('0x{:0x}'.format(the_region.read32(0x28)))<br>
0xdeadbeef

the_region.write64(OFFSET, VALUE): method that writes a
64-bit VALUE to the CSR at OFFSET.

the_region.read64(OFFSET): method that returns a 64-bit
CSR at the given OFFSET.

0000:2b:00.0[0]>> the_region.write64(0x28, 0xbaddecaf)<br>
0000:2b:00.0[0]>> print('0x{:0x}'.format(the_region.read64(0x28)))<br>
0xbaddecaf

the_region.index(): method that returns the MMIO index
of `the_region`.

0000:2b:00.0[0]>> print(the_region.index())<br>
0

the_region.__repr__(): method that is invoked when a `region`
object is printed.

0000:2b:00.0[0]>> print(the_region)<br>
0

the_region.__len__(): method that is invoked to determine the
MMIO region size.

0000:2b:00.0[0]>> print(len(the_region))<br>
524288

The `allocate_buffer()` built-in function and the
`device.allocate()` method return objects of type `system_buffer`.

The `system_buffer` class is as follows:

`buf.size`: read-only property that gives the buffer size.

0000:2b:00.0[0]>> print(b1.size)<br>
4096

`buf.address`: read-only property that gives the buffer's
user mode virtual address.

0000:2b:00.0[0]>> print('0x{:0x}'.format(b1.address))<br>
0x7f2c15d8200

`buf.io_address`: read-only property that gives the buffer's
IO address.

0000:2b:00.0[0]>> print('0x{:0x}'.format(b1.io_address))<br>
0x0

`buf.__getitem__` and `buf.__setitem__`: indexing get/set
of 64-bit data item.

0000:2b:00.0[0]>> b1[0] = 0xdecafbad<br>
0000:2b:00.0[0]>> print('0x{:0x}'.format(b1[0]))<br>
0xdecafbad

`buf.read8(OFFSET)`<br>
`buf.read16(OFFSET)`<br>
`buf.read32(OFFSET)`<br>
`buf.read64(OFFSET)` : methods that read the given size
data item from the given buffer OFFSET.

`buf.fill8(VALUE)`<br>
`buf.fill16(VALUE)`<br>
`buf.fill32(VALUE)`<br>
`buf.fill64(VALUE)` : methods that fill the buffer with
the given VALUE, using the given size.

`b1.compare(b2)`: method that compares buffers.
The method returns the index of the first byte that miscompares,
or the length of b1.

## Revision History ##

Document Version | Intel Acceleration Stack Version | Changes
-----------------|----------------------------------|--------
2021.01.25 | IOFS EA | Initial release.
