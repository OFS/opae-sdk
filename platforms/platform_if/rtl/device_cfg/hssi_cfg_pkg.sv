//
// Copyright (c) 2018, Intel Corporation
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

`include "platform_afu_top_config.vh"

`ifdef PLATFORM_PROVIDES_HSSI

//
// Platform-specific HSSI configuration.
//
// It is assumed that this package will NOT be wildcard imported.  The
// package name serves as a prefix instead of making all symbols inside
// the package long.
//

package hssi_cfg_pkg;

    parameter VERSION_NUMBER = 1;

    typedef enum
    {
        HSSI_BW_10G = 1,
        HSSI_BW_4x10G = 2,
        HSSI_BW_40G = 4,
        HSSI_BW_2x40G = 8,
        HSSI_BW_100G = 16
    }
    bw_mode_enum;

    // Which modes are supported? (Bit mask)
    parameter int BW_MODES = `PLATFORM_PARAM_HSSI_BANDWIDTH_OFFERED;

    // Characteristics of the raw HSSI interface
    parameter int RAW_NUM_LANES = `PLATFORM_PARAM_HSSI_RAW_NUM_LANES;
    parameter int RAW_LANE_WIDTH = `PLATFORM_PARAM_HSSI_RAW_LANE_WIDTH;

endpackage // hssi_cfg_pkg

`endif // PLATFORM_PROVIDES_HSSI
