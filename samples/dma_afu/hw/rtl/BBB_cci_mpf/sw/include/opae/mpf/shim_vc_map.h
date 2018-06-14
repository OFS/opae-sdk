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
 * \file shim_vc_map.h
 * \brief MPF VC Map (virtual channel mapper) shim
 */

#ifndef __FPGA_MPF_SHIM_VC_MAP_H__
#define __FPGA_MPF_SHIM_VC_MAP_H__

#include <opae/mpf/csrs.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Set the VC Map mode
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  enable_mapping   Turns on eVC_VA to physical channel mapping.
 * @param[in]  enable_dynamic_mapping Turns on automatic tuning of the channel
 *                              ratios based on traffic. (Ignored when
 *                              enable_mapping is false.)
 * @param[in]  sampling_window_radix  Determines the sizes of sampling windows
 *                              for dynamic mapping and, consequently, controls
 *                              the frequency at which dynamic changes may
 *                              occur.  Dynamic changes are expensive
 *                              since a write fence must be emitted to
 *                              synchronize traffic.  Passing 0 picks the
 *                              default value.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapSetMode(
    mpf_handle_t mpf_handle,
    bool enable_mapping,
    bool enable_dynamic_mapping,
    uint32_t sampling_window_radix
);


/**
 * Map requests either for reads or writes, but not both.
 *
 * This mode does not accomplish the usual VC Map goals, since mapping will
 * not be consistent by address for both reads and writes.  In combination
 * with mpfVcMapSetFixedMapping(), this mode is useful for some bandwidth
 * allocation scenarios, most likely for limiting writes to a subset of
 * the I/O channels.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  map_writes       When true, writes are mapped and reads are not.
 *                              When false, reads are mapped and writes are not.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapSetMapOnlyReadsOrWrites(
    mpf_handle_t mpf_handle,
    bool map_writes
);


/**
 * Disable mapping
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ vcmapDisable(
    mpf_handle_t mpf_handle
);


/**
 * Map all requests or just those on eVC_VA?
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  map_all_requests When false (default), only incoming eVC_VA
 *                              requests are mapped.  When true, all
 *                              incoming requests are remapped.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapSetMapAll(
    mpf_handle_t mpf_handle,
    bool map_all_requests
);


/**
 * Set a fixed mapping ratio.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  user_specified   When true, "ratio" is used as the new ratio.  When
 *                              false, a platform-specific fixed mapping is used.
 * @param[in]  ratio            Fixed mapping where VL0 gets ratio 64ths of the
 *                              traffic.  Ratio is ignored when user_specified
 *                              is false.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapSetFixedMapping(
    mpf_handle_t mpf_handle,
    bool user_specified,
    uint32_t ratio
);


/**
 * Low latency control.
 *
 * This mode is experimental and will likely not improve performance.
 * Set a traffic threshold below which all traffic will be mapped to
 * the low-latency VL0.  The treshold is the sum of read and write requests
 * in a sampling window.  The sampling window can be set in mpfVcMapSetMode()
 * as sampling_window_radix, defaulting to 10 (1K cycles).
 * The threshold must be some 2^n-1 so it can be used as a mask.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  t                Threshold.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapSetLowTrafficThreshold(
    mpf_handle_t mpf_handle,
    uint32_t t
);


/**
 * VC Map statistics
 */
typedef struct
{
    // Number of times the mapping function has changed
    uint64_t numMappingChanges;
}
mpf_vc_map_stats;


/**
 * Return VC Map statistics.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[out] stats       Statistics.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVcMapGetStats(
    mpf_handle_t mpf_handle,
    mpf_vc_map_stats* stats
);


/*
 * Get the VC Map history.
 *
 * Mapping history is a state vector.  The vector is an array of 8
 * bit entries with the lowest 8 bits corresponding to the most recent
 * state.  The 8 bit values each are the fraction of references, in 64ths,
 * to direct to VL0.  The remaining references are distributed evenly between
 * the PCIe channels.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @returns                History vector.
 */
uint64_t __MPF_API__ mpfVcMapGetMappingHistory(
    mpf_handle_t mpf_handle
);


#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_SHIM_VC_MAP_H__
