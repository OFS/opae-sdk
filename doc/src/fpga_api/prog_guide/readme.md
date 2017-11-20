# OPAE C API Programming Guide #

.. toctree::

## Overview ##
The OPAE C library (*libopae-c*) is a lightweight user-space library that
provides abstraction for FPGA resources in a compute environment. Built on
top of the driver stack that supports FPGA device, the library abstracts away
hardware specific and OS specific details and exposes the underlying FPGA
resources as a set of features accessible from within software programs
running on the host. These features include the acceleration logic
preconfigured on the device, as well as functions to manage and reconfigure
the device. Hence, the library is able to enable user applications to
transparently and seamlessly leverage FPGA-based acceleration.

![Layered architecture](./FPGA-lib-1.png "A user space library built on top of FPGA driver stack")


By providing a unified C API, the library supports different kinds of FPGA
integration and deployment models, ranging from single-node systems with one or
more FPGA devices to large-scale FPGA deployment in a data center.
A simple use case, for example, is for a user
application running on a system with an FPGA PCIe device to easily use the FPGA
to accelerate certain algorithms. At the other end of the spectrum, resource
management and orchestration services in a data center can use this API to
discover and select FPGA resources and then dice them
up to be used by workloads with acceleration needs.

## Philosophy ##

The purpose of OPAE is to provide a common base layer for as wide a range of
use cases as possible without sacrificing performance or efficiency. It aims
at freeing the developers of applications and frameworks from having to deal
with driver interfaces and FPGA interconnect details by providing a thin
abstraction to expose required details of the platform.

To that end, OPAE abstracts access to the key components that frameworks and
abstractions need to deal with (for example, FPGA devices and accelerators).
It then provides means to interact with these components in the most
efficient way possible. Essentially, it tries to provide friendly and
consistent interfaces to crucial components of the platform. At the same
time, OPAE tries not to constrain frameworks and applications by making
optimizations that do not translate to many use cases - and where it does
provide convenience functions or optimizations, these will be optional.

For example, OPAE provides an interface to allocate physically contiguous
buffers in system memory that can be shared between user-space software and
an accelerator. This interface enables the most basic feature set of
allocating and sharing a large page of memory in one API call; it however
does *not* provide a malloc()-like interface backed by a memory pool or slab
allocator. These kinds of optimizations and added functionality are left to
higher layers of the software stack, which will be better suited to make
domain-specific optimizations.

## Some key concepts ##
The following key concepts are essential for writing code using the
OPAE C API.
These concepts are modeled with corresponding data structures and functions in
the API specification, as discussed in the [Object model](#object-model) section.

* **FPGA**: [Field Programmable Gate
Array](https://en.wikipedia.org/wiki/Field-programmable_gate_array) is a
discrete or integrated peripheral device connecting to a host CPU via PCIe or
other type of interconnects.
* **AFU**: Accelerator Function Unit, is a computation logic preconfigured on
FPGA with the purpose of accelerating certain computation. It represents a
resource discoverable and usable by user applications. The
logic is designed in RTL and synthesized into a bitstream. A tool (_fpgaconf_)
is provided to reconfigure an FPGA using a bitstream.
* **Green bitstream (GBS)**: A bitstream for an application-specific 
accelerator logic, for example, compression, encryption, mathematical operations, 
etc.
* **Accelerator**: An allocatable accelerator function implemented in an FPGA, 
closely related to an AFU. An accelerator tracks the  _ownership_
of an AFU (or part of it) for a process that uses it. An accelerator can be shared by multiple
processes.
* **Shared memory buffers**: Memory buffers allocated in user process memory
on the host to be shared with an accelerator on the FPGA. Shared memory buffers
fascilitate data transfers between the user process and the accelerator it owns.
* **Events**: Events are asynchronous notification mechanism. The FPGA driver
triggers certian events to indicate error conditions. An accelerator logic can also
define its own events. User applications can choose to be
notified when certain types of the events occur and respond accordingly.
* **Reconfiguration**: An AFU can be replaced by another AFU by a user application
that has appropriate privilege.

## Link with the library ##
Linking with this library is straightforward.
Code using this library should include the header file `fpga.h`. Taking the GCC
compiler on Linux as an example, the minimalist compile and link line should look like:

`gcc myprog.c -I</path/to/fpga.h> -L</path/to/libopae-c.so> -lopae-c -luuid -ljson-c -lpthread`

.. note::

```
Third-party library dependency: The library internally uses `libuuid` and
`libjson-c`. But they are not distributed as part of the library. Make sure you
have these libraries properly installed.
```

## Use the sample code ##
The library source include two code samples. Use these samples
to learn how to call functions in the library. Build and run these samples as
quick sanity checks to determine if your installation and environment are set up
properly. Details about using the sample code can be found in 
[this section in the Quick Start
Guide](/fpga-doc/docs/fpga_api/quick_start/readme.html#building-a-sample-application). 

## High-level directory structure ##
When successfully built and installed, users see the following directory
structure. This discussion is using installation on Unix/Linux systems as an
example. But it will be a similar situation on Windows and MacOS
installations.

|Directory & Files |Contents |
|------------------|---------|
|include/opae      |Directory containing all header files|
|include/opae/fpga.h |Top-level header for user code to include|
|include/opae/access.h |Header file for accelerator acquire/release, MMIO, memory management, event handling, etc. |
|include/opae/bitstream.h |Header file for bitstream manipulation functions |
|include/opae/common.h |Header file for error reporting functions |
|include/opae/enum.h |Header file for AFU enumeration functions |
|include/opae/manage.h |Header file for FPGA management functions |
|include/opae/types.h |Various type definitions |
|lib               |Directory containing shared library files |
|lib/libopae-c.so    |The shared dynamic library for user application to link against|
|doc               |Directory containing API documentation |

## Basic application flow ##
The picture below depicts the basic application flow from the
viewpoint of a user process. 
API components are discussed in the next section. The `hello_fpga.c` sample code
is a good example showing the flow in action.

![Basic flow](./FPGA-lib-2.png "Basic application flow")

## API Components ##
The API is designed around an object model that abstracts physical FPGA device and
functions available on the device. The object model is not tied to a particular
type FPGA product. Instead, it is a generalized model and can be extended to 
describe any type of FPGAs. 

### Object model ###
* `fpga_objtype`: An enum type to represent the type of an FPGA resource, which
is either `FPGA_DEVICE` or `FPGA_ACCELERATOR`. An `FPGA_DEVICE` object is corresponding to
a physical FPGA device. Only `FPGA_DEVICE` objects can invoke management function.
`FPGA_ACCELERATOR` represents an instance of an AFU. 
* `fpga_token`: An opaque type to represent a resource known to but not
necessarily owned by the calling process. The calling process must own a
resource before it can invoke functions of the resource.
* `fpga_handle`: An opaque type to represent a resource owned by the
calling process. API functions `fpgaOpen()` and `fpgaClose()` (see [below](#functions))
acquire and release ownership of a resource represented by an `fpga_handle`.
* `fpga_properties`: An opaque type for a properties object. User
applications use these properties to query and search for resources that suit
their needs. The properties visible to user applications are documented in
[this section](#fpga-resource-properties).
* `fpga_event_handle`: An opaque handle used by FPGA driver to notify user
application about an event, and used by the user application to wait for the
notification of the event.
* `fpga_event_type`: An enum type to represent kinds of events which can be
`FPGA_EVENT_INTERRUPT`, `FPGA_EVENT_ERROR`, or `FPGA_EVENT_POWER_THERMAL`.
* `fpga_result`: An enum type to represent the result of an API function. If the
function returns successfully the result is `FPGA_OK`. Otherwise, the result is
one of the error codes. Function `fpgaErrStr()` can translate an error code
into human-readable strings.

### Functions ###
The table below groups key API functions by their purposes. Consult with the
[OPAE C API reference manual](https://atp-lab.jf.intel.com/fpga-doc/docs/fpga_api/fpga_api.html)
for detail documentation for each function.

|Purpose |Functions |Note |
|--------|----------|-----|
|Enumeration | `fpgaEnumerate()` | Query FPGA resources that match certain properties |
|Enumeration: Properties | `fpga[Get|Update|Clear|Clone|Destroy]Properties]()` | Manage `fpga_properties` life cycle |
|           | `fpgaPropertiesGet[Prop]()` | Get a certain property *Prop*, see [below](#fpga-resource-properties) |
|           | `fpgaPropertiesSet[Prop]()` | Set a certain property *Prop*, see [below](#fpga-resource-properties) |
|Access: Ownership  | `fpga[Open|Close]()` | Aquire/release ownership |
|Access: Reset      | `fpgaReset()` | Reset an accelerator |
|Access: Event handling | `fpga[Register|Unregister]Event()` | Register/unregister an event to be notified about |
|               | `fpga[Create|Destroy]EventHandle()` | Manage `fpga_event_handle` life cycle |
|Access: UMsg           | `fpgaGetNumUmsg()`, `fpgaSetUmsgAttributes()`, `fpgaTriggerUmsg()`, `fpgaGetUmsgPtr()` | Low-latency accelerator notification mechanism |
|Access: MMIO       | `fpgaMapMMIO()`, `fpgaUnMapMMIO()` | Map/unmap MMIO space |
|           | `fpgaGetMMIOInfo()` | Get information about a particular MMIO space |
|           | `fpgaReadMMIO[32|64]()` | Read a 32-bit/64-bit value from MMIO space |
|           | `fpgaWriteMMIO[32|64]()` | Write a 32-bit/64-bit value to MMIO space |
|Memory management: Shared memory | `fpga[Prepare|Release]Buffer()` | Manage memory buffer shared between the calling process and an accelerator |
|              | `fpgaGetIOVA()` | Return the virtual address of a shared memory buffer |
|Management: Reconfiguration | `fpgaReconfigureSlot()` | Replace an existing AFU with a new one |
|Error report | `fpgaErrStr()` | Map an error code to a human readable string |

### FPGA resource properties ###
These are the properties of a resource that can be queried by a user application,
by plugging  property name for `Prop` in the names of `fpgaPropertiesGet[Prop]()` and
`fpgaPropertiesSet[Prop]()` functions.

|Property |FPGA |accelerator |Note |
|---------|-----|----|-----|
|Parent |No |Yes |`fpga_token` of the parent object |
|ObjectType |Yes |Yes |The type of the resource: either `FPGA_DEVICE` or `FPGA_ACCELERATOR` |
|Bus |Yes |Yes |The bus number |
|Device |Yes |Yes |The PCI device number |
|Function |Yes |Yes |The PCI function number |
|SocketId |Yes |Yes |The socket ID |
|DeviceId |Yes |Yes |The device ID |
|NumSlots |Yes |No |Number of AFU slots available on an `FPGA_DEVICE` resource |
|BBSId |Yes |No |The blue bitstream ID of an `FPGA_DEVICE` resource |
|BBSVersion |Yes |No |The blue bitstream version of an `FPGA_DEVICE` resource |
|VendorId |Yes |No |The vendor ID of an `FPGA_DEVICE` resource |
|Model |Yes |No |The model of an `FPGA_DEVICE` resource |
|LocalMemorySize |Yes |No |The local memory size of an `FPGA_DEVICE` resource |
|Capabilities |Yes |No |The capabilities of an `FPGA_DEVICE` resource |
|Guid |Yes |Yes |The GUID of an `FPGA_DEVICE` or `FPGA_ACCELERATOR` resource |
|NumMMIO |No |Yes |The number of MMIO space of an `FPGA_ACCELERATOR` resource |
|NumInterrupts |No |Yes |The number of interrupts of an `FPGA_ACCELERATOR` resource |
|AcceleratorState |No |Yes |The state of an `FPGA_ACCELERATOR` resource: either `FPGA_ACCELERATOR_ASSIGNED` or `FPGA_ACCELERATOR_UNASSIGNED`|

## Usage Models ##
This section illustrates a few typical API usage models with code snippets.

### Query and search for a resource ###
User code first populates an `fpga_properties` object with desired properties.
Then, it passes it to `fpgaEnumerate()` to search for matching resources.
`fpgaEnumerate()` may return more than one matching resources.

```c
    #include "fpga/fpga.h"

    fpga_guid               guid;
    fpga_properties         filter = NULL;
    fpga_result             res;
    fpga_token              tokens[MAX_NUM_TOKENS];
    uint32_t                num_matches = 0;

    /* Start with an empty properties object */
    res = fpgaGetProperties(NULL, &filter);

    /* Populate the properties object with desired values.
       In this case, we want to search for accelerators that match a
       particular GUID.
    */
    uuid_parse(GUID, guid);
    res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
    res = fpgaPropertiesSetGuid(filter, guid);

    /* Query the number of matched resources */
    res = fpgaEnumerate(&filter, 1, NULL, 1, &num_matches);

    /* Return all matched resources in tokens */
    res = fpgaEnumerate(&filter, 1, tokens, num_matches, &num_matches);

    /* Destroy the properties object */
    res = fpgaDestroyProperties(&filter);

    /* More code */
    ......

    /* Destroy tokens */
    for (uint32_t i = 0; i < num_matches; ++i) {
        res = fpgaDestroyToken(tokens[i]);
    }
```

.. note::

```
The `fpgaEnumerate()` function can take multiple `fpg_properties`
objects (in an array). In this situation, the function returns resources that
match *any* of the properties object. In other words, the multiple properties
objects are logically OR'ed in the query operation.
* Again, `fpga_token` objects return by `fpgaEnumerate()` do *not* signify
ownership. To acquire ownership of a resource represented by a token, pass the
token to `fpgaOpen()`.
```

### Acquire and release a resource ###
Straighforwardly enough, acquiring/releasing ownership of a resource is done
using
`fpgaOpen()` and `fpgaClose()`. The calling process must own the resource
before it can do MMIO, share memory buffers, and use functions offered by the
resource.

```c
    #include "fpga/fpga.h"

    fpga_handle             handle;
    fpga_result             res;

    /* Acquire ownership of a resource that was previously returned by
       `fpgaEnumerate()` as a token
    */
    res = fpgaOpen(token, &handle);

    /* More code */
    ......

    /* Release the ownership */
    res = fpgaClose(handle);
```

### Shared memory buffer ###
This code snippet shows how to prepare a memory buffer for sharing between the
calling process and an accelerator.

```c
    #include "fpga/fpga.h"

    fpga_handle             handle;
    fpga_result             res;

    /* Hint for the virtual address of the buffer */
    volatile uint64_t       *addr_hint;
    /* An ID we can use to reference the buffer later */
    uint32_t                bufid;
    /* Flag to indicate if the buffer is preallocated or not */
    int                     flag = 0;

    /* Allocate (if necessary), pin, and map a buffer to be accessible
       by an accelerator
    */
    res = fpgaPrepareBuffer(handle, BUF_SIZE, (void **) &addr_hint,
                            &bufid, flag);

    /* The actual address mapped to the buffer */
    uint64_t                iova;
    /* Get the IO virtual address for the buffer */
    res = fpgaGetIOVA(handle, bufid, &iova);

    /* Inform the accelerator about the virtual address by writing to its mapped
       register file
    */
    ......

    /* More code */
    ......

    /* Release the shared buffer */
    res = fpgaReleaseBuffer(handle, bufid);
```

.. note::

```
The `flag` variable can take a constant `FPGA_BUF_PREALLOCATED`, which
indicates that the address space pointed to by `addr_hint` is already allocated
by the calling process.
```

### MMIO ###
This code snippet shows how to map/unmap the register file of an accelerator into the
virtual memory space of the calling process.

```c
    #include "fpga/fpga.h"

    fpga_handle             handle;
    fpga_result             res;

    /* Index of the MMIO space. There might be multiple spaces on an accelerator */
    uint32_t                mmio_num = 0;
    /* Mapped address */
    uint64_t                mmio_addr;

    /* Map MMIO */
    res = fpgaMapMMIO(handle, mmio_num, &mmio_addr);

    /* Write to a 32-bit value to the mapped register file at a certain byte
       offset.

       CSR_CTL is the offset in the mapped space to where the value will be
       written. It's defined elsewhere.
    */
    res = fpgaWriteMMIO32(handle, mmio_num, CSR_CTL, value);

    /* More code */
    ......

    /* Unmap MMIO */
    res = fpgaUnmapMMIO(handle, mmio_num);
```

.. note::

```
Every AFU has its own layout of register spaces and its own protocol about
how to control its behavior through the registers. These are defined in the
green bitstream used to implemented the AFU.
```

