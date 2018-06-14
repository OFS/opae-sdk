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

`include "cci_mpf_if.vh"
`include "cci_csr_if.vh"

//
// Latency QoS throttles request rates in an attempt to have the request
// rate on a given channel match the bandwidth of the channel without
// buffering extra requests in the FIU.  This should have no impact on
// bandwidth but it may reduce the latency of responses from the perspective
// of the AFU.  For single kernels this change in latency is likely
// irrelevant.  Adding latency QoS may improve performance when multiple
// kernels are multiplexed on the same CCI interface and one or more of
// those kernels is sensitive to read response latency.
//

//
// Type to hold the change in active lines for one request/response channel
// in one cycle.  This is new request lines - returned response lines.  It
// needs a sign bit.
//
typedef logic signed [$clog2(CCI_MAX_MULTI_LINE_BEATS+1)+1 : 0]
    t_cci_mpf_latency_qos_line_upd;


module cci_mpf_shim_latency_qos
  #(
    // Maximum number of outstanding read and write lines per channel
    parameter MAX_ACTIVE_LINES = 128
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    // CSRs
    cci_mpf_csrs.latency_qos csrs
    );

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    assign fiu.c0Tx = afu.c0Tx;
    assign fiu.c1Tx = afu.c1Tx;
    assign fiu.c2Tx = afu.c2Tx;
    assign afu.c0Rx = fiu.c0Rx;
    assign afu.c1Rx = fiu.c1Rx;

    // Assert almost full when the threshold is reached
    logic c0_force_almost_full, c1_force_almost_full;
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull || c0_force_almost_full;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull || c1_force_almost_full;

    // Add an extra bit to the counter to handle overflow of the credit
    // limit gracefullys.
    typedef logic [$clog2(MAX_ACTIVE_LINES+1) : 0] t_active_lines;

    t_active_lines active_lines_max[0:1];


    // ====================================================================
    //
    //   Define a sampling epoch and signal epochs to the credit managers.
    //
    // ====================================================================

    localparam EPOCH_COUNTER_BITS = 15;

    // At the start of each epoch the counter is initialized with the
    // value in this register.  The counter counts down and a new epoch
    // is triggered when the counter underflows.
    logic [EPOCH_COUNTER_BITS-1 : 0] epoch_cycles[0:1];

    // Signal a new epoch when the counter underflows
    logic [EPOCH_COUNTER_BITS : 0] epoch_counter[0:1];
    logic new_epoch[0:1];

    genvar i;
    generate
        //
        // The two channels have separate epoch counters since the
        // different lengths may be optimal.
        //
        for (i = 0; i < 2; i = i + 1)
        begin : epoch
            assign new_epoch[i] = epoch_counter[i][EPOCH_COUNTER_BITS];

            always_ff @(posedge clk)
            begin
                epoch_counter[i] <= epoch_counter[i] - 1;
                if (new_epoch[i])
                begin
                    epoch_counter[i] <= { 1'b0, epoch_cycles[i] };
                end

                if (reset)
                begin
                    epoch_counter[i] <= 0;
                end
            end
        end
    endgenerate


    // ====================================================================
    //
    //   CSR -- Set configuration using a single configuration register:
    //
    //   Bit 0:
    //      Enable (1) or disable (0) QoS management for reads (channel 0).
    //   Bit 1:
    //      Enable (1) or disable (0) QoS management for writes (channel 1).
    //   Bits 16-2:
    //      Maximum number of active read lines (channel 0).
    //   Bits 31-17:
    //      Maximum number of active write lines (channel 1).
    //   Bits 47-32:
    //      Number of cycles in an epoch for reads (channel 0).
    //   Bits 63-48:
    //      Number of cycles in an epoch for writes (channel 1).
    //
    // ====================================================================

    logic latency_qos_ctrl_valid_q;
    logic enable_qos[0:1];

    always_ff @(posedge clk)
    begin
        latency_qos_ctrl_valid_q <= csrs.latency_qos_ctrl_valid;

        if (csrs.latency_qos_ctrl_valid)
        begin
            //
            // Set configuration from the CSR
            //
            enable_qos[0] <= csrs.latency_qos_ctrl[0];
            enable_qos[1] <= csrs.latency_qos_ctrl[1];

            active_lines_max[0] <= t_active_lines'(csrs.latency_qos_ctrl[16:2]);
            active_lines_max[1] <= t_active_lines'(csrs.latency_qos_ctrl[31:17]);

            epoch_cycles[0] <= csrs.latency_qos_ctrl[32 +: EPOCH_COUNTER_BITS];
            epoch_cycles[1] <= csrs.latency_qos_ctrl[48 +: EPOCH_COUNTER_BITS];
        end

        if (reset)
        begin
            enable_qos[0] <= 1'b1;
            // Write QoS disabled by default
            enable_qos[1] <= 1'b0;

            active_lines_max[0] <= t_active_lines'(MAX_ACTIVE_LINES);
            active_lines_max[1] <= t_active_lines'(MAX_ACTIVE_LINES);

            epoch_cycles[0] <= 'd63;
            epoch_cycles[1] <= 'd63;

            //
            // Platform-specific defaults.
            //
            if (MPF_PLATFORM == "SKX")
            begin
//                active_lines_max[0] <= t_active_lines'(272);
                active_lines_max[0] <= t_active_lines'(328);
            end
            else if (MPF_PLATFORM == "BDX")
            begin
                active_lines_max[0] <= t_active_lines'(288);
                epoch_cycles[0] <= 'd511;
            end
        end
    end


    // ====================================================================
    //
    // Track changes in the number of active lines for c0 and c1.
    //
    // ====================================================================
    
    t_cci_mpf_latency_qos_line_upd c0_line_upd, c1_line_upd;

    t_cci_mpf_latency_qos_line_upd c0_req_line_upd, c0_rsp_line_upd;
    t_cci_mpf_latency_qos_line_upd c1_req_line_upd, c1_rsp_line_upd;

    always_ff @(posedge clk)
    begin
        // Change in active lines this cycle
        c0_line_upd <= c0_req_line_upd - c0_rsp_line_upd;
        c1_line_upd <= c1_req_line_upd - c1_rsp_line_upd;

        if (reset)
        begin
            c0_line_upd <= t_cci_mpf_latency_qos_line_upd'(0);
            c1_line_upd <= t_cci_mpf_latency_qos_line_upd'(0);
        end
    end

    always_comb
    begin
        c0_req_line_upd = t_cci_mpf_latency_qos_line_upd'(0);
        c0_rsp_line_upd = t_cci_mpf_latency_qos_line_upd'(0);

        if (cci_mpf_c0TxIsReadReq(afu.c0Tx))
        begin
            // Number of lines being read
            c0_req_line_upd = t_cci_mpf_latency_qos_line_upd'(1) +
                              t_cci_mpf_latency_qos_line_upd'(afu.c0Tx.hdr.base.cl_len);
        end

        if (cci_c0Rx_isReadRsp(fiu.c0Rx))
        begin
            // Single line read response
            c0_rsp_line_upd = t_cci_mpf_latency_qos_line_upd'(1);
        end


        c1_req_line_upd = t_cci_mpf_latency_qos_line_upd'(0);
        c1_rsp_line_upd = t_cci_mpf_latency_qos_line_upd'(0);

        if (cci_mpf_c1TxIsWriteReq(afu.c1Tx) && afu.c1Tx.hdr.base.sop)
        begin
            // Number of lines being written
            c1_req_line_upd = t_cci_mpf_latency_qos_line_upd'(1) +
                              t_cci_mpf_latency_qos_line_upd'(afu.c1Tx.hdr.base.cl_len);
        end

        if (cci_c1Rx_isWriteRsp(fiu.c1Rx))
        begin
            // Write responses are either packed (one response for
            // all lines) or unpacked.
            c1_rsp_line_upd = t_cci_mpf_latency_qos_line_upd'(1) +
                              (fiu.c1Rx.hdr.format ?
                                t_cci_mpf_latency_qos_line_upd'(fiu.c1Rx.hdr.cl_num) :
                                t_cci_mpf_latency_qos_line_upd'(0));
        end
    end


    // ====================================================================
    //
    //   Dynamic channel credit limiters.
    //
    // ====================================================================

    cci_mpf_shim_latency_qos_mgr
      #(
        .MAX_ACTIVE_LINES(MAX_ACTIVE_LINES)
        )
      c0mgr
       (
        .clk,
        .reset(reset || latency_qos_ctrl_valid_q),
        .enabled(enable_qos[0]),
        .new_epoch(new_epoch[0]),
        .line_upd(c0_line_upd),
        .fiu_almost_full(fiu.c0TxAlmFull),
        .active_lines_max(active_lines_max[0]),
        .almost_full(c0_force_almost_full)
        );

`ifdef LATENCY_QOS_MANAGE_C1
    //
    // There is currently no platform on which managing writes improves
    // performance.  This instance is kept for reference.
    //
    cci_mpf_shim_latency_qos_mgr
      #(
        .MAX_ACTIVE_LINES(MAX_ACTIVE_LINES)
        )
      c1mgr
       (
        .clk,
        .reset(reset || latency_qos_ctrl_valid_q),
        .enabled(enable_qos[1]),
        .new_epoch(new_epoch[1]),
        .line_upd(c1_line_upd),
        .fiu_almost_full(fiu.c1TxAlmFull),
        .active_lines_max(active_lines_max[1]),
        .almost_full(c1_force_almost_full)
        );
`else
    // No c1 management
    assign c1_force_almost_full = 1'b0;
`endif

endmodule // cci_mpf_shim_latency_qos


//
// cci_mpf_shim_latency_qos_mgr --
//
//   Manage credit limits for a single channel.
//
module cci_mpf_shim_latency_qos_mgr
  #(
    // Maximum number of outstanding read and write lines per channel.
    parameter MAX_ACTIVE_LINES = 128,

    // Minimum number of credits that must be allowed.
    parameter MIN_LINE_CREDITS = 32
    )
   (
    input  logic clk,
    input  logic reset,

    // QoS enabled?
    input  logic enabled,

    // New sampling epoch?
    input  logic new_epoch,

    // Change in the number of active lines
    input  t_cci_mpf_latency_qos_line_upd line_upd,

    // FIU signals almost full,
    input  logic fiu_almost_full,

    // Dynamic maximum number of active lines
    input  logic [$clog2(MAX_ACTIVE_LINES+1) : 0] active_lines_max,

    // Credit limit reached?
    output logic almost_full
    );
      
    // Add an extra bit to the counter to handle overflow of the credit
    // limit gracefully.
    typedef logic [$clog2(MAX_ACTIVE_LINES+1) : 0] t_active_lines;

    // Track active lines
    t_active_lines active_lines;
    t_active_lines line_credits_limit;


    //
    // Assert almost full when the credit threshold is reached.
    //
    always_ff @(posedge clk)
    begin
        almost_full <= enabled && (active_lines >= line_credits_limit);
        if (reset)
        begin
            almost_full <= 1'b0;
        end
    end


    //
    // Did the limiter fire during this epoch?
    //

    logic forced_almost_full;

    always_ff @(posedge clk)
    begin
        if (almost_full)
        begin
            forced_almost_full <= 1'b1;
        end

        if (reset || new_epoch)
        begin
            forced_almost_full <= 1'b0;
        end
    end


    //
    // Update active line count
    //

    // Sign extend line_upd
    t_active_lines line_upd_ext;
    assign line_upd_ext = t_active_lines'(line_upd);

    always_ff @(posedge clk)
    begin
        active_lines <= active_lines + line_upd_ext;

        if (reset)
        begin
            active_lines <= t_active_lines'(0);
        end
    end


    //
    // Did the FIU assert almost full?  If so, the credit threshold should
    // be reduced.
    //
    logic fiu_was_almost_full;

    always_ff @(posedge clk)
    begin
        if (fiu_almost_full)
        begin
            fiu_was_almost_full <= 1'b1;
        end

        if (reset || new_epoch)
        begin
            fiu_was_almost_full <= 1'b0;
        end
    end


    // ====================================================================
    //
    //   Track epochs since the last credit decrement.  Credit increments
    //   are blocked for a while after decrements to avoid jitter in
    //   the credit limit.
    //
    // ====================================================================

    logic credit_incr_locked;
    logic [3:0] credit_incr_locked_cnt;

    // Lock when the high bit of the counter is set
    assign credit_incr_locked = credit_incr_locked_cnt[3];

    logic did_epoch_decr;

    always_ff @(posedge clk)
    begin
        if (did_epoch_decr)
        begin
            // Did a decrement this cycle.  Start counting epochs.
            credit_incr_locked_cnt <= ~ 4'b0;
        end
        else if (new_epoch && credit_incr_locked)
        begin
            // Counter is active.  Decrement at epoch end.
            credit_incr_locked_cnt <= credit_incr_locked_cnt - 4'b1;
        end

        if (reset)
        begin
            credit_incr_locked_cnt <= 4'b0;
        end
    end


    // ====================================================================
    //
    //   Adjust credit limit to avoid filling the FIU queue.
    //
    // ====================================================================

    logic credits_at_min, credits_at_max;

    // Update the limiter at the end of each epoch
    always_ff @(posedge clk)
    begin
        if (! fiu_was_almost_full)
        begin
            // The FIU hasn't asserted almost full.  Consider
            // raising the credit limit.
            if (new_epoch && forced_almost_full &&
                ! credit_incr_locked && ! credits_at_max)
            begin
                // New epoch, the QoS limiter fired and credit increments
                // are currently permitted.  Increase the credit limit.
                line_credits_limit <= line_credits_limit + t_active_lines'(1);
            end
        end
        else
        begin
            // FIU asserted almost full.  Reduce the number of credits
            // (once per epoch).
            if (! did_epoch_decr && ! credits_at_min)
            begin
                line_credits_limit <= line_credits_limit - t_active_lines'(1);
            end
        end

        credits_at_min <= (line_credits_limit <= t_active_lines'(MIN_LINE_CREDITS));
        // Maximum number of credits is configurable at run time
        credits_at_max <= (line_credits_limit >= t_active_lines'(active_lines_max));

        did_epoch_decr <= fiu_was_almost_full;
        if (new_epoch)
        begin
            did_epoch_decr <= 1'b0;
        end

        if (reset)
        begin
            // Reset is asserted by the parent module when the control CSR
            // is valid so line_credits_limit is updated whenever
            // active_lines_max has a new value.
            line_credits_limit <= t_active_lines'(active_lines_max);
            did_epoch_decr <= 1'b0;
        end
    end

endmodule // cci_mpf_shim_latency_qos_mgr
