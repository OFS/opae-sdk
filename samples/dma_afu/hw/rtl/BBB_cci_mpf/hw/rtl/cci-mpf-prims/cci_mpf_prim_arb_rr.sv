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
// Round-robin arbiter, derived from the Altera Advanced Synthesis Cookbook.
//

module cci_mpf_prim_arb_rr
  #(
    parameter NUM_CLIENTS = 0
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic ena,
    input  logic [NUM_CLIENTS-1: 0] request,

    // One hot grant (same cycle as request)
    output logic [NUM_CLIENTS-1 : 0] grant,
    output logic [$clog2(NUM_CLIENTS)-1 : 0] grantIdx
    );

    typedef logic [NUM_CLIENTS-1 : 0] t_vec;
    typedef logic [2*NUM_CLIENTS-1 : 0] t_dbl_vec;

    // Priority (one hot)
    t_vec base;

    t_dbl_vec dbl_request;
    assign dbl_request = {request, request};

    t_dbl_vec dbl_grant;
    assign dbl_grant = dbl_request & ~(dbl_request - base);

    t_vec grant_reduce;
    assign grant_reduce = dbl_grant[NUM_CLIENTS-1 : 0] |
                          dbl_grant[2*NUM_CLIENTS-1 : NUM_CLIENTS];

    always_comb
    begin
        grantIdx = 0;
        for (int i = 0; i < NUM_CLIENTS; i = i + 1)
        begin
            grant[i] = grant_reduce[i] && ena;

            if (grant_reduce[i])
            begin
                grantIdx = ($clog2(NUM_CLIENTS))'(i);
            end
        end
    end

    // Record winner for next cycle's priority
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            base <= t_vec'(1);
        end
        else if (ena && |(request))
        begin
            // Rotate grant left so that the slot after the current winner
            // is given priority.
            base <= { grant_reduce[NUM_CLIENTS-2 : 0],
                      grant_reduce[NUM_CLIENTS-1] };
        end
    end

endmodule // cci_mpf_prim_arb_rr
