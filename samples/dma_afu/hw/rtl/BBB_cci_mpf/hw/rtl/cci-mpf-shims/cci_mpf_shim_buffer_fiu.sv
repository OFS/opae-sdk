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

`include "cci_mpf_if.vh"


//
// This is more a primitive shim than a full fledged shim.  It takes a
// FIU-side raw connection (wires) and adds one cycle of buffering to
// all the RX signals that flow toward the AFU.  The TX signals flowing
// toward the FIU pass through as wires.
//
// This structure is useful when a shim needs to look up some data
// associated with a response that is stored in block RAM.  The RAM read
// request can be triggered when the response arrives from the FIU and
// the RAM response is then available when the response exits the
// buffer built here.
//
// Unlike the equivalent AFU buffer there is no flow control.  The CCI
// interface does not provide for back pressure on the flow of responses.
//

module cci_mpf_shim_buffer_fiu
  #(
    // Register outbound signals if nonzero.
    parameter REGISTER_OUTBOUND = 0,

    // Number of RX register stages.  Even 0 works, which is just wires.
    // This may be useful for incorporating in shims that may or may not
    // need any buffering.
    parameter N_RX_REG_STAGES = 1
    )
   (
    input  logic clk,

    // Raw unbuffered connection.  This is the FIU-side connection of the
    // parent module.
    cci_mpf_if.to_fiu fiu_raw,

    // Generated buffered connection.  The confusing interface direction
    // arises because the shim is an interposer on the FIU side of a
    // standard shim.
    cci_mpf_if.to_afu fiu_buf
    );

    assign fiu_buf.reset = fiu_raw.reset;

    //
    // Tx wires pass through toward the FIU. They are straight assignments
    // if REGISTER_OUTBOUND is 0.
    //
    generate
        if (REGISTER_OUTBOUND == 0)
        begin : tx_nr
            always_comb
            begin
                fiu_raw.c0Tx = fiu_buf.c0Tx;
                fiu_raw.c1Tx = fiu_buf.c1Tx;
                fiu_raw.c2Tx = fiu_buf.c2Tx;
            end
        end
        else
        begin : tx_r
            always_ff @(posedge clk)
            begin
                fiu_raw.c0Tx <= fiu_buf.c0Tx;
                fiu_raw.c1Tx <= fiu_buf.c1Tx;
                fiu_raw.c2Tx <= fiu_buf.c2Tx;
            end
        end
    endgenerate

    assign fiu_buf.c0TxAlmFull = fiu_raw.c0TxAlmFull;
    assign fiu_buf.c1TxAlmFull = fiu_raw.c1TxAlmFull;


    //
    // Rx
    //
    t_if_ccip_c0_Rx c0rx[N_RX_REG_STAGES-1 : 0];
    t_if_ccip_c1_Rx c1rx[N_RX_REG_STAGES-1 : 0];

    generate
        if (N_RX_REG_STAGES == 0)
        begin : rx_nr
            always_comb
            begin
                fiu_buf.c0Rx = fiu_raw.c0Rx;
                fiu_buf.c1Rx = fiu_raw.c1Rx;
            end
        end
        else
        begin : rx_r
            always_ff @(posedge clk)
            begin
                c0rx[0] <= fiu_raw.c0Rx;
                c1rx[0] <= fiu_raw.c1Rx;

                for (int s = 1; s < N_RX_REG_STAGES; s = s + 1)
                begin
                    c0rx[s] <= c0rx[s-1];
                    c1rx[s] <= c1rx[s-1];
                end
            end

            assign fiu_buf.c0Rx = c0rx[N_RX_REG_STAGES-1];
            assign fiu_buf.c1Rx = c1rx[N_RX_REG_STAGES-1];
        end
    endgenerate

endmodule // cci_mpf_shim_buffer_fiu
