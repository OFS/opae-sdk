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
 * \file umsg.h
 * \brief FPGA UMsg API
 */

#ifndef __FPGA_UMSG_H__
#define __FPGA_UMSG_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get number of Umsgs
 *
 * Retuns number of umsg supported by AFU.
 *
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[out] value    Returns number of UMsgs
 * @returns             FPGA_OK on success.
 *                      FPGA_INVALID_PARAM if input parameter combination
 *                      is not valid.
 *                      FPGA_EXCEPTION if input parameter fpga handle is not
 *                      valid.
 */
fpga_result fpgaGetNumUmsg(fpga_handle handle, uint64_t *value);

/**
 * Sets Umsg hint
 *
 * Writes usmg hint bit.
 *
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  value    Value to use for UMsg hint, Umsg hit is N wide bitvector
 *                      where N = number of Umsgs.
 * @returns             FPGA_OK on success.
 *                      FPGA_INVALID_PARAM if input parameter combination
 *                      is not valid.
 *                      FPGA_EXCEPTION if input parameter fpga handle is not
 *                      valid.
 */
fpga_result fpgaSetUmsgAttributes(fpga_handle handle,
				  uint64_t value);

/**
 * Trigger Umsg
 *
 * Writes a 64-bit value to trigger low-latency accelerator notification mechanism
 * (UMsgs).
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[in]  value    Value to use for UMsg
 * @returns             FPGA_OK on success.
 *                      FPGA_INVALID_PARAM if input parameter combination
 *                      is not valid.
 *                      FPGA_EXCEPTION if input parameter fpga handle is not
 *                      valid.
 */
fpga_result fpgaTriggerUmsg(fpga_handle handle, uint64_t value);

/**
 * Access UMsg memory directly
 *
 * This function will return a pointer to the memory allocated for low latency
 * accelerator notifications (UMsgs).
 *
 * @param[in]  handle   Handle to previously opened accelerator resource
 * @param[out] umsg_ptr Pointer to memory where a pointer to the virtual
 *                      address space will be returned
 * @returns             FPGA_OK on success.
 *                      FPGA_INVALID_PARAM if input parameter combination
 *                      is not valid.
 *                      FPGA_EXCEPTION if input parameter fpga handle is not
 *                      valid.
 *                      FPGA_NO_MEMORY if memory allocation fails or system
 *                      doesn't configure huge pages.
 */
fpga_result fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_UMSG_H__
