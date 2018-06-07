=========================
OPAE Python API Reference
=========================

The reference documentation for the OPAE Python API

.. contents::
   :local:

.. autoclass:: opae.api.fpga_objtype
        :members: FPGA_DEVICE, FPGA_ACCELERATOR

.. autoclass:: opae.api.fpga_open_flags
        :members: FPGA_OPEN_SHARED

.. autoclass:: opae.api.fpga_event_type
        :members: FPGA_EVENT_INTERRUPT, FPGA_EVENT_ERROR, FPGA_EVENT_POWER_THERMAL

.. autoclass:: opae.api.fpga_accelerator_state
        :members: FPGA_ASSIGNED, FPGA_UNASSIGNED

.. autoclass:: opae.api.fpga_reconf_flags
        :members: FPGA_RECONF_FORCE

.. autoclass:: opae.api.properties
        :members: __init__, accelerator_state, bbs_id, bbs_version, bus, capabilities, device, function, model, num_interrupts, num_mmio, num_slots, object_id, parent, socket_id, vendor_id

.. autoclass:: opae.api.token
        :members:

.. autoclass:: opae.api.handle
        :members:

.. autoclass:: opae.api.event
        :members:

.. autoclass:: opae.api.shared_buffer
        :members:


