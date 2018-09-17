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
// Platform definitions for MPF.
//

`ifndef __CCI_MPF_PLATFORM__
`define __CCI_MPF_PLATFORM__

// Default platform: 3 physical channels (VL0, VH0, VH1)
`define MPF_PLATFORM_NUM_PHYSICAL_CHANNELS 3
`define MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL eVC_VL0


`ifdef MPF_PLATFORM_SKX
     //
     // Skylake Xeon+FPGA
     //

    localparam MPF_PLATFORM = "SKX";
    `define MPF_HOST_IFC_CCIP

    // CCI-P with support for WrPush_I
    `define MPF_HOST_IFC_CCIP_WRPUSH

`elsif MPF_PLATFORM_DCP_PCIE
     //
     // Discrete FPGA on a PCIe interface
     //

    localparam MPF_PLATFORM = "DCP_PCIE";
    `define MPF_HOST_IFC_CCIP

    // Use only VH0 (PCIe)
    `undef  MPF_PLATFORM_NUM_PHYSICAL_CHANNELS
    `define MPF_PLATFORM_NUM_PHYSICAL_CHANNELS 1
    `undef  MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL
    `define MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL eVC_VH0

`elsif MPF_PLATFORM_BDX
     //
     // Broadwell Xeon+FPGA
     //

    localparam MPF_PLATFORM = "BDX";
    `define MPF_HOST_IFC_CCIP

`elsif MPF_PLATFORM_OME
     //
     // OME2 and OME3 dual socket system development platforms
     //

    localparam MPF_PLATFORM = "OME";
    `define MPF_HOST_IFC_CCIS

    // Use only VL0 (QPI)
    `undef  MPF_PLATFORM_NUM_PHYSICAL_CHANNELS
    `define MPF_PLATFORM_NUM_PHYSICAL_CHANNELS 1
    `undef  MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL
    `define MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL eVC_VL0

`else

     ** ERROR: Select a valid MPF platform

`endif

`endif // __CCI_MPF_PLATFORM__
