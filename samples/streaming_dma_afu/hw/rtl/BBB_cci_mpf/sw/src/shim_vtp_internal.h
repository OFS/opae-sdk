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
 * \file mpf_shim_vtp_internal.h
 * \brief Internal functions and data structures for managing VTP.
 */

#ifndef __FPGA_MPF_SHIM_VTP_INTERNAL_H__
#define __FPGA_MPF_SHIM_VTP_INTERNAL_H__

#include "shim_vtp_pt.h"


/**
 * Initialize VTP.
 *
 * This function should be called automatically as a side effect of
 * establishing a connection to MPF.
 *
 * @param[in]  _mpf_handle Internal handle to MPF state.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpInit(
    _mpf_handle_p _mpf_handle
);


/**
 * Terminate VTP.
 *
 * This function should be called automatically as a side effect of
 * disconnecting MPF.
 *
 * @param[in]  _mpf_handle Internal handle to MPF state.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpTerm(
    _mpf_handle_p _mpf_handle
);


/**
 * VTP persistent state.  An instance of this struct is stored in the
 * MPF handle.
 */
typedef struct
{
    // VTP page table state
    mpf_vtp_pt* pt;

    // VTP mutex (one allocation at a time)
    mpf_os_mutex_handle alloc_mutex;

    // Maximum requested page size
    mpf_vtp_page_size max_physical_page_size;

    // Does libfpga support FPGA_PREALLOCATED?  The old AAL compatibility
    // version does not.
    bool use_fpga_buf_preallocated;

    // Is VTP available in the FPGA?
    bool is_available;
}
mpf_vtp_state;

#endif // __FPGA_MPF_SHIM_VTP_INTERNAL_H__
