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
 * \file connect.h
 * \brief Connect to MPF shims on an FPGA
 */

#ifndef __FPGA_MPF_CONNECT_H__
#define __FPGA_MPF_CONNECT_H__

#include <opae/mpf/csrs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Establish a connection to MPF
 *
 * Scans the feature chain, looking for MPF shims.  MPF has a debug mode
 * in which details such as VTP mapping are printed to stdout.  Debug mode
 * is enabled either by passing MPF_FLAG_DEBUG to mpf_flags or by defining
 * an environment variable at run time named MPF_ENABLE_DEBUG.
 *
 * @param[in]  handle       Handle returned by fpgaOpen().
 * @param[in]  mmio_num     Number of MMIO space to access.
 * @param[in]  mmio_offset  Byte offset in MMIO space at which scanning for MPF
 *                          features should begin. This is typically 0.
 * @param[out] mpf_handle   Pointer to preallocated memory into which MPF
 *                          will generate a handle describing the instantiated
 *                          MPF shims.
 * @param[in] mpf_flags     Bitwise OR of flags to control MPF behavior, such as debugging
 *                          output.
 * @returns                 FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfConnect(
    fpga_handle handle,
    uint32_t mmio_num,
    uint64_t mmio_offset,
    mpf_handle_t *mpf_handle,
    uint32_t mpf_flags
);


/**
 * Disconnect from MPF
 *
 * Close an existing MPF connection previously established with mpfConnect().
 *
 * @param[in]  mpf_handle   Handle to MPF instance.
 * @returns                 FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfDisconnect(
    mpf_handle_t mpf_handle
);


/**
 * Test whether a given MPF shim is present
 *
 * MPF shims are detected by walking the feature list stored in MMIO space
 * when mpfConnect() is called.
 *
 * @param[in]  mpf_handle   Handle to MPF instance.
 * @param[in]  mpf_shim_idx Requested MPF shim.
 * @returns                 true if shim is present.
 */
bool __MPF_API__ mpfShimPresent(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx
);


#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_CONNECT_H__
