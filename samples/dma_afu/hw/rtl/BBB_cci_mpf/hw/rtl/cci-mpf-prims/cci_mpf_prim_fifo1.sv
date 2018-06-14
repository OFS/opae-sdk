//
// Copyright (c) 2015, Intel Corporation
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
// FIFO1 --
//   A FIFO with a single storage element allowing only deq or enq in a cycle
//   but not both.
//

module cci_mpf_prim_fifo1
  #(parameter N_DATA_BITS = 32)
    (input  logic clk,
     input  logic reset,

     input  logic [N_DATA_BITS-1 : 0] enq_data,
     input  logic                     enq_en,
     output logic                     notFull,

     output logic [N_DATA_BITS-1 : 0] first,
     input  logic                     deq_en,
     output logic                     notEmpty
     );
     
    logic [N_DATA_BITS-1 : 0] data;
    logic valid;

    assign first = data;
    assign notFull = ! valid;
    assign notEmpty = valid;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            valid <= 1'b0;
        end
        else if (deq_en)
        begin
            valid <= 1'b0;

            assert (notEmpty) else
                $fatal("cci_mpf_prim_fifo1: Can't DEQ when empty!");
            assert (! enq_en) else
                $fatal("cci_mpf_prim_fifo1: Can't DEQ and ENQ in same cycle!");
        end
        else if (enq_en)
        begin
            valid <= 1'b1;

            assert (notFull) else
                $fatal("cci_mpf_prim_fifo1: Can't ENQ when full!");
        end
    end

    // Write the data as long as the FIFO isn't full.  This leaves the
    // data path independent of control.  notFull/notEmpty will track
    // the control messages.
    always_ff @(posedge clk)
    begin
        if (notFull)
        begin
            data <= enq_data;
        end
    end

endmodule // cci_mpf_prim_fifo1
