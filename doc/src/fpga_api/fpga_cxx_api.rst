===========================
OPAE CXX Core API Reference
===========================

The reference documentation for the OPAE CXX Core API is grouped into the following
sections:

.. contents::
   :local:


Enumeration API
===============

properties.h
------------

.. doxygenfile:: include/opae/cxx/core/properties.h


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

events.h
-------

.. doxygenfile:: include/opae/cxx/core/events.h

Misc
====

except.h
--------

.. doxygenfile:: include/opae/cxx/core/except.h

handle.h
--------

.. doxygenfile:: include/opae/cxx/core/handle.h

pvalue.h
--------

.. doxygenfile:: include/opae/cxx/core/pvalue.h

shared_buffer.h
--------

.. doxygenfile:: include/opae/cxx/core/shared_buffer.h

token.h
--------

.. doxygenfile:: include/opae/cxx/core/token.h

version.h
--------

.. doxygenfile:: include/opae/cxx/core/version.h





