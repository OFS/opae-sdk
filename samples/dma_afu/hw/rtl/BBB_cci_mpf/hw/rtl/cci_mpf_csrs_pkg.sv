//
// Copyright (c) 2016, Intel Corporation
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

//
// Description of the configuration/status register address space in MPF.
//

package cci_mpf_csrs_pkg;

// Include the enumerations describing CSR offsets shared with C here.
// There is no `define protecting the header file from being included once
// since there isn't common preprocessor syntax.
`ifndef CCI_MPF_CSRS_H_INC
`define CCI_MPF_CSRS_H_INC
`include "cci_mpf_csrs.h"
`endif

    // MPF needs MMIO address space to hold its feature lists and CSRs.
    // AFUs that instantiate MPF must allocate at least this much space.
    // AFUs will pass the base MMIO address of the allocated space to
    // the MPF wrapper.  See cci_mpf.sv.

    // Assume all shims are allocated.  Size is in bytes.
    parameter CCI_MPF_MMIO_SIZE = CCI_MPF_VTP_CSR_SIZE +
                                  CCI_MPF_RSP_ORDER_CSR_SIZE +
                                  CCI_MPF_VC_MAP_CSR_SIZE +
                                  CCI_MPF_LATENCY_QOS_CSR_SIZE +
                                  CCI_MPF_WRO_CSR_SIZE +
                                  CCI_MPF_PWRITE_CSR_SIZE;

    // CCI_MPF_VTP_CSR_MODE -- see cci_mpf_csrs.h
    typedef struct packed {
        logic inval_translation_cache;
        logic enabled;
    } t_cci_mpf_vtp_csr_mode;

endpackage // cci_mpf_csrs_pkg
