=========================
OPAE Python API Reference
=========================

The reference documentation for the OPAE Python API and is grouped into the
following sections:

.. contents::
   :local:


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
        :members: size, wsid, io_address, fill, compare

Error
-----
.. autoclass:: opae.fpga.error
        :members: name, can_clear, read_value

SysObject
---------
.. autoclass:: opae.fpga.sysobject
        :members: __getattr__, __getitem__, find, read64, write64, size, bytes
