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
 * \file shim_wro.h
 * \brief MPF WRO (write/read order) shim
 */

#ifndef __FPGA_MPF_SHIM_WRO_H__
#define __FPGA_MPF_SHIM_WRO_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * WRO statistics
 */
typedef struct
{
    // Cycles in which new read blocked due to conflict with old read
    uint64_t numConflictCyclesRR;
    // Cycles in which new read blocked due to conflict with old write
    uint64_t numConflictCyclesRW;
    // Cycles in which new write blocked due to conflict with old read
    uint64_t numConflictCyclesWR;
    // Cycles in which new write blocked due to conflict with old write
    uint64_t numConflictCyclesWW;
}
mpf_wro_stats;


/**
 * Return WRO statistics.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[out] stats       Statistics.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfWroGetStats(
    mpf_handle_t mpf_handle,
    mpf_wro_stats* stats
);


#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_SHIM_WRO_H__
