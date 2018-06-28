=========================
OPAE Python API Reference
=========================

The reference documentation for the OPAE Python API and is grouped into the
following sections:

.. contents::
   :local:

Overview
========
The OPAE Python API is built on top of the OPAE C++ Core API and its object
model. Because of this, developing OPAE applications in Python is very similar
to developing OPAE applications in C++ which significantly reduces the learning
curve required to adapt to the Python API. While the object model remains the same,
some static factory functions in the OPAE C++ Core API have been moved to module
level methods in the OPAE Python API with the exception of the properties class.
The goal of the OPAE Python API is to enable fast prototyping, test automation,
infrastructure managment, and an easy to use framework for FPGA resource
interactions that don't rely on software algorithms with a high runtime
complexity.

Implementation
==============
The OPAE Python API is implemented by creating a Python extension using `pybind11
<http://pybind11.readthedocs.io/en/stable>`_.
This extension is created by using the pybind11 API which relies mostly on
macros and compile time introspection to define the module initialization point
as well as type converters between OPAE C++ Core types and OPAE Python types.

Benefits
--------
The major benefits of using pybind11 for developing the OPAE Python API
include, but are not limited to, the following features of pybind11:

* Uses C++ 11 standard library although it can use C++ 14 or C++17.
* Automatic conversions of shared_ptr types
* Built-in support for numpy and Eigen numerical libraries
* Interoperable with the Python C API


Module Types, Methods, and Constants
====================================
.. automodule:: opae.fpga
        :members: enumerate, open, register_event, allocate_shared_buffer, errors, DEVICE, ACCELERATOR, OPEN_SHARED, EVENT_ERROR, EVENT_INTERRUPT, EVENT_POWER_THERMAL, ACCELERATOR_ASSIGNED, ACCELERATOR_UNASSIGNED, RECONF_FORCE




Fundamental Types
=================


Properties
----------
.. autoclass:: opae.fpga.properties
        :members: __init__, accelerator_state, bbs_id, bbs_version, bus, capabilities, device, function, model, num_interrupts, num_mmio, num_slots, object_id, parent, socket_id, vendor_id


Token
-----
.. autoclass:: opae.fpga.token
        :members:

Handle
------
.. autoclass:: opae.fpga.handle
        :members: __enter__, __exit__, close, reset, read_csr32, read_csr64, write_csr32, write_csr64

Event
-----
.. autoclass:: opae.fpga.event
        :members: os_object

Shared Buffer
-------------
.. autoclass:: opae.fpga.shared_buffer
        :members: size, wsid, iova, fill, compare, memoryview

Error
-----
.. autoclass:: opae.fpga.error
        :members: name, can_clear, read_value


