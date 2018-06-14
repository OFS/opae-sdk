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
// Random replacement algorithm.  The interface is the same as the LRU
// replacement algorithm, allowing either to be chosen.
//

module cci_mpf_prim_repl_random
  #(
    parameter N_WAYS = 4,
    parameter N_ENTRIES = 1024
    )
   (
    input  logic clk,
    input  logic reset,

    // The entire module is ready after initialization.  Once ready, the
    // lookup function is always available.  The reference functions appear
    // always ready but make no guarantees that all references will be
    // recorded.  Contention on internal memory may require that some
    // references be ignored.
    output logic rdy,

    // Look up LRU for index
    input  logic [$clog2(N_ENTRIES)-1 : 0] lookupIdx,
    input  logic lookupEn,

    // Returns LRU one cycle after lookupEn.  The response is returned
    // in two forms with the same answer.  One is a one-hot vector.  The
    // other is an index.
    output logic [N_WAYS-1 : 0] lookupVecRsp,
    output logic [$clog2(N_WAYS)-1 : 0] lookupRsp,
    output logic lookupRspRdy,

    // Port 0 update.
    input  logic [$clog2(N_ENTRIES)-1 : 0] refIdx0,
    // Update refWayVec will be ORed into the current state
    input  logic [N_WAYS-1 : 0] refWayVec0,
    input  logic refEn0,

    // Port 1 update
    input  logic [$clog2(N_ENTRIES)-1 : 0] refIdx1,
    input  logic [N_WAYS-1 : 0] refWayVec1,
    input  logic refEn1
    );

    typedef logic [$clog2(N_WAYS)-1 : 0] t_way_idx;
    typedef logic [N_WAYS-1 : 0] t_way_vec;

    // Random number generator
    logic [11:0] random;
    cci_mpf_prim_lfsr12 lfsr
       (
        .clk,
        .reset,
        .en(lookupEn),
        .value(random)
        );


    assign rdy = 1'b1;

    // Pick an index randomly
    t_way_idx idx;
    assign idx = t_way_idx'(random);

    // Convert index to one-hot vector
    t_way_vec way_vec;
    always_comb
    begin
        for (int i = 0; i < N_WAYS; i = i + 1)
        begin
            way_vec[i] = (t_way_idx'(i) == idx);
        end
    end


    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            lookupRspRdy <= 1'b0;
        end
        else
        begin
            lookupVecRsp <= way_vec;
            lookupRsp <= idx;
            lookupRspRdy <= lookupEn;
        end
    end

endmodule // cci_mpf_prim_repl_random

