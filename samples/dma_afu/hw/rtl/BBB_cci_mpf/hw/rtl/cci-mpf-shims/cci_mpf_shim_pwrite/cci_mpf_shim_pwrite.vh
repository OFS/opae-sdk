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

`ifndef __CCI_MPF_SHIM_PWRITE_VH__
`define __CCI_MPF_SHIM_PWRITE_VH__

//
// Interface between edge modules and the partial write shim.
//
interface cci_mpf_shim_pwrite_if
  #(
    parameter N_WRITE_HEAP_ENTRIES = 0
    );

    //
    // Forward write masks from AFU to the partial write shim, bypassing
    // the MPF pipeline.
    //
    logic wen;
    logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] widx;
    t_cci_clNum wclnum;
    t_cci_mpf_c1_PartialWriteHdr wpartial;

    //
    // Update write data with existing state for the unmodified portion.
    //
    logic upd_en;
    logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] upd_idx;
    t_cci_clNum upd_clNum;
    t_cci_clData upd_data;
    t_cci_mpf_clDataByteMask upd_mask;

    modport pwrite
       (
        input  wen,
        input  widx,
        input  wclnum,
        input  wpartial,

        output upd_en,
        output upd_idx,
        output upd_clNum,
        output upd_data,
        output upd_mask
        );
        
    modport pwrite_edge_afu
       (
        output wen,
        output widx,
        output wclnum,
        output wpartial
        );

    modport pwrite_edge_fiu
       (
        input  upd_en,
        input  upd_idx,
        input  upd_clNum,
        input  upd_data,
        input  upd_mask
        );

endinterface

`endif // __CCI_MPF_SHIM_PWRITE_VH__
