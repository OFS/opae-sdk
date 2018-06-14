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

`ifndef __CCI_MPF_SHIM_WRO_VH__
`define __CCI_MPF_SHIM_WRO_VH__

//
// The read/write ordering filter operates on addresses of the maximum
// aligned multi-line request size.  Conflicts between reads and writes
// are managed at this level:  any request within a conflicting multi-line
// window, independent of size, is considered a conflict.  Read/read conflicts
// are resovled by using a counting filter so that multiple requests to
// the same line may be live simultaneously.  Write/write conflicts from
// smaller requests within the same address group are resolved using a
// mask indicating which lines within the group have active writes.
//

localparam N_WRO_WR_FILTER_MASK_BITS = 4;
typedef logic [N_WRO_WR_FILTER_MASK_BITS-1 : 0] t_cci_mpf_wro_line_mask;

//
// Map a write request's address and size to a wrFilter mask.
//
function automatic t_cci_mpf_wro_line_mask wroWrFilterLineMask(
    input t_cci_mpf_c1_ReqMemHdr h
    );

    t_cci_mpf_wro_line_mask mask;

    // LUT from cl_len and low two bits of address to a mask
    casex ({ h.base.cl_len, h.base.address[1:0] })
        // 4 beat request
        4'b11xx: mask = 4'b1111;
        // 2 beat request, low half of 4 beat aligned group
        4'b010x: mask = 4'b0011;
        // 2 beat request, high half of 4 beat aligned group
        4'b011x: mask = 4'b1100;
        // 1 beat requests
        4'b0000: mask = 4'b0001;
        4'b0001: mask = 4'b0010;
        4'b0010: mask = 4'b0100;
        4'b0011: mask = 4'b1000;

        // This is illegal -- there are no 3 beat references
        default: mask = 4'b0000;
    endcase

    return mask;
endfunction

`endif // __CCI_MPF_SHIM_WRO_VH__
