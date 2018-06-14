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

`ifndef __CCI_MPF_SHIM_EDGE_VH__
`define __CCI_MPF_SHIM_EDGE_VH__

//
// Interface between an AFU edge and the FIU edge.
//
interface cci_mpf_shim_edge_if
  #(
    parameter N_WRITE_HEAP_ENTRIES = 0
    );

    //
    // Forward write data from AFU to FIU, bypassing the MPF pipeline.
    //
    logic wen;
    logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] widx;
    t_cci_clNum wclnum;
    t_cci_clData wdata;
    logic wAlmFull;

    //
    // Free write data heap entries.
    //
    logic free;
    logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] freeidx;


    modport edge_afu
       (
        output wen,
        output widx,
        output wclnum,
        output wdata,
        input  wAlmFull,

        input  free,
        input  freeidx
        );

    modport edge_fiu
       (
        input  wen,
        input  widx,
        input  wclnum,
        input  wdata,
        output wAlmFull,

        output free,
        output freeidx
        );

endinterface

`endif // __CCI_MPF_SHIM_EDGE_VH__
