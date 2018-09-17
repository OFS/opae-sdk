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
 * \file shim_latency_qos.h
 * \brief MPF Latency QoS shim
 */

#ifndef __FPGA_MPF_SHIM_LATENCY_QOS_H__
#define __FPGA_MPF_SHIM_LATENCY_QOS_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Set the Latency QoS configuration.
 *
 * Configuration is set using a single 64 bit register.  See
 * cci_mpf_shim_latency_qos.sv for the meaning of each field.
 *
 * @param[in]  mpf_handle       MPF handle initialized by mpfConnect().
 * @param[in]  config           Configuration bits.
 * @returns                     FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfLatencyQosSetConfig(
    mpf_handle_t mpf_handle,
    uint64_t config
);


#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_SHIM_LATENCY_QOS_H__
