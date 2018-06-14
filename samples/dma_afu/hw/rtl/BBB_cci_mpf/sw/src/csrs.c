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

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"


fpga_result __MPF_API__ mpfWriteCsr(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx,
    uint64_t shim_csr_offset,
    uint64_t value
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    if (! mpfShimPresent(mpf_handle, mpf_shim_idx))
    {
        return FPGA_NOT_FOUND;
    }

    return fpgaWriteMMIO64(_mpf_handle->handle, _mpf_handle->mmio_num,
                           shim_csr_offset + _mpf_handle->shim_mmio_base[mpf_shim_idx],
                           value);
}


uint64_t __MPF_API__ mpfReadCsr(
    mpf_handle_t mpf_handle,
    t_cci_mpf_shim_idx mpf_shim_idx,
    uint64_t shim_csr_offset,
    fpga_result* result
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;
    fpga_result r = FPGA_OK;
    uint64_t value = (uint64_t) -1;

    if (! mpfShimPresent(mpf_handle, mpf_shim_idx))
    {
        r = FPGA_NOT_FOUND;
    }
    else
    {
        r = fpgaReadMMIO64(_mpf_handle->handle, _mpf_handle->mmio_num,
                           shim_csr_offset + _mpf_handle->shim_mmio_base[mpf_shim_idx],
                           &value);
    }

    if (NULL != result)
    {
        *result = r;
    }

    return value;
}
