// Copyright(c) 2017, Intel Corporation
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
 * @file access.h
 * @brief Functions to acquire, release, and reset OPAE FPGA resources
 */

#ifndef __FPGA_ACCESS_H__
#define __FPGA_ACCESS_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open an FPGA object
 *
 * Acquires ownership of the FPGA resource referred to by 'token'.
 *
 * Most often this will be used to open an accelerator object to directly interact
 * with an accelerator function, or to open an FPGA object to perform
 * management functions.
 *
 * @param[in]  token    Pointer to token identifying resource to acquire
 *                      ownership of
 * @param[out] handle   Pointer to preallocated memory to place a handle in.
 *                      This handle will be used in subsequent API calls.
 * @param[in]  flags    One of the following flags:
 *                        * FPGA_OPEN_SHARED allows the resource to be opened
 *                          multiple times (not supported in ASE)
 *                          Shared resources (including buffers) are released
 *                          when all associated handles have been closed
 *                          (either explicitly with fpgaClose() or by process
 *                          termination).
 *                        * FPGA_OPEN_HAS_PARENT_AFU is currently supported only
 *                          inside OPAE, between the shell and a plugin. It
 *                          indicates that the resource being opened is a child
 *                          of a parent resource that is already open in the
 *                          same process. When set, the value of *handle on
 *                          entry is the parent handle.
 * @returns             FPGA_OK on success. FPGA_NOT_FOUND if the resource for
 *                      'token' could not be found. FPGA_INVALID_PARAM if
 *                      'token' does not refer to a resource that can be
 *                       opened, or if either argument is NULL or invalid.
 *                      FPGA_EXCEPTION if an internal exception occurred while
 *                      creating the handle. FPGA_NO_DRIVER if the driver is
 *                      not loaded. FPGA_BUSY if trying to open a resource that
 *                      has already been opened in exclusive mode.
 *                      FPGA_NO_ACCESS if the current process' privileges are
 *                      not sufficient to open the resource.
 */
fpga_result fpgaOpen(fpga_token token, fpga_handle *handle,
		     int flags);

/**
 * Extract the handles of children of a previously fpgaOpen()ed resource.
 * Only AFUs with feature parameters that name child AFU GUIDs will have
 * children.
 *
 * Child AFU handles may be used to connect to child-specific MMIO regions
 * and manage interrupts. Children will be closed automatically along with
 * the parent. fpgaClose() should not be called on returned child handles.
 *
 * Child handles may not be passed to fpgaPrepareBuffer(). All shared
 * memory management must be associated with the parent.
 *
 * @param[in]  handle       Handle to previously opened FPGA object
 * @param[in]  max_children Maximum number of handles that may be returned
 *                          in the `children` array.
 * @param[out] children     Pointer to an array of child handles currently
 *                          open. When NULL or `max_children` is 0, the
 *                          number of children will still be returned in
 *                          `num_children`.
 * @param[out] num_children Number of children belonging to the parent.
 *                          This number may be higher than `max_children`.
 * @returns                 FPGA_OK on success.
 *                          FPGA_INVALID_PARAM if handle does not refer to
 *                          an acquired resource, or if handle is NULL.
 *                          FPGA_EXCEPTION if an internal error occurred
 *                          while accessing the handle.
 */
fpga_result fpgaGetChildren(fpga_handle handle, uint32_t max_children,
			    fpga_handle *children, uint32_t *num_children);

/**
 * Close a previously opened FPGA object
 *
 * Relinquishes ownership of a previously fpgaOpen()ed resource. This enables
 * others to acquire ownership if the resource was opened exclusively.
 * Also deallocates / unmaps MMIO and UMsg memory areas.
 *
 * @param[in]  handle   Handle to previously opened FPGA object
 * @returns             FPGA_OK on success. FPGA_INVALID_PARAM if handle does
 *                      not refer to an acquired resource, or if handle is NULL.
 *                      FPGA_EXCEPTION if an internal error occurred while
 *                      accessing the handle.
 */
fpga_result fpgaClose(fpga_handle handle);

/**
 * Reset an FPGA object
 *
 * Performs an accelerator reset.
 *
 * @param[in]  handle   Handle to previously opened FPGA object
 * @returns             FPGA_OK on success. FPGA_INVALID_PARAM if handle does
 *                      not refer to an acquired resource or to a resource that
 *                      cannot be reset. FPGA_EXCEPTION if an internal error
 *                      occurred while trying to access the handle or resetting
 *                      the resource.
 */
fpga_result fpgaReset(fpga_handle handle);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_ACCESS_H__
