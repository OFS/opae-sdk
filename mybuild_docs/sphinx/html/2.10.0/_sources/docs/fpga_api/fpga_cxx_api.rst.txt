===========================
OPAE C++ Core API Reference
===========================

The reference documentation for the OPAE C++ Core API is grouped into the following
sections:

.. contents::
   :local:

Overview
========

The OPAE C++ API enables C++ developers with the means to use FPGA resources
by integrating the OPAE software stack into C++ applications.

Goals
=====

Simplicity
----------

Keep the API as small and lightweight as possible. Although features such as
system validation and orchestration are beyond the scope of this API, using
this API for their development should be relatively easy.

Extensibility and Interoperability
----------------------------------

While keeping to the goal of simplicity, the OPAE C++ API is designed to allow
for better reuse by either extending the API or by integrating with other
languages.

Modern C++ Coding Practices
---------------------------

The OPAE C++ API uses the C++ 11 standard library and makes use of its features
whenever practical. The OPAE C++ API is also designed to require the minimum
number of third-party libraries/dependencies.

Error Handling
--------------

The OPAE C++ API is designed to throw exceptions when appropriate. The
structure of OPAE C++ exceptions is similar to the error codes in the
OPAE C API. This gives users of the API more freedom on error handling
while providing better debug information in cases of failure.

Coding Style
------------

For formatting of the OPAE C++ API complies with most of the recommendations
of the Google C++ style. For example, the OPAE C++ API uses:

* opening braces on the same line as their scope definition
* spaces instead of tabs for indentation
* indentation of two spaces

Fundamental Types
=================

Basic types for the OPAE C++ API are found in the `opae::fpga::types`
namespace. They serve as an adapter layer between the OPAE C API and
the OPAE C++ layer. Aside from providing a C++ binding to the C
fundamental types, these types also:

* manage the lifetime and scope of the corresponding C struct.
 * For example a C++ destructor will take care of calling the
   appropriate C function to release the data structure being
   wrapped.
* provide a friendly syntax for using the OPAE C type.

Most classes in this namespace have a `c_type()` method that returns
the C data structure being wrapped, making it easy to use the OPAE C++
type with the OPAE C API. Alternatively, most classes in this namespace
have implicit conversion operators that enable interoperability with
the OPAE C API.

Properties
----------

C++ class `properties` wraps `fpga_properties` and uses `pvalue`
and `guid_t` to get and set properties stored in an instance of
an `fpga_properties`. `pvalue` and `guid_t` are designed to call
an accessor method in the OPAE C API to either read property
values or write them. Most accessor methods in the OPAE C API
share a similar signature, so `pvalue` generalizes them into
common operations that translate into calling the corresponding
C API function. `guid_t` follows similar patterns when reading
or assigning values.

pvalue.h
--------

.. doxygenfile:: include/opae/cxx/core/pvalue.h

properties.h
------------

.. doxygenfile:: include/opae/cxx/core/properties.h

Resource Classes
----------------

The `token`, `handle`, and `shared_buffer` classes are used to
enumerate and access FPGA resources. `properties` are used to
narrow the search space for `token`'s. Before enumerating the
accelerator resources in the system, applications can produce
one or more `properties` objects whose values are set to the
desired characteristics for the resource. For example, an
application may search for an accelerator resource based on
its guid.

Once one or more `token`'s have been enumerated, the application
must choose which `token`'s to request. The `token` is then
converted to a `handle` by requesting that a `handle` object
be allocated and opened for it.

Once a `handle` has been successfully opened, the application
can read and write the associated configuration and status
space. Additionally, the application may use the `handle` to
allocate `shared_buffer`'s or to register `event`'s. The
`shared_buffer` and `event` objects retain a reference to
their owning `handle` so that the `handle` does not lose
scope before freeing the `shared_buffer` and `event` objects.

token.h
--------

.. doxygenfile:: include/opae/cxx/core/token.h

handle.h
--------

.. doxygenfile:: include/opae/cxx/core/handle.h

shared_buffer.h
---------------

.. doxygenfile:: include/opae/cxx/core/shared_buffer.h

errors.h
--------

.. doxygenfile:: include/opae/cxx/core/errors.h

events.h
-------

.. doxygenfile:: include/opae/cxx/core/events.h

sysobject.h
-----------

.. doxygenfile:: include/opae/cxx/core/sysobject.h

Exceptions
----------

When the OPAE C++ API encounters an error from the OPAE C
API, it captures the current source code location and
the error code into an object of type `except`, then
throws the `except`. Applications should implement the
appropriate catch blocks required to respond to runtime
exceptions.

except.h
--------

.. doxygenfile:: include/opae/cxx/core/except.h

Misc
----

The `version` class wraps the OPAE C version API.

version.h
---------

.. doxygenfile:: include/opae/cxx/core/version.h

