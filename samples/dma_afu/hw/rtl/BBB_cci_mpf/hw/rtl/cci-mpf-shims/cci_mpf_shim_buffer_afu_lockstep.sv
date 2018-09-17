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
// The same as cci_mpf_shim_buffer_afu except that channels 0 and 1 are held
// together and move through the buffer in lock step.  This is important
// for portions of the pipeline that need to maintain read/write ordering.
//

module cci_mpf_shim_buffer_afu_lockstep
  #(
    parameter THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,
    parameter N_ENTRIES = THRESHOLD * 2,

    // Register c0Tx and c1Tx outputs?
    parameter REGISTER_OUTPUT = 0
    )
   (
    input  logic clk,

    // Raw unbuffered connection.  This is the AFU-side connection of the
    // parent module.
    cci_mpf_if.to_afu afu_raw,

    // Generated buffered connection.  The confusing interface direction
    // arises because the shim is an interposer on the AFU side of a
    // standard shim.
    cci_mpf_if.to_fiu afu_buf,

    // Dequeue signal combined with the buffering make the buffered interface
    // latency insensitive.  Requests sit in the buffers unless explicitly
    // removed.
    //
    // Unlike cci_mpf_shim_buffer_afu, a single deq signal moves both channels.
    // The client must be prepared to move both channels or none.
    input  logic deqTx,

    //
    // Configure the internal QoS algorithm.  The sizes of thresholds may
    // be larger here than inside the module.
    //

    // Set parameters
    input  logic setqos,
    // Enable QoS throttling
    input  logic setqos_enable,
    // Beat disparity between channels that triggers QoS throttling
    input  logic [7:0] setqos_beat_delta_threshold,
    // Minimum beat count within a channel at which QoS may trigger
    input  logic [7:0] setqos_min_beat_threshold
    );

    logic reset;
    assign reset = afu_buf.reset;
    assign afu_raw.reset = afu_buf.reset;

    //
    // Rx wires pass through toward the AFU.  They are latency sensitive
    // since the CCI provides no back pressure.
    //
    assign afu_raw.c0Rx = afu_buf.c0Rx;
    assign afu_raw.c1Rx = afu_buf.c1Rx;


    // ====================================================================
    //
    //   Tx buffer.
    //
    //   The buffer triggers TxAlmFull when there are 4 or fewer slots
    //   available, as required by the CCI specification.  Unlike the
    //   usual CCI request interface, movement through the pipeline is
    //   explicit.  The code that instantiates this buffer must dequeue
    //   the head of the FIFO in order to consume a request.
    //
    // ====================================================================

    localparam C0TX_BITS = $bits(t_if_cci_mpf_c0_Tx);
    localparam C1TX_BITS = $bits(t_if_cci_mpf_c1_Tx);
    localparam TX_BITS = C0TX_BITS + C1TX_BITS;

    // Request payload exists when one of the valid bits is set.
    logic c0_enq_en;
    assign c0_enq_en = cci_mpf_c0TxIsValid(afu_raw.c0Tx);
    logic c1_enq_en;
    assign c1_enq_en = cci_mpf_c1TxIsValid(afu_raw.c1Tx);
    logic enq_en;
    assign enq_en = c0_enq_en || c1_enq_en;

    logic notEmpty;

    // Pull request details out of the head of the FIFO.
    logic [TX_BITS-1 : 0] first;
    always_comb
    begin
        { afu_buf.c0Tx, afu_buf.c1Tx } = first;

        // Valid bits are only meaningful when the FIFO isn't empty.
        afu_buf.c0Tx = cci_mpf_c0TxMaskValids(afu_buf.c0Tx, notEmpty);
        afu_buf.c1Tx = cci_mpf_c1TxMaskValids(afu_buf.c1Tx, notEmpty);
    end


    // ====================================================================
    //
    //   The channels are kept in lockstep to keep memory requests ordered.
    //   This can lead to one channel being starved.  Typically, this is
    //   the store channel when multi-beat requests are being generated
    //   since 4 store request beats are required to correspond with each
    //   4 beat read request.
    //
    //   Here we apply QoS in order to balance traffic.
    //
    // ====================================================================

    // Record the number of beats represented by requests in the queue.
    // One extra bit beyond enough for a maximum value is reserved for
    // use during comparison.
    typedef logic [$clog2(N_ENTRIES * 4) : 0] t_beats;

    t_beats channel_beats[0 : 1];

    //
    // Compute the change in outstanding beats for a single channel
    // given a potential new request entering and an old request leaving.
    //
    function automatic t_beats update_beats(logic req_in,
                                            t_cci_clLen req_in_size,
                                            logic req_out,
                                            t_cci_clLen req_out_size);
        return (req_in ? t_beats'(req_in_size) + t_beats'(1) : t_beats'(0)) -
               (req_out ? t_beats'(req_out_size) + t_beats'(1) : t_beats'(0));
    endfunction

    //
    // Track queued request beats.
    //
    logic req_enq_q[0 : 1];
    t_cci_clLen req_enq_len_q[0 : 1];
    logic req_deq_q[0 : 1];
    t_cci_clLen req_deq_len_q[0 : 1];

    always_ff @(posedge clk)
    begin
        req_enq_q[0] <= enq_en && cci_mpf_c0TxIsReadReq(afu_raw.c0Tx);
        req_enq_len_q[0] <= afu_raw.c0Tx.hdr.base.cl_len;
        req_deq_q[0] <= deqTx && cci_mpf_c0TxIsReadReq(afu_buf.c0Tx);
        req_deq_len_q[0] <= afu_buf.c0Tx.hdr.base.cl_len;

        req_enq_q[1] <= enq_en && cci_mpf_c1TxIsWriteReq(afu_raw.c1Tx);
        req_enq_len_q[1] <= afu_raw.c1Tx.hdr.base.cl_len;
        req_deq_q[1] <= deqTx && cci_mpf_c1TxIsWriteReq(afu_buf.c1Tx);
        req_deq_len_q[1] <= afu_buf.c1Tx.hdr.base.cl_len;

        if (reset)
        begin
            for (int i = 0; i < 2; i = i + 1)
            begin
                req_enq_q[i] <= 1'b0;
                req_deq_q[i] <= 1'b0;
            end
        end
    end

    always_ff @(posedge clk)
    begin
        for (int i = 0; i < 2; i = i + 1)
        begin
            channel_beats[i] <= channel_beats[i] +
                                update_beats(req_enq_q[i], req_enq_len_q[i],
                                             req_deq_q[i], req_deq_len_q[i]);

            if (reset)
            begin
                channel_beats[i] <= 1'b0;
            end
        end
    end

    //
    // QoS dynamically configurable parameters
    //
    logic qos_enable;
    t_beats qos_beat_delta_threshold;
    t_beats qos_min_beat_threshold;

    typedef logic [5 : 0] t_qos_cycles;
    t_qos_cycles qos_throttle_cycles;
    // Varying this doesn't seem to make a difference
    assign qos_throttle_cycles = t_qos_cycles'(8);

    always_ff @(posedge clk)
    begin
        if (setqos)
        begin
            qos_enable <= setqos_enable;
            qos_beat_delta_threshold <= t_beats'(setqos_beat_delta_threshold);
            qos_min_beat_threshold <= t_beats'(setqos_min_beat_threshold);
        end

        if (reset)
        begin
            qos_enable <= 1'b1;
            qos_beat_delta_threshold <= 6;
            qos_min_beat_threshold <= 0;
        end
    end


    //
    // Should pipe be throttled to enforce QoS?
    //
    function automatic logic throttle_for_qos(t_beats pipe, t_beats other_pipe);

        return qos_enable &&
               // Throttle if the difference in beats is above the threshold
               (pipe > (other_pipe + qos_beat_delta_threshold)) &&
               // and the other pipeline has at least the minimum number of beats
               (other_pipe > qos_min_beat_threshold);

    endfunction


    logic throttled_for_qos[0 : 1];
    logic was_throttled_for_qos[0 : 1];
    t_qos_cycles throttle_cycles[0 : 1];

    genvar c;
    generate
        for (c = 0; c < 2; c = c + 1)
        begin : t
            int c_other = (c == 0) ? 1 : 0;

            always_ff @(posedge clk)
            begin
                if (! throttled_for_qos[c] &&
                    ! was_throttled_for_qos[c_other] &&
                    throttle_for_qos(channel_beats[c], channel_beats[c_other]))
                begin
                    // Start throttling channel for qos_throttle_cycles
                    throttled_for_qos[c] <= 1'b1;
                    was_throttled_for_qos[c] <= 1'b1;
                    throttle_cycles[c] <= qos_throttle_cycles;
                end

                // Time to stop throttling channel?
                if (throttled_for_qos[c])
                begin
                    throttled_for_qos[c] <= |(throttle_cycles[c]);
                    throttle_cycles[c] <= throttle_cycles[c] - 1;
                end
                else if (was_throttled_for_qos[c])
                begin
                    // Count another throttling quantum in which the other channel
                    // is kept from overcompensating.
                    was_throttled_for_qos[c] <= |(throttle_cycles[c]);
                    throttle_cycles[c] <= throttle_cycles[c] - 1;
                end

                if (reset)
                begin
                    throttled_for_qos[c] <= 1'b0;
                    was_throttled_for_qos[c] <= 1'b0;
                end
            end
        end
    endgenerate


    //
    // Use the almost full wire to enforce QoS.
    //
    logic almostFull;
    assign afu_raw.c0TxAlmFull = almostFull || throttled_for_qos[0];
    assign afu_raw.c1TxAlmFull = almostFull || throttled_for_qos[1];


    // ====================================================================
    //
    //   Lockstep buffer.
    //
    // ====================================================================

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(TX_BITS),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT)
        )
      fifo
       (
        .clk,
        .reset(afu_buf.reset),

        // The concatenated field order must match the use of c1_first above.
        .enq_data({ afu_raw.c0Tx, afu_raw.c1Tx }),
        .enq_en,
        .notFull(),
        .almostFull,

        .first,
        .deq_en(deqTx),
        .notEmpty(notEmpty)
        );


    // ====================================================================
    //
    //   Channel 2 Tx (MMIO read response) is unbuffered.
    //
    // ====================================================================

    assign afu_buf.c2Tx = afu_raw.c2Tx;

endmodule // cci_mpf_shim_buffer_afu_lockstep
