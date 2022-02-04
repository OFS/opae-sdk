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
 * @file manage.h
 * @brief Functions for managing FPGA configurations
 *
 * FPGA accelerators can be reprogrammed at run time by providing new partial
 * bitstreams ("green bitstreams"). This file defines API functions for
 * programming green bitstreams as well as for assigning accelerators to host
 * interfaces for more complex deployment setups, such as virtualized systems.
 */

#ifndef __FPGA_MANAGE_H__
#define __FPGA_MANAGE_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* Assign Port to a host interface.
*
* This function assign Port to a host interface for subsequent use. Only
* Port that have been assigned to a host interface can be opened by
* fpgaOpen().
*
* @param[in]  fpga           Handle to an FPGA object previously opened that
*                            both the host interface and the slot belong to
* @param[in]  interface_num  Host interface number
* @param[in]  slot_num       Slot number
* @param[in]  flags          Flags (to be defined)
* @returns                   FPGA_OK on success
*                            FPGA_INVALID_PARAM if input parameter combination
*                            is not valid.
*                            FPGA_EXCEPTION if an exception occcurred accessing
*                            the `fpga` handle.
*                            FPGA_NOT_SUPPORTED if driver does not support
*                            assignment.
*/
fpga_result fpgaAssignPortToInterface(fpga_handle fpga,
					uint32_t interface_num,
					uint32_t slot_num,
					int flags);

/**
 * Assign an accelerator to a host interface
 *
 * This function assigns an accelerator to a host interface for subsequent use. Only
 * accelerators that have been assigned to a host interface can be opened by
 * fpgaOpen().
 *
 * @note This function is currently not supported.
 *
 * @param[in]  fpga           Handle to an FPGA object previously opened that
 *                            both the host interface and the accelerator belong to
 * @param[in]  accelerator            accelerator to assign
 * @param[in]  host_interface Host interface to assign accelerator to
 * @param[in]  flags          Flags (to be defined)
 * @returns                   FPGA_OK on success
 */
fpga_result fpgaAssignToInterface(fpga_handle fpga,
				  fpga_token accelerator,
				  uint32_t host_interface,
				  int flags);

/**
 * Unassign a previously assigned accelerator
 *
 * This function removes the assignment of an accelerator to an host interface (e.g. to
 * be later assigned to a different host interface). As a consequence, the accelerator
 * referred to by token 'accelerator' will be reset during the course of this function.
 *
 * @note This function is currently not supported.
 *
 * @param[in]  fpga           Handle to an FPGA object previously opened that
 *                            both the host interface and the accelerator belong to
 * @param[in]  accelerator            accelerator to unassign/release
 * @returns                   FPGA_OK on success
 */
fpga_result fpgaReleaseFromInterface(fpga_handle fpga,
				     fpga_token accelerator);

/**
 * Reconfigure a slot
 *
 * Sends a green bitstream file to an FPGA to reconfigure a specific slot. This
 * call, if successful, will overwrite the currently programmed AFU in that
 * slot with the AFU in the provided bitstream.
 *
 * As part of the reconfiguration flow, all accelerators associated with this slot will
 * be unassigned and reset.
 *
 * @param[in]  fpga           Handle to an FPGA object previously opened
 * @param[in]  slot           Token identifying the slot to reconfigure
 * @param[in]  bitstream      Pointer to memory holding the bitstream
 * @param[in]  bitstream_len  Length of the bitstream in bytes
 * @param[in]  flags          Flags that control behavior of reconfiguration.
 *                            Value of 0 indicates no flags. FPGA_RECONF_FORCE
 *                            indicates that the bitstream is programmed into
 *                            the slot without checking if the resource is
 *                            currently in use.
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if the provided parameters
 * are not valid. FPGA_EXCEPTION if an internal error occurred accessing the
 * handle or while sending the bitstream data to the driver. FPGA_BUSY if the
 * accelerator for the given slot is in use. FPGA_RECONF_ERROR on errors
 * reported by the driver (such as CRC or protocol errors).
 *
 * @note By default, fpgaReconfigureSlot will not allow reconfiguring a slot
 * with an accelerator in use. Add the flag FPGA_RECONF_FORCE to force
 * reconfiguration without checking for accelerators in use.
 */
fpga_result fpgaReconfigureSlot(fpga_handle fpga,
				uint32_t slot,
				const uint8_t *bitstream,
				size_t bitstream_len, int flags);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_MANAGE_H__

