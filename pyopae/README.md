# OPAE Python Bindings

OPAE (Open Programmable Acceleration Engine) now includes Python bindings for
interacting with FPGA resources. The OPAE Python API is built on top of the
OPAE C++ Core API and its object model. Because of this, developing OPAE
applications in Python is very similar to developing OPAE applications in C++
which significantly reduces the learning curve required to adapt to the Python API.
While the object model remains the same, some static factory functions in the
OPAE C++ Core API have been moved to module level methods in the OPAE Python API
with the exception of the properties class. The goal of the OPAE Python API is
to enable fast prototyping, test automation, infrastructure managment, and an
easy to use framework for FPGA resource interactions that don't rely on software
algorithms with a high runtime complexity.

Currently, the only Python package that is part of OPAE is `opae.fpga`

## Implementation

The OPAE Python API is implemented by creating a Python extension using `pybind11
<http://pybind11.readthedocs.io/en/stable>`_.
This extension is created by using the pybind11 API which relies mostly on
macros and compile time introspection to define the module initialization point
as well as type converters between OPAE C++ Core types and OPAE Python types.

## Benefits
The major benefits of using pybind11 for developing the OPAE Python API
include, but are not limited to, the following features of pybind11:

* Uses C++ 11 standard library although it can use C++ 14 or C++17.
* Automatic conversions of shared_ptr types
* Built-in support for numpy and Eigen numerical libraries
* Interoperable with the Python C API

## Runtime Requirements
Because opae.fpga is built on top of the opae-cxx-core API, it does require
that the runtime libraries for both opae-cxx-core and opae-c be installed on
the system (as well as any other libraries they depend on). Those libraries can
be installed using the opae-libs package (from either RPM or DEB format -
depending on your Linux distribution).

## Installation

## Python Wheels
The preferred method of installation is to use a binary wheel package for your
version of Python.

The following table lists example names for different Python versions and
platforms.

| Python Version | Python ABI      | Linux Platform | Package Name |
|----------------|-----------------|----------------|--------------|
| 2.7 | CPython w/ UCS4 | x86_64 | opae.fpga.<release>-cp27-cp27mu-linux_x86_64.whl |
| 3.4 | CPython w/ UCS4 | x86_64 | opae.fpga.<release>-cp34-cp34mu-linux_x86_64.whl |
| 3.6 | CPython w/ UCS4 | x86_64 | opae.fpga.<release>-cp36-cp36mu-linux_x86_64.whl |


opae.fpga is currently not available in the Python Package Index but once it
does become available, one should be able to install using pip by simply typing
the following:
```shell
> pip install --user opae.fpga
```

## Installing From Source
In addition to the runtime libraries mentioned above, installing from source
does require that the OPAE header files be installed as well as those header
files for pybind11. The former can be installed with the opae-devel package and
the latter can be installed by installing pybind11 Python module.

### Example Installation
The following example shows how to build from source by installing the
prerequisites before running the setup.py file.

```shell
>sudo yum install opae-libs-<release>.x86_64.rpm
>sudo yum install opae-devel-<release>.x86_64.rpm
>pip install --user pybind11
>pip install --user opae.fpga-<release>.tar.gz
```


_NOTE_: The `pip` examples above use the `--user` flag to avoid requiring root
permissions. Those packages will be installed in the user's `site-packages`
directory found in the user's `.local` directory.

## Example Scripts
The following example is an implementation of the sample, hello_fpga.c, which
is designed to configure the NLB0 diagnostic accelerator for a simple loopback.


```Python
import time
from opae import fpga

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"
CTL = 0x138
CFG = 0x140
NUM_LINES = 0x130
SRC_ADDR = 0x0120
DST_ADDR = 0x0128
DSM_ADDR = 0x0110
DSM_STATUS = 0x40

def cl_align(addr):
    return addr >> 6

tokens = fpga.enumerate(type=fpga.ACCELERATOR, guid=NLB0)
assert tokens, "Could not enumerate accelerator: {}".format(NlB0)

with fpga.open(tokens[0], fpga.OPEN_SHARED) as handle:
    src = fpga.allocate_shared_buffer(handle, 4096)
    dst = fpga.allocate_shared_buffer(handle, 4096)
    dsm = fpga.allocate_shared_buffer(handle, 4096)
    handle.write_csr32(CTL, 0)
    handle.write_csr32(CTL, 1)
    handle.write_csr64(DSM_ADDR, dsm.io_address())
    handle.write_csr64(SRC_ADDR, cl_align(src.io_address())) # cacheline-aligned
    handle.write_csr64(DST_ADDR, cl_align(dst.io_address())) # cacheline-aligned
    handle.write_csr32(CFG, 0x42000)
    handle.write_csr32(NUM_LINES, 4096/64)
    handle.write_csr32(CTL, 3)
    while dsm[DSM_STATUS] & 0x1 == 0:
        time.sleep(0.001)
    handle.write_csr32(CTL, 7)

```

This example shows how one might reprogram (Partial Reconfiguration) an
accelerator on a given bus, 0x5e, using a bitstream file, m0.gbs.

```Python
from opae import fpga

BUS = 0x5e
GBS = 'm0.gbs'
tokens = fpga.enumerate(type=fpga.DEVICE, bus=BUS)
assert tokens, "Could not enumerate device on bus: {}".format(BUS)
with open(GBS, 'rb') as fd, fpga.open(tokens[0]) as device:
    device.reconfigure(0, fd)
```

