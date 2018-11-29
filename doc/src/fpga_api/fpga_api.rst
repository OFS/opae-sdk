====================
OPAE C API Reference
====================

The reference documentation for the OPAE C API is grouped into the following
sections:

.. contents::
   :local:


Types
=====

The OPAE C API defines a number of types; most prominent are the types
`fpga_token`, `fpga_handle`, and `fpga_properties`. All regular types are
defined in [types.h](#types-h), while the values of enumeration types are
defined in [types_enum.h](#types-enum-h).

types.h
-------

.. doxygenfile:: include/opae/types.h

types_enum.h
------------

.. doxygenfile:: include/opae/types_enum.h


Enumeration API
===============

The OPAE enumeration API allows selective discovery of FPGA resources. When
enumerating resources, a list of filter criteria can be passed to the
respective function to select a subset of all resources in the system. The
fpgaEnumerate() function itself then returns a list of fpga_tokens denoting
resources, which can be used in subsequent API calls.

Filter criteria are specified using one or more fpga_properties object. These
objects need to be created using fpgaGetProperties() (defined in
<opae/properties/h>) before being passed to fpgaEnumerate(). Individual
attributes of an fpga_properties object are set using specific accessors,
which are also defined in <opae/properties.h>.

enum.h
------

.. doxygenfile:: include/opae/enum.h

properties.h
------------

.. doxygenfile:: include/opae/properties.h

Access API
==========

The access API provides functions for opening and closing FPGA resources.
Opening a resource yields an fpga_handle, which denotes ownership and can be
used in subsequent API calls to interact with a specific resource. Ownership
can be exclusive or shared.

access.h
--------

.. doxygenfile:: include/opae/access.h

Event API
=========

The event API provides functions and types for handling asynchronous events
such as errors or AFC interrupts.

To natively support asynchronous event, the driver for the FPGA platform
needs to support events natively (in which case the OPAE C library will
register the event directly with the driver). For some platforms that do not
support interrupt-driven event delivery, you need to run the FPGA Daemon
(fpgad) to enable asynchronous OPAE events. fpgad will act as a proxy for the
application and deliver asynchronous notifications for registered events.

event.h
-------

.. doxygenfile:: include/opae/event.h


MMIO and Shared Memory APIs
===========================

These APIs feature functions for mapping and accessing control registers
through memory-mapped IO (mmio.h), allocating and sharing system memory
buffers with an accelerator (buffer.h), and using low-latency notifications
(umsg.h).

mmio.h
------

.. doxygenfile:: include/opae/mmio.h

buffer.h
--------

.. doxygenfile:: include/opae/buffer.h

umsg.h
------

.. doxygenfile:: include/opae/umsg.h


Management API
==============

The management APIs define functions for reconfiguring an FPGA (writing new
partial bitstreams) as well as assigning accelerators to host interfaces.

manage.h
--------

.. doxygenfile:: include/opae/manage.h


Metrics API
==============

The metrics APIs define functions for discovery/enumeration of metrics information
and reading metrics values.

metrics.h
--------

.. doxygenfile:: include/opae/metrics.h

Utilities
=========

Functions for mapping fpga_result values to meaningful error strings are
provided by the utilities API.

utils.h
-------

.. doxygenfile:: include/opae/utils.h


Samples
=======

Code samples demonstrate how to use OPAE C API.

hello_fpga.c
------------

.. doxygenfile:: samples/hello_fpga.c

hello_events.c
--------------

.. doxygenfile:: samples/hello_events.c


