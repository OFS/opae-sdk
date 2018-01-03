//
// Copyright (c) 2017, Intel Corporation
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
// Track pending response counts on CCI-P channels.  These counts may be
// used for handing out request credits when responses are buffered.
//

import ccip_if_pkg::*;

module platform_utils_ccip_c0_active_cnt
  #(
    parameter C0RX_DEPTH_RADIX = 10
    )
   (
    input logic clk,
    input logic reset,

    input t_if_ccip_c0_Tx c0Tx,
    input t_if_ccip_c0_Rx c0Rx,

    output logic [C0RX_DEPTH_RADIX-1 : 0] cnt
    );

    typedef logic [C0RX_DEPTH_RADIX-1 : 0] t_active_cnt;

    logic [2:0] active_incr;
    logic active_decr;

    always_comb
    begin
        // Multi-beat read requests cause the activity counter to incremement
        // by the number of lines requested.
        if (c0Tx.valid)
        begin
            active_incr = 3'b1 + 3'(c0Tx.hdr.cl_len);
        end
        else
        begin
            active_incr = 3'b0;
        end

        // Only one line comes back at a time
        active_decr = c0Rx.rspValid && (c0Rx.hdr.resp_type == eRSP_RDLINE);
    end

    // Two stage counter improves timing but delays updates for a cycle.
    logic [3:0] active_delta;
    always_ff @(posedge clk)
    begin
        active_delta <= 4'(active_incr) - 4'(active_decr);
    end

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            cnt <= t_active_cnt'(0);
        end
        else
        begin
            cnt <= cnt + t_active_cnt'(signed'(active_delta));
        end
    end

endmodule // platform_utils_ccip_c0_active_cnt


module platform_utils_ccip_c1_active_cnt
  #(
    parameter C1RX_DEPTH_RADIX = 10
    )
   (
    input logic clk,
    input logic reset,

    input t_if_ccip_c1_Tx c1Tx,
    input t_if_ccip_c1_Rx c1Rx,

    output logic [C1RX_DEPTH_RADIX-1 : 0] cnt
    );

    typedef logic [C1RX_DEPTH_RADIX-1 : 0] t_active_cnt;

    logic active_incr;
    logic [2:0] active_decr;

    always_comb
    begin
        // New request?
        active_incr = c1Tx.valid;

        if (c1Rx.rspValid)
        begin
            if ((c1Rx.hdr.resp_type == eRSP_WRLINE) && c1Rx.hdr.format)
            begin
                // Packed response for multiple lines
                active_decr = 3'b1 + 3'(c1Rx.hdr.cl_num);
            end
            else
            begin
                // Response for a single request
                active_decr = 3'b1;
            end
        end
        else
        begin
            active_decr = 3'b0;
        end
    end

    // Two stage counter improves timing but delays updates for a cycle.
    logic [3:0] active_delta;
    always_ff @(posedge clk)
    begin
        active_delta <= 4'(active_incr) - 4'(active_decr);
    end

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            cnt <= t_active_cnt'(0);
        end
        else
        begin
            cnt <= cnt + t_active_cnt'(signed'(active_delta));
        end
    end

endmodule // platform_utils_ccip_c1_active_cnt
