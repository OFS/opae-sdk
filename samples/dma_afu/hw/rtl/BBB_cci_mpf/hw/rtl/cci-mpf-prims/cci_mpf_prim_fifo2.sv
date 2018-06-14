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
// FIFO --
//   A FIFO with two storage elements supporting full pipelining.
//

module cci_mpf_prim_fifo2
  #(
    parameter N_DATA_BITS = 32
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [N_DATA_BITS-1 : 0] enq_data,
    input  logic enq_en,
    output logic notFull,

    output logic [N_DATA_BITS-1 : 0] first,
    input  logic deq_en,
    output logic notEmpty
    );

    cci_mpf_prim_fifo2_peek
      #(
        .N_DATA_BITS(N_DATA_BITS)
        )
      f
       (
        .clk,
        .reset,

        .enq_data,
        .enq_en,
        .notFull,

        .first,
        .deq_en,
        .notEmpty,

        .peek_valid(),
        .peek_value()
        );

endmodule // cci_mpf_prim_fifo2


//
// FIFO2 with the internal data state exposed
//
module cci_mpf_prim_fifo2_peek
  #(
    parameter N_DATA_BITS = 32
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [N_DATA_BITS-1 : 0] enq_data,
    input  logic enq_en,
    output logic notFull,

    output logic [N_DATA_BITS-1 : 0] first,
    input  logic deq_en,
    output logic notEmpty,

    output logic [1:0] peek_valid,
    output logic [1:0][N_DATA_BITS-1 : 0] peek_value
    );

    logic [1:0] valid;
    logic [N_DATA_BITS-1 : 0] data[0 : 1];

    assign notFull = ~valid[0];
    assign first = data[1];
    assign notEmpty = valid[1];

    always_ff @(posedge clk)
    begin
        // Target output slot if it is empty or dequeued this cycle
        if (! valid[1] || deq_en)
        begin
            // Output slot will be valid next cycle if slot 0 has data
            // or a new value is enqueued this cycle.  Both can't be
            // true, so valid[0] will definitely be 0 next cycle.
            valid[0] <= 1'b0;
            valid[1] <= valid[0] || enq_en;
            data[1] <= valid[0] ? data[0] : enq_data;
        end
        else
        begin
            // Target of enq is slot 0.  It will be valid if there was
            // data and it didn't shift or if new data is enqueued.
            valid[0] <= (valid[0] && ! deq_en) || enq_en;
        end

        // Speculatively store data if the slot is empty. If the slot
        // was valid then no enqueue will be allowed.
        if (! valid[0])
        begin
            data[0] <= enq_data;
        end

        if (reset)
        begin
            valid <= 2'b0;
        end
        else
        begin
            assert (! (enq_en && valid[0])) else
                $fatal("cci_mpf_prim_fifo2: ENQ to full FIFO!");
            assert (! (deq_en && ! valid[1])) else
                $fatal("cci_mpf_prim_fifo2: DEQ from empty FIFO!");
        end
    end

    assign peek_valid = valid;
    assign peek_value[0] = data[0];
    assign peek_value[1] = data[1];

endmodule // cci_mpf_prim_fifo2_peek
