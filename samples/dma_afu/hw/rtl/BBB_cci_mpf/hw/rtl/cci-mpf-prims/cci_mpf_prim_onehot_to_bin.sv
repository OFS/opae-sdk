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
// Decode one hot encoding to a binary value.  Taken from the Altera cookbook.
//

module cci_mpf_prim_onehot_to_bin
  #(
    parameter ONEHOT_WIDTH = 16,
    parameter BIN_WIDTH = $clog2(ONEHOT_WIDTH)
    )
   (
    input  [ONEHOT_WIDTH-1:0] onehot,
    output [BIN_WIDTH-1:0] bin
    );

    genvar i, j;
    generate
        for (j = 0; j < BIN_WIDTH; j = j + 1)
        begin : jl
            wire [ONEHOT_WIDTH-1:0] tmp_mask;

            for (i = 0; i < ONEHOT_WIDTH; i = i + 1)
            begin : il
                assign tmp_mask[i] = i[j];
            end

            assign bin[j] = |(tmp_mask & onehot);
        end
    endgenerate

endmodule // cci_mpf_prim_onehot_to_bin
