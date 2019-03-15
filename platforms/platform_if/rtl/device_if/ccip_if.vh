//
// Copyright (c) 2019, Intel Corporation
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
// Wrapper module to import and configure the CCI-P interface.
//

`ifndef __CCIP_IF_VH__
`define __CCIP_IF_VH__

//
// These macros indicate the presence of additions to features that changed
// after the initial release, allowing RTL to work with multiple versions.
//

// Speculative loads: eREQ_RDLSPEC and the error flag in t_ccip_c0_RspMemHdr
`define CCIP_RDLSPEC_AVAIL 1


import ccip_if_pkg::*;


//
// Because CCI-P's clocks are passed as separate wires to ccip_std_afu
// we can't simply change the primary clock for CCI-P traffic.  Here
// we guarantee that the macro `PLATFORM_PARAM_CCI_P_CLOCK matches the
// CCI-P clock coming out of the Platform Interface Manager.  By
// default it is pClk.  It will only change when an AFU's JSON database
// requests CCI-P on a different clock.
//
`ifndef PLATFORM_PARAM_CCI_P_CLOCK
    `define PLATFORM_PARAM_CCI_P_CLOCK pClk
`elsif PLATFORM_PARAM_CCI_P_CLOCK_IS_DEFAULT
    `undef PLATFORM_PARAM_CCI_P_CLOCK
    `define PLATFORM_PARAM_CCI_P_CLOCK pClk
`endif

// The standard reset signal is moved to the `PLATFORM_PARAM_CCI_P_CLOCK
// domain.  Provide a macro for reset for symmetry.
`define PLATFORM_PARAM_CCI_P_RESET pck_cp2af_softReset

`endif // __CCIP_IF_VH__
