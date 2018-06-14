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
 * \file shim_vc_map.c
 * \brief MPF VC Map (virtual channel mapper) shim
 */

#include <string.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"


fpga_result __MPF_API__ mpfVcMapSetMode(
    mpf_handle_t mpf_handle,
    bool enable_mapping,
    bool enable_dynamic_mapping,
    uint32_t sampling_window_radix
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;
    if (sampling_window_radix >= 16) return FPGA_INVALID_PARAM;

    fpga_result r;

    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    (enable_mapping ? 3 : 0) |
                    (enable_dynamic_mapping ? 4 : 0) |
                    (sampling_window_radix << 3) |
                    // Group A
                    ((uint64_t)1 << 63));

    return r;
}


fpga_result __MPF_API__ mpfVcMapSetMapOnlyReadsOrWrites(
    mpf_handle_t mpf_handle,
    bool map_writes
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;

    fpga_result r;

    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    (map_writes ? 2 : 1) |
                    // Group A
                    ((uint64_t)1 << 63));

    return r;
}


fpga_result __MPF_API__ vcmapDisable(
    mpf_handle_t mpf_handle
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;

    fpga_result r;

    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    0 |
                    // Group A
                    ((uint64_t)1 << 63));

    return r;
}


fpga_result __MPF_API__ mpfVcMapSetMapAll(
    mpf_handle_t mpf_handle,
    bool map_all_requests
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;

    fpga_result r;

    uint64_t map_all = (map_all_requests ? (1 << 7) : 0);
    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    map_all |
                    // Group B
                    ((uint64_t)1 << 62));

    return r;
}


fpga_result __MPF_API__ mpfVcMapSetFixedMapping(
    mpf_handle_t mpf_handle,
    bool user_specified,
    uint32_t ratio
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;

    if (ratio > 64) return FPGA_INVALID_PARAM;

    fpga_result r;

    uint64_t specified = (user_specified ? (1 << 8) : 0);
    if (! user_specified)
    {
        ratio = 0;
    }

    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    specified | (ratio << 9) |
                    // Group C
                    ((uint64_t)1 << 61));

    return r;
}


fpga_result __MPF_API__ mpfVcMapSetLowTrafficThreshold(
    mpf_handle_t mpf_handle,
    uint32_t t
)
{
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP)) return FPGA_NOT_SUPPORTED;

    // The threshold must fit in the size and must be some 2^n-1 so only
    // a contiguous set of low bits are set.  This way it can be used as
    // a mask in the hardware.
    if (! ((t <= 0xffff) && (((t + 1) & t) == 0))) return FPGA_INVALID_PARAM;

    fpga_result r;

    r = mpfWriteCsr(mpf_handle,
                    CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_CTRL_REG,
                    (t << 16) |
                    // Group D
                    ((uint64_t)1 << 60));

    return r;
}


fpga_result __MPF_API__ mpfVcMapGetStats(
    mpf_handle_t mpf_handle,
    mpf_vc_map_stats* stats
)
{
    // Is the VC Map feature present?
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP))
    {
        memset(stats, -1, sizeof(mpf_vc_map_stats));
        return FPGA_NOT_SUPPORTED;
    }

    stats->numMappingChanges = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_STAT_NUM_MAPPING_CHANGES, NULL);

    return FPGA_OK;
}


uint64_t __MPF_API__ mpfVcMapGetMappingHistory(
    mpf_handle_t mpf_handle
)
{
    // Is the VC Map feature present?
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VC_MAP))
    {
        return ~(uint64_t)0;
    }

    return mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VC_MAP, CCI_MPF_VC_MAP_CSR_STAT_HISTORY, NULL);
}
