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
// NULL instances of MPF main wrapper, useful sometimes for maintaining
// a consistent interface for AFUs that don't use MPF but want to use
// the MPF interface for representing requests and responses.
//
// The module connects an FIU to an AFU interface and generates the
// not empty signals expected by AFUs.  The module also enforces a maximum
// number of requests in flight.
//

`include "cci_mpf_if.vh"
`include "cci_mpf_csrs.vh"
`include "cci_mpf_shim_edge.vh"
`include "cci_mpf_shim_pwrite.vh"


module cci_mpf_null
   (
    input  logic      clk,

    //
    // Signals connecting to QA Platform
    //
    cci_mpf_if.to_fiu fiu,

    //
    // Signals connecting to AFU, the client code
    //
    cci_mpf_if.to_afu afu,

    //
    // Is a request active somewhere in the memory?  Clients waiting for
    // all responses to complete can monitor these signals.
    //
    output logic c0NotEmpty,
    output logic c1NotEmpty
    );

    // Maximum number of outstanding read and write line requests per channel
`ifdef MPF_PLATFORM_SKX
    localparam MAX_ACTIVE_LINES = 512;
`elsif MPF_PLATFORM_DCP_PCIE
    localparam MAX_ACTIVE_LINES = 512;
`elsif MPF_PLATFORM_BDX
    localparam MAX_ACTIVE_LINES = 512;
`elsif MPF_PLATFORM_OME
    localparam MAX_ACTIVE_LINES = 128;
`else
    ** ERROR: Unknown platform
`endif

    localparam MAX_ACTIVE_WRFENCES = CCI_TX_ALMOST_FULL_THRESHOLD * 2;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    logic c0_almost_full;
    logic c1_almost_full;

    always_comb
    begin
        afu.reset = fiu.reset;

        afu.c0TxAlmFull = fiu.c0TxAlmFull || c0_almost_full;
        afu.c1TxAlmFull = fiu.c1TxAlmFull || c1_almost_full;

        afu.c0Rx = fiu.c0Rx;
        afu.c1Rx = fiu.c1Rx;

        fiu.c0Tx = afu.c0Tx;
        fiu.c1Tx = afu.c1Tx;
        fiu.c2Tx = afu.c2Tx;
    end


    // ====================================================================
    //
    //  Track active request counts
    //
    // ====================================================================

    // Leave an extra bit since the counter is at the edge before the
    // initial buffer that limits request counts to MAX_ACTIVE_LINES.
    // The count can thus be higher than MAX_ACTIVE_LINES here.
    typedef logic [$clog2(MAX_ACTIVE_LINES) : 0] t_active_cnt;
    typedef logic [$clog2(MAX_ACTIVE_WRFENCES)-1 : 0] t_active_wrfence_cnt;

    t_active_cnt c0_num_active, c1_num_active;
    t_active_wrfence_cnt c1_wrfence_cnt;

    // Signal almost full when too many requests are active
    always_ff @(posedge clk)
    begin
        // Block all traffic during a write fence.  Writes tend to hang
        // if traffic is allowed to flow during a fence request.

        c0_almost_full <= (c0_num_active > t_active_cnt'(MAX_ACTIVE_LINES - CCI_TX_ALMOST_FULL_THRESHOLD - 4)) ||
                          (c1_wrfence_cnt != t_active_wrfence_cnt'(0));
        c1_almost_full <= (c1_num_active > t_active_cnt'(MAX_ACTIVE_LINES - CCI_TX_ALMOST_FULL_THRESHOLD - 4)) ||
                          (c1_wrfence_cnt != t_active_wrfence_cnt'(0));
    end

    // Increment/decrement count updates for current cycle.  Some counts
    // must be large enough to hold multi-line counts.
    logic [2:0] c0_active_incr;
    logic c0_active_decr;
    logic c1_active_incr;
    logic [2:0] c1_active_decr;

    always_comb
    begin
        if (cci_mpf_c0TxIsReadReq(afu.c0Tx))
        begin
            c0_active_incr = 3'b1 + 3'(afu.c0Tx.hdr.base.cl_len);
        end
        else
        begin
            c0_active_incr = 3'b0;
        end
        c0_active_decr = cci_c0Rx_isReadRsp(afu.c0Rx);

        c1_active_incr = cci_mpf_c1TxIsWriteReq(afu.c1Tx);
        if (cci_c1Rx_isWriteRsp(afu.c1Rx))
        begin
            if (afu.c1Rx.hdr.format)
            begin
                // Packed response for multiple lines
                c1_active_decr = 3'b1 + 3'(afu.c1Rx.hdr.cl_num);
            end
            else
            begin
                c1_active_decr = 3'b1;
            end
        end
        else
        begin
            c1_active_decr = 3'b0;
        end
    end

    always_ff @(posedge clk)
    begin
        c0NotEmpty <= cci_mpf_c0TxIsReadReq(afu.c0Tx) || (|(c0_num_active));
        c1NotEmpty <= cci_mpf_c1TxIsWriteReq(afu.c1Tx) || (|(c1_num_active));

        c0_num_active <= c0_num_active +
                         t_active_cnt'(c0_active_incr) -
                         t_active_cnt'(c0_active_decr);

        c1_num_active <= c1_num_active +
                         t_active_cnt'(c1_active_incr) -
                         t_active_cnt'(c1_active_decr);

        if (reset)
        begin
            c0NotEmpty <= 1'b0;
            c1NotEmpty <= 1'b0;

            c0_num_active <= t_active_cnt'(0);
            c1_num_active <= t_active_cnt'(0);
        end
    end

    // Track write fence activity
    always_ff @(posedge clk)
    begin
        if (cci_mpf_c1TxIsWriteFenceReq(afu.c1Tx) && ! cci_c1Rx_isWriteFenceRsp(afu.c1Rx))
        begin
            // New fence request
            c1_wrfence_cnt <= c1_wrfence_cnt + t_active_wrfence_cnt'(1);
        end
        else if (! cci_mpf_c1TxIsWriteFenceReq(afu.c1Tx) && cci_c1Rx_isWriteFenceRsp(afu.c1Rx))
        begin
            // Response
            c1_wrfence_cnt <= c1_wrfence_cnt - t_active_wrfence_cnt'(1);
        end

        if (reset)
        begin
            c1_wrfence_cnt <= t_active_wrfence_cnt'(0);
        end
    end

endmodule // cci_mpf_null
