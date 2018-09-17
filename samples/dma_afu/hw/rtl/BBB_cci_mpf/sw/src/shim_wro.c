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
 * \file shim_wro.c
 * \brief MPF WRO (write/read order) shim
 */

#include <string.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"


fpga_result __MPF_API__ mpfWroGetStats(
    mpf_handle_t mpf_handle,
    mpf_wro_stats* stats
)
{
    // Is the WRO feature present?
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_WRO))
    {
        memset(stats, -1, sizeof(mpf_wro_stats));
        return FPGA_NOT_SUPPORTED;
    }

    stats->numConflictCyclesRR = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_WRO, CCI_MPF_WRO_CSR_STAT_RR_CONFLICT, NULL);
    stats->numConflictCyclesRW = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_WRO, CCI_MPF_WRO_CSR_STAT_RW_CONFLICT, NULL);
    stats->numConflictCyclesWR = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_WRO, CCI_MPF_WRO_CSR_STAT_WR_CONFLICT, NULL);
    stats->numConflictCyclesWW = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_WRO, CCI_MPF_WRO_CSR_STAT_WW_CONFLICT, NULL);

    return FPGA_OK;
}
