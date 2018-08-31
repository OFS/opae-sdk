// Copyright(c) 2018, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * @file types.h
 * @brief Type definitions for FPGA API
 *
 * OPAE uses the three opaque types fpga_properties, fpga_token, and
 * fpga_handle to create a hierarchy of objects that can be used to enumerate,
 * reference, acquire, and query FPGA resources. This object model is designed
 * to be extensible to account for different FPGA architectures and platforms.
 *
 * Initialization
 * --------------
 * OPAEs management of the opaque types `fpga_properties`,
 * `fpga_token`, and `fpga_handle` relies on the proper initialization of
 * variables of these types. In other words, before doing anything with a
 * variable of one of these opaque types, you need to first initialize them.
 *
 * The respective functions that initizalize opaque types are:
 *
 *   * fpgaGetProperties() and fpgaCloneProperties() for `fpga_properties`
 *   * fpgaEnumerate() and fpgaCloneToken() for `fpga_token`
 *   * fpgaOpen() for `fpga_handle`
 *
 * This should intuitively make sense - fpgaGetProperties() creates
 * `fpga_properties` objects, fpgaEnumerate() creates `fpga_token` objects,
 * fpgaOpen() creates `fpga_handle` objects, and fpgaCloneProperties() and
 * fpgaCloneToken() clone (create) `fpga_properties` and `fpga_token` objects,
 * respectively.
 *
 * Since these opaque types are interpreted as pointers (they are typedef'd to
 * a `void *`), passing an uninitialized opaque type into any function except
 * the respective initailzation function will result in undefined behaviour,
 * because OPAE will try to follow an invalid pointer. Undefined behaviour in
 * this case may include an unexpected error code, or an application crash.
 *
 */

#ifndef __FPGA_TYPES_H__
#define __FPGA_TYPES_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <opae/types_enum.h>

/**
 * Object for expressing FPGA resource properties
 *
 * `fpga_properties` objects encapsulate all enumerable information about an
 * FPGA resources. They can be used for two purposes: selective enumeration
 * (discovery) and querying information about existing resources.
 *
 * For selective enumeration, usually an empty `fpga_properties` object is
 * created (using fpgaGetProperties()) and then populated with the desired
 * criteria for enumeration. An array of `fpga_properties` can then be passed
 * to fpgaEnumerate(), which will return a list of `fpga_token` objects
 * matching these criteria.
 *
 * For querying properties of existing FPGA resources, fpgaGetProperties() can
 * also take an `fpga_token` and will return an `fpga_properties` object
 * populated with information about the resource referenced by that token.
 *
 * After use, `fpga_properties` objects should be destroyed using
 * fpga_destroyProperties() to free backing memory used by the
 * `fpga_properties` object.
 */
typedef void *fpga_properties;

/**
 * Token for referencing FPGA resources
 *
 * An `fpga_token` serves as a reference to a specific FPGA resource present in
 * the system. Holding an `fpga_token` does not constitute ownership of the
 * FPGA resource - it merely allows the user to query further information about
 * a resource, or to use fpgaOpen() to acquire ownership.
 *
 * `fpga_token`s are usually returned by fpgaEnumerate() or
 * fpgaPropertiesGetParent(), and used by fpgaOpen() to acquire ownership and
 * yield a handle to the resource. Some API calls also take `fpga_token`s as
 * arguments if they don't require ownership of the resource in question.
 */
typedef void *fpga_token;

/**
 * Handle to an FPGA resource
 *
 * A valid `fpga_handle` object, as populated by fpgaOpen(), denotes ownership
 * of an FPGA resource. Note that ownership can be exclusive or shared,
 * depending on the flags used in fpgaOpen(). Ownership can be released by
 * calling fpgaClose(), which will render the underlying handle invalid.
 *
 * Many OPAE C API functions require a valid token (which is synonymous with
 * ownership of the resource).
 */
typedef void *fpga_handle;

/**
 * Globally unique identifier (GUID)
 *
 * GUIDs are used widely within OPAE for helping identify FPGA resources. For
 * example, every FPGA resource has a `guid` property, which can be (and in the
 * case of FPGA_ACCELERATOR resource primarily is) used for enumerating a resource of a
 * specific type.
  *
 * `fpga_guid` is compatible with libuuid's uuid_t, so users can use libuuid
 * functions like uuid_parse() to create and work with GUIDs.
 */
typedef uint8_t fpga_guid[16];

/**
 * Semantic version
 *
 * Data structure for expressing version identifiers following the semantic
 * versioning scheme. Used in various properties for tracking component
 * versions.
 */
typedef struct {
	uint8_t major;        /**< Major version */
	uint8_t minor;        /**< Minor version */
	uint16_t patch;       /**< Revision or patchlevel */
} fpga_version;

/** Handle to an event object
 *
 * OPAE provides an interface to asynchronous events that can be generated by
 * different FPGA resources. The event API provides functions to register for
 * these events; associated with every event a process has registered for is an
 * `fpga_event_handle`, which encapsulates the OS-specific data structure for
 * event objects.
 *
 * After use, `fpga_event_handle` objects should be destroyed using
 * fpgaDestroyEventHandle() to free backing memory used by the
 * `fpga_event_handle` object.
 */
typedef void *fpga_event_handle;

/** Information about an error register
 *
 * This data structure captures information about an error register exposed by
 * an accelerator resource. The error API provides functions to retrieve these
 * information structures from a particular resource.
 */

typedef void *fpga_object;
#define FPGA_ERROR_NAME_MAX 64
struct fpga_error_info {
	char name[FPGA_ERROR_NAME_MAX];   /** name of the error */
	bool can_clear;                   /** whether error can be cleared */
};

typedef void *fpga_object;

#endif // __FPGA_TYPES_H__
