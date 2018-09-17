//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * \file csrs.h
 * \brief Read and write MPF CSRs
 */

#ifndef __FPGA_MPF_CSRS_H__
#define __FPGA_MPF_CSRS_H__

#include <opae/mpf/cci_mpf_csrs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write to a CSR in an MPF shim
 *
 * All MPF CSRs are 64 bits.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  mpf_shim_idx     MPF shim to write.
 * @param[in]  shim_csr_offset  Offset within the shim's CSR space.  Offsets in
 *                              MPF shim CSR spaces are defined in cci_mpf_csrs.h.
 * @param[in]  value            Data to write to CSR.
 * @returns                     FPGA_OK on success and FPGA_NOT_FOUND when
 *                              the requested shim is not present.
 */
fpga_result __MPF_API__ mpfWriteCsr(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx,
    uint64_t shim_csr_offset,
    uint64_t value
);


/**
 * Read from a CSR in an MPF shim
 *
 * All MPF CSRs are 64 bits.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  mpf_shim_idx     MPF shim to write.
 * @param[in]  shim_csr_offset  Offset within the shim's CSR space.  Offsets in
 *                              MPF shim CSR spaces are defined in cci_mpf_csrs.h.
 * @param[in]  result           Pointer to a location where the return status
 *                              may be stored (FPGA_OK on success).  If result is
 *                              NULL no status is stored.
 * @returns                     CSR value.
 */
uint64_t __MPF_API__ mpfReadCsr(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx,
    uint64_t shim_csr_offset,
    fpga_result* result
);

#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_CSRS_H__
