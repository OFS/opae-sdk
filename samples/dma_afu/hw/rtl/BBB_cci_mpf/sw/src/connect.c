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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"


static void _mpf_find_features(_mpf_handle_p _mpf_handle);


fpga_result __MPF_API__ mpfConnect(
    fpga_handle handle,
    uint32_t mmio_num,
    uint64_t mmio_offset,
    mpf_handle_t *mpf_handle,
    uint32_t mpf_flags
)
{
    fpga_result r;
    _mpf_handle_p _mpf_handle;

    _mpf_handle = malloc(sizeof(struct _mpf_handle_t));
    if (NULL == _mpf_handle)
    {
        return FPGA_NO_MEMORY;
    }

    memset(_mpf_handle, 0, sizeof(*_mpf_handle));

    _mpf_handle->handle = handle;
    _mpf_handle->mmio_num = mmio_num;
    _mpf_handle->mmio_offset = mmio_offset;
    _mpf_handle->dbg_mode = (0 != (mpf_flags & MPF_FLAG_DEBUG));

    // Debugging can also be enabled by defining an environment
    // variable named MPF_ENABLE_DEBUG (any value).
    if (getenv("MPF_ENABLE_DEBUG"))
    {
        _mpf_handle->dbg_mode = true;
    }

    // set handle return value
    *mpf_handle = (void *)_mpf_handle;

    _mpf_find_features(_mpf_handle);

    //
    // Initialize features that require it.
    //
    if (mpfShimPresent(_mpf_handle, CCI_MPF_SHIM_VTP))
    {
        r = mpfVtpInit(_mpf_handle);
        if (FPGA_OK != r) return r;
    }

    return FPGA_OK;
}


fpga_result __MPF_API__ mpfDisconnect(
    mpf_handle_t mpf_handle
)
{
    fpga_result r;
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    //
    // Terminate features that require it.
    //
    if (mpfShimPresent(_mpf_handle, CCI_MPF_SHIM_VTP))
    {
        r = mpfVtpTerm(_mpf_handle);
        if (FPGA_OK != r) return r;
    }

    free(_mpf_handle);

    return FPGA_OK;
}


bool __MPF_API__ mpfShimPresent(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;
    return (0 != _mpf_handle->shim_mmio_base[mpf_shim_idx]);
}


// ========================================================================
//
//   Internal code.
//
// ========================================================================

//
// UUIDs of shims, broken down into 64 bit halves
//

#define MPF_SHIM_VTP_UUID_L           0xa70545727f501901
#define MPF_SHIM_VTP_UUID_H           0xc8a2982fff9642bf

#define MPF_SHIM_RSP_ORDER_UUID_L     0xb383c70ace57bfe4
#define MPF_SHIM_RSP_ORDER_UUID_H     0x4c9c96f465ba4dd8

#define MPF_SHIM_VC_MAP_UUID_L        0xb8f93b76e3dd4e74
#define MPF_SHIM_VC_MAP_UUID_H        0x5046c86fba484856

#define MPF_SHIM_LATENCY_QOS_UUID_L   0x9412a4cf1a999c49
#define MPF_SHIM_LATENCY_QOS_UUID_H   0xb35138f6ea394603

#define MPF_SHIM_WRO_UUID_L           0xa47e0681b4207a6d
#define MPF_SHIM_WRO_UUID_H           0x56b06b489dd74004

#define MPF_SHIM_PWRITE_UUID_L        0xa63675b19a0b4f5c
#define MPF_SHIM_PWRITE_UUID_H        0x9bdbbcaf2c5a4d17


// Shim to UUID mapping
typedef struct
{
    char *name;
    uint64_t uuid[2];
}
mpf_shim_info_t;

const static mpf_shim_info_t mpf_shim_info[CCI_MPF_SHIM_LAST_IDX] =
{
    // Initialization order must match t_cci_mpf_shim_idx order!

    { .name = "VTP", .uuid = { MPF_SHIM_VTP_UUID_L, MPF_SHIM_VTP_UUID_H } },
    { .name = "RSP Order", .uuid = { MPF_SHIM_RSP_ORDER_UUID_L, MPF_SHIM_RSP_ORDER_UUID_H } },
    { .name = "VC Map", .uuid = { MPF_SHIM_VC_MAP_UUID_L, MPF_SHIM_VC_MAP_UUID_H } },
    { .name = "Latency QoS", .uuid = { MPF_SHIM_LATENCY_QOS_UUID_L, MPF_SHIM_LATENCY_QOS_UUID_H } },
    { .name = "WRO", .uuid = { MPF_SHIM_WRO_UUID_L, MPF_SHIM_WRO_UUID_H } },
    { .name = "PWRITE", .uuid = { MPF_SHIM_PWRITE_UUID_L, MPF_SHIM_PWRITE_UUID_H } }
};


// End of feature list?
static bool _mpf_feature_eol(
    uint64_t dfh
)
{
    return ((dfh >> 40) & 1) == 1;
}

// Feature type is BBB?
static bool _mpf_feature_is_bbb(
    uint64_t dfh
)
{
    // BBB is type 2
    return ((dfh >> 60) & 0xf) == 2;
}

// Offset to the next feature header
static uint64_t _mpf_feature_next(
    uint64_t dfh
)
{
    return (dfh >> 16) & 0xffffff;
}

//
// Walk the feature chain and discover all MPF features.
//
static void _mpf_find_features(
    _mpf_handle_p _mpf_handle
)
{
    uint64_t offset = _mpf_handle->mmio_offset;
    uint64_t dfh = 0;
    bool eol;

    do
    {
        // Read the next feature header
        fpgaReadMMIO64(_mpf_handle->handle, _mpf_handle->mmio_num, offset, &dfh);

        // Read the current feature's UUID
        uint64_t feature_uuid[2];
        fpgaReadMMIO64(_mpf_handle->handle, _mpf_handle->mmio_num, offset + 8,
                       &feature_uuid[0]);
        fpgaReadMMIO64(_mpf_handle->handle, _mpf_handle->mmio_num, offset + 16,
                       &feature_uuid[1]);

        if (_mpf_feature_is_bbb(dfh))
        {
            // Look for MPF features
            for (int i = 0; i < CCI_MPF_SHIM_LAST_IDX; i++)
            {
                if ((feature_uuid[0] == mpf_shim_info[i].uuid[0]) &&
                    (feature_uuid[1] == mpf_shim_info[i].uuid[1]))
                {
                    // Found one.  Record it in the internal descriptor.
                    _mpf_handle->shim_mmio_base[i] = offset;

                    if (_mpf_handle->dbg_mode)
                    {
                        MPF_FPGA_MSG("Found MPF shim UUID [0x%016" PRIx64 ", 0x%016" PRIx64 "] (%s) at offset %" PRId64,
                                     feature_uuid[0], feature_uuid[1], mpf_shim_info[i].name, offset);
                    }
                }
            }
        }

        // End of the list?
        eol = _mpf_feature_eol(dfh);

        // Move to the next feature header
        offset = offset + _mpf_feature_next(dfh);
    }
    while (! eol);
}
