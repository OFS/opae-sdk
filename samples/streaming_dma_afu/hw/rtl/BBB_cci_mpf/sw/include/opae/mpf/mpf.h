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
 * \file mpf.h
 * \brief CCI Memory Properties Factory API
 */

/**
 * @mainpage Memory Properties Factory BBB
 * @section mpf_c_api C API for MPF
 * @subsection mpf_c_key_functions Key functions
 *
 * - Initialize MPF and connect to an AFU with mpfConnect()
 * - Test whether a shim is instantiated in a connected AFU with mpfShimPresent()
 * - Allocate virtually referenced buffers with mpfVtpBufferAllocate()
 * - Control channel mapping on AFUs with VC Map enabled using functions
 *   in shim_vc_map.h
 *
 * - Many shims have functions that read statistics:
 *   - VC Map (virtual channel mapper): mpfVcMapGetStats()
 *   - VTP (virtual to physical): mpfVtpGetStats()
 *   - WRO (write/read order): mpfWroGetStats()
 *   - PWrite (partial write): mpfPwriteGetStats()
 */

#ifndef __FPGA_MPF_MPF_H__
#define __FPGA_MPF_MPF_H__

#include <stdbool.h>
#include <stdint.h>

#include <opae/fpga.h>
#include <opae/mpf/types.h>
#include <opae/mpf/connect.h>
#include <opae/mpf/csrs.h>
#include <opae/mpf/shim_latency_qos.h>
#include <opae/mpf/shim_pwrite.h>
#include <opae/mpf/shim_vc_map.h>
#include <opae/mpf/shim_vtp.h>
#include <opae/mpf/shim_wro.h>

#endif // __FPGA_MPF_MPF_H__
