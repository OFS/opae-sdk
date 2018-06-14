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
`include "cci_mpf_prim_hash.vh"


//
// A buffered AFU that detects inter-channel (read vs. write) hazards.
// The buffer defines an epoch as a region with no hazards.  When a
// hazard is detected the epoch is incremented.  When a read and a
// write to the same address arrive in the same cycle the read has
// priority and the epoch is incremented between the two, forcing
// the read to be available to the client separate from the write.
//

module cci_mpf_shim_buffer_afu_epoch
  #(
    parameter THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,
    parameter N_ENTRIES = THRESHOLD + 8,

    // Depth of the client's pipeline during which requests are active
    // in the epoch counter lifetime.
    parameter CLIENT_PIPE_STAGES = 0,

    // Register client_afu c0Tx and c1Tx outputs?
    parameter REGISTER_OUTPUT = 0,

    // FILTER_ADDRESS_HASH_BITS defines the hash space of the internal epoch
    // tracking filters.
    parameter FILTER_ADDRESS_HASH_BITS = 10,

    // The remove FIFO is driven by Tx requests on client_fiu and may fill
    // because of memory port contention in the epoch filters.  The filter
    // being full is indicated by almFull signals on client_fiu.  These
    // parameters control when almost full is signalled.
    parameter N_REMOVE_FIFO_ENTRIES = CCI_TX_ALMOST_FULL_THRESHOLD + 16,
    parameter N_REMOVE_FIFO_THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD
    )
   (
    input  logic clk,

    //
    // The MPF interfaces are structured differently in this module.
    // In addition to the usual AFU-side bus connected to "afu" and
    // FIU-side bus connected to "fiu" there are client connections.
    // The AFU side of the client is on "client_afu" and the FIU
    // on "client_fiu".  Requests (Tx) flow in on afu, out to the
    // client on client_afu, back in to this model on client_fiu
    // and then out again on fiu.  This extra flow exists because
    // the epoch tracking is maintained both inside the buffers here
    // and inside client pipelines.  This module retires active
    // addresses as Tx requests flow from client_fiu to fiu.
    //
    cci_mpf_if.to_afu afu,
    cci_mpf_if.to_fiu client_afu,
    cci_mpf_if.to_afu client_fiu,
    cci_mpf_if.to_fiu fiu,

    // Dequeue signal combined with the buffering make the buffered interface
    // latency insensitive.  Requests sit in the buffers in front of
    // client_afu Tx requests unless explicitly removed.
    input  logic deqC0Tx,
    input  logic deqC1Tx,

    // Hints to client about inter-channel dependence.  If an epoch unchanged
    // flag is set then the next request is in the same epoch as the previous
    // one, indicating that the next request is to a different address than
    // the most recent request on the other channel.
    output logic c0Tx_epoch_unchanged,
    output logic c1Tx_epoch_unchanged
    );

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    assign afu.reset = client_afu.reset;
    assign client_fiu.reset = fiu.reset;

    // Connect the Rx wires.
    assign afu.c0Rx = client_afu.c0Rx;
    assign afu.c1Rx = client_afu.c1Rx;
    assign client_fiu.c0Rx = fiu.c0Rx;
    assign client_fiu.c1Rx = fiu.c1Rx;


    // ====================================================================
    //
    //   Epoch is used to track hazards between the read and write
    //   request queues. The epoch is advanced each time a new hazard is
    //   detected in order to enforce inter-channel ordering.
    //
    // ====================================================================

    // The epoch counter must be large enough that it can represent all
    // active epochs plus an extra bit that is used to manage counter
    // wrapping.  At overflow (counter wrap), epoch comparison works
    // because the difference between two counters can never be more than
    // half the maximum value.
    typedef logic [2 : 0] t_epoch;

    // Return 1 iff epoch a is less than (older than) or equal to epoch b
    function automatic logic cmp_epoch_le(input t_epoch a, input t_epoch b);
        // As long as their difference is under half the epoch space then
        // a is less than b.
        t_epoch d = b - a;
        return ~d[$high(d)];
    endfunction


    logic c0Tx_incr_epoch;
    logic c1Tx_incr_epoch;

    logic c0_epoch_in_en;
    logic c1_epoch_in_en;

    logic rdy;
    logic c0_epoch_deq_almostFull;
    logic c1_epoch_deq_almostFull;

    cci_mpf_shim_buffer_afu_epoch_hazards
      #(
        .CLIENT_PIPE_STAGES(CLIENT_PIPE_STAGES + N_ENTRIES + 1),
        .FILTER_ADDRESS_HASH_BITS(FILTER_ADDRESS_HASH_BITS),
        .N_REMOVE_FIFO_ENTRIES(N_REMOVE_FIFO_ENTRIES),
        .N_REMOVE_FIFO_THRESHOLD(N_REMOVE_FIFO_THRESHOLD)
        )
      epoch_upd
       (
        .clk,
        .reset,
        .rdy,

        .c0Tx_in(afu.c0Tx),
        .c1Tx_in(afu.c1Tx),

        .c0Tx_incr_epoch,
        .c0Tx_en(c0_epoch_in_en),

        .c1Tx_incr_epoch,
        .c1Tx_en(c1_epoch_in_en),

        .c0Tx_out(client_fiu.c0Tx),
        .c1Tx_out(client_fiu.c1Tx),

        .c0Tx_deq_almostFull(c0_epoch_deq_almostFull),
        .c1Tx_deq_almostFull(c1_epoch_deq_almostFull)
    );

    //
    // Update epoch counters as new requests arrive from the AFU.
    //
    t_epoch c0_epoch_in, c0_epoch_in_next;
    t_epoch c1_epoch_in, c1_epoch_in_next;

    always_comb
    begin
        if (c0Tx_incr_epoch)
        begin
            // c0 comes from last cycle's c1 epoch since c1 may have been
            // updated on its own last cycle
            c0_epoch_in_next = c1_epoch_in + t_epoch'(1);
        end
        else
        begin
            // If c1 epoch was updated last cycle make c0 epoch match
            c0_epoch_in_next = c1_epoch_in;
        end

        if (c1Tx_incr_epoch)
        begin
            // Increment by 2 if both c0 and c1 need updates this cycle,
            // in which case both references this cycle are to the same
            // address.  If only c1 is updated then add just 1.
            c1_epoch_in_next = c1_epoch_in + t_epoch'(c0Tx_incr_epoch ? 2'd2 : 2'd1);
        end
        else if (c0Tx_incr_epoch)
        begin
            // Incrementing c0 causes c1 to increment this cycle too
            c1_epoch_in_next = c1_epoch_in + t_epoch'(1);
        end
        else
        begin
            c1_epoch_in_next = c1_epoch_in;
        end
    end

    always_ff @(posedge clk)
    begin
        c0_epoch_in <= c0_epoch_in_next;
        c1_epoch_in <= c1_epoch_in_next;

        if (reset)
        begin
            c0_epoch_in <= t_epoch'(0);
            c1_epoch_in <= t_epoch'(0);
        end
    end


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

    logic c0Tx_enabled;
    logic c1Tx_enabled;

    // Pull request details out of the heads of the FIFOs.
    t_if_cci_mpf_c0_Tx c0Tx;
    t_if_cci_mpf_c1_Tx c1Tx;

    always_comb
    begin
        // Valid bits are only meaningful when the FIFO isn't empty.
        client_afu.c0Tx = cci_mpf_c0TxMaskValids(c0Tx, c0Tx_enabled);
        client_afu.c1Tx = cci_mpf_c1TxMaskValids(c1Tx, c1Tx_enabled);
    end


    // ====================================================================
    //
    //   Channel buffers remain independent.  The correspondence between
    //   the two in case of address conflicts is maintained in a separate
    //   FIFO.
    //
    // ====================================================================

    logic c0Tx_notEmpty;
    logic c1Tx_notEmpty;

    t_epoch c0_epoch;
    t_epoch c1_epoch;

    logic c0_epoch_deq;
    logic c1_epoch_deq;

    logic c0Tx_almFull;
    logic c1Tx_almFull;
    always_ff @(posedge clk)
    begin
        afu.c0TxAlmFull <= c0Tx_almFull || ~ rdy;
        afu.c1TxAlmFull <= c1Tx_almFull || ~ rdy;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_if_cci_mpf_c0_Tx)),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT)
        )
      c0tx_fifo
       (
        .clk,
        .reset,

        .enq_data(afu.c0Tx),
        .enq_en(cci_mpf_c0TxIsValid(afu.c0Tx)),
        .notFull(),
        .almostFull(c0Tx_almFull),

        .first(c0Tx),
        .deq_en(deqC0Tx),
        // Not empty driven by epoch FIFO below
        .notEmpty()
        );

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_epoch)),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(1),
        .BYPASS_TO_REGISTER(1)
        )
      c0tx_epoch_fifo
       (
        .clk,
        .reset,

        .enq_data(c0_epoch_in_next),
        .enq_en(c0_epoch_in_en),
        .notFull(),
        // almostFull driven by request FIFO above
        .almostFull(),

        .first(c0_epoch),
        .deq_en(c0_epoch_deq),
        .notEmpty(c0Tx_notEmpty)
        );


    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_if_cci_mpf_c1_Tx)),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT)
        )
      c1tx_fifo
       (
        .clk,
        .reset,

        .enq_data(afu.c1Tx),
        .enq_en(cci_mpf_c1TxIsValid(afu.c1Tx)),
        .notFull(),
        .almostFull(c1Tx_almFull),

        .first(c1Tx),
        .deq_en(deqC1Tx),
        // Not empty driven by epoch FIFO below
        .notEmpty()
        );

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_epoch)),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(1),
        .BYPASS_TO_REGISTER(1)
        )
      c1tx_epoch_fifo
       (
        .clk,
        .reset,

        .enq_data(c1_epoch_in_next),
        .enq_en(c1_epoch_in_en),
        .notFull(),
        // almostFull driven by request FIFO above
        .almostFull(),

        .first(c1_epoch),
        .deq_en(c1_epoch_deq),
        .notEmpty(c1Tx_notEmpty)
        );


    // ====================================================================
    //
    //   Channel 2 Tx (MMIO read response) is unbuffered.
    //
    // ====================================================================

    assign client_afu.c2Tx = afu.c2Tx;


    // ====================================================================
    //
    //   Use epochs to enforce channel ordering by masking each request
    //   channel's valid bit as needed.
    //
    // ====================================================================

    t_epoch cur_epoch;

    always_ff @(posedge clk)
    begin
        //
        // No need to compare epochs when advancing.  Deque will be
        // permitted only in epoch order.
        //
        if (c0_epoch_deq)
        begin
            cur_epoch <= c0_epoch;
            assert (reset ||
                    (cur_epoch == c0_epoch) ||
                    (cur_epoch + t_epoch'(1) == c0_epoch)) else $fatal(2, "c0_epoch");
        end
        if (c1_epoch_deq)
        begin
            cur_epoch <= c1_epoch;
            assert (reset ||
                    (cur_epoch == c1_epoch) ||
                    (cur_epoch + t_epoch'(1) == c1_epoch)) else $fatal(2, "c1_epoch");
        end

        if (reset)
        begin
            cur_epoch <= t_epoch'(0);
        end
    end


    //
    // Compute the next state of Tx_enabled, assuming all previous enabled
    // requests have been consumed.
    //
    logic c0Tx_enabled_next;
    logic c1Tx_enabled_next;

    logic enabled_reqs_complete;

    always_comb
    begin
        c0Tx_enabled_next = c0Tx_notEmpty && ! c0_epoch_deq_almostFull &&
                             // Epoch hasn't changed
                            ((cur_epoch == c0_epoch) ||
                             // No c1 request
                             (! c1Tx_notEmpty) ||
                             // c0 request is older than c1 request
                             cmp_epoch_le(c0_epoch, c1_epoch));
                           
        c1Tx_enabled_next = c1Tx_notEmpty && ! c1_epoch_deq_almostFull &&
                             // Epoch hasn't changed
                            ((cur_epoch == c1_epoch) ||
                             // No c0 request
                             (! c0Tx_notEmpty) ||
                             // c1 request is older than c0 request
                             cmp_epoch_le(c1_epoch, c0_epoch));

        // Requests are enabled across all channels in the same cycle in
        // order to preserve epoch ordering.  New requests will be considered
        // when no enabled requests remain.
        enabled_reqs_complete = (! c0Tx_enabled || deqC0Tx) &&
                                (! c1Tx_enabled || deqC1Tx);

        c0_epoch_deq = enabled_reqs_complete && c0Tx_enabled_next;
        c1_epoch_deq = enabled_reqs_complete && c1Tx_enabled_next;
    end

    //
    // Track Tx_enabled state, indicating whether a channel is allowed to
    // fire because it is both data ready and in the proper epoch.
    //
    always_ff @(posedge clk)
    begin
        // Was the state consumed this cycle?
        if (deqC0Tx)
        begin
            c0Tx_enabled <= 1'b0;
        end
        if (deqC1Tx)
        begin
            c1Tx_enabled <= 1'b0;
        end

        if (enabled_reqs_complete)
        begin
            c0Tx_enabled <= c0Tx_enabled_next;
            c1Tx_enabled <= c1Tx_enabled_next;

            c0Tx_epoch_unchanged <= (cur_epoch == c0_epoch);
            c1Tx_epoch_unchanged <= (cur_epoch == c1_epoch);
        end

        if (reset)
        begin
            c0Tx_enabled <= 1'b0;
            c1Tx_enabled <= 1'b0;
        end
    end


    //
    // client_fiu to fiu Tx connections
    //
    assign fiu.c0Tx = client_fiu.c0Tx;
    assign fiu.c1Tx = client_fiu.c1Tx;
    assign fiu.c2Tx = client_fiu.c2Tx;
    assign client_fiu.c0TxAlmFull = fiu.c0TxAlmFull || c0_epoch_deq_almostFull;
    assign client_fiu.c1TxAlmFull = fiu.c1TxAlmFull || c1_epoch_deq_almostFull;

endmodule // cci_mpf_shim_buffer_lockstep_afu


//
// Track the need to increment the epoch based on inter-channel hazards.
//
module cci_mpf_shim_buffer_afu_epoch_hazards
  #(
    // Maximum value of a counter.  This should usually be the depth
    // of the full pipeline between filter insertion and removal.
    parameter CLIENT_PIPE_STAGES = 0,
    parameter FILTER_ADDRESS_HASH_BITS = 10,

    // Remove FIFO control.  Filter removal shares memory ports with insertion,
    // so may fill.
    parameter N_REMOVE_FIFO_ENTRIES = 16,
    parameter N_REMOVE_FIFO_THRESHOLD = 2
    )
   (
    input  logic clk,
    input  logic reset,
    output logic rdy,

    //
    // Filter insertion state.  This interface is latency insensitive.
    // Output responses signal when they are available by raising
    // the output enable wires when the incr_epoch state is available.
    // There is one output for every valid input on a Tx request bus.
    //

    input  t_if_cci_mpf_c0_Tx c0Tx_in,
    input  t_if_cci_mpf_c1_Tx c1Tx_in,

    output logic c0Tx_incr_epoch,
    output logic c0Tx_en,

    output logic c1Tx_incr_epoch,
    output logic c1Tx_en,

    //
    // Filter removal state.
    //

    input  t_if_cci_mpf_c0_Tx c0Tx_out,
    input  t_if_cci_mpf_c1_Tx c1Tx_out,

    // Removal queue may fill because it shares a filter memory port with
    // insertion.
    output logic c0Tx_deq_almostFull,
    output logic c1Tx_deq_almostFull
    );

    // ***
    //
    // Represent pipeline stages as array indices throughout the module
    //
    // ***

    t_if_cci_mpf_c0_Tx c0Tx[0:3];
    t_if_cci_mpf_c1_Tx c1Tx[0:3];

    //
    // Pipeline data flow
    //
    assign c0Tx[0] = c0Tx_in;
    assign c1Tx[0] = c1Tx_in;

    genvar p;
    generate
        for (p = 1; p <= 3; p = p + 1)
        begin : upipe
            always_ff @(posedge clk)
            begin
                c0Tx[p] <= c0Tx[p-1];
                c1Tx[p] <= c1Tx[p-1];

                if (reset)
                begin
                    c0Tx[p] <= cci_mpf_c0Tx_clearValids();
                    c1Tx[p] <= cci_mpf_c1Tx_clearValids();
                end
            end
        end
    endgenerate


    //
    // The filter is a set of counters with the bucket sizes set based on
    // the depth of the FIFO.  The client should set the counters large enough
    // that they never overflow.
    //
    localparam N_FILTER_BUCKETS = (1 << FILTER_ADDRESS_HASH_BITS);
    typedef logic [FILTER_ADDRESS_HASH_BITS-1 : 0] t_filter_idx;

    // Filter indices of current requests
    t_filter_idx c0_filter_idx[1:3];
    t_filter_idx c1_filter_idx[1:3];

    always_ff @(posedge clk)
    begin
        // Filter using hashed addresses, ignoring the low bits that might be
        // included in multi-line requests.
        c0_filter_idx[1] <=
            t_filter_idx'(hash32(c0Tx_in.hdr.base.address[$bits(t_cci_clLen) +: 32]));
        c0_filter_idx[2:3] <= c0_filter_idx[1:2];

        c1_filter_idx[1] <=
            t_filter_idx'(hash32(c1Tx_in.hdr.base.address[$bits(t_cci_clLen) +: 32]));
        c1_filter_idx[2:3] <= c1_filter_idx[1:2];
    end


    //
    // Track filter readiness
    //
    logic c0_rdy;
    logic c1_rdy;

    always_ff @(posedge clk)
    begin
        rdy <= c0_rdy && c1_rdy;
        if (reset)
        begin
            rdy <= 1'b0;
        end
    end


    logic c0_addr_inactive;
    logic c1_addr_inactive;

    logic c0_remove_en;
    t_filter_idx c0_remove_idx;
    logic c1_remove_en;
    t_filter_idx c1_remove_idx;

    always_ff @(posedge clk)
    begin
        c0_remove_en <= cci_mpf_c0TxIsReadReq(c0Tx_out) &&
                        c0Tx_out.hdr.ext.checkLoadStoreOrder;
        c0_remove_idx <= 
            t_filter_idx'(hash32(c0Tx_out.hdr.base.address[$bits(t_cci_clLen) +: 32]));

        c1_remove_en <= cci_mpf_c1TxIsWriteReq(c1Tx_out) &&
                        c1Tx_out.hdr.ext.checkLoadStoreOrder;
        c1_remove_idx <=
            t_filter_idx'(hash32(c1Tx_out.hdr.base.address[$bits(t_cci_clLen) +: 32]));

        if (reset)
        begin
            c0_remove_en <= 1'b0;
            c1_remove_en <= 1'b0;
        end
    end


    cci_mpf_prim_filter_banked_counting
      #(
        .N_BANKS_RADIX(1),
        .N_BUCKETS(N_FILTER_BUCKETS),
        .BITS_PER_BUCKET($clog2(CLIENT_PIPE_STAGES+4)),
        .N_REMOVE_FIFO_ENTRIES(N_REMOVE_FIFO_ENTRIES),
        .N_REMOVE_FIFO_THRESHOLD(N_REMOVE_FIFO_THRESHOLD)
        )
      c0filter
       (
        .clk,
        .reset,
        .rdy(c0_rdy),

        .test_value_req(),
        .test_value_en(1'b0),
        .T2_test_value(),

        .test_isZero_req(c1_filter_idx[1]),
        .test_isZero_en(cci_mpf_c1TxIsWriteReq(c1Tx[1])),
        .T2_test_isZero(c1_addr_inactive),

        .insert_en(cci_mpf_c0TxIsReadReq(c0Tx[1]) &&
                   c0Tx[1].hdr.ext.checkLoadStoreOrder),
        .insert(c0_filter_idx[1]),

        .remove_en(c0_remove_en),
        .remove(c0_remove_idx),
        .remove_notFull(),
        .remove_almostFull(c0Tx_deq_almostFull)
        );


    cci_mpf_prim_filter_banked_counting
      #(
        .N_BANKS_RADIX(1),
        .N_BUCKETS(N_FILTER_BUCKETS),
        .BITS_PER_BUCKET($clog2(CLIENT_PIPE_STAGES+4)),
        .N_REMOVE_FIFO_ENTRIES(N_REMOVE_FIFO_ENTRIES),
        .N_REMOVE_FIFO_THRESHOLD(N_REMOVE_FIFO_THRESHOLD)
        )
      c1filter
       (
        .clk,
        .reset,
        .rdy(c1_rdy),

        .test_value_req(),
        .test_value_en(1'b0),
        .T2_test_value(),

        .test_isZero_req(c0_filter_idx[1]),
        .test_isZero_en(cci_mpf_c0TxIsReadReq(c0Tx[1])),
        .T2_test_isZero(c0_addr_inactive),

        .insert_en(cci_mpf_c1TxIsWriteReq(c1Tx[1]) &&
                   c1Tx[1].hdr.ext.checkLoadStoreOrder),
        .insert(c1_filter_idx[1]),

        .remove_en(c1_remove_en),
        .remove(c1_remove_idx),
        .remove_notFull(),
        .remove_almostFull(c1Tx_deq_almostFull)
        );


    // Is a hazard detected this cycle?
    logic c0_hazard;
    logic c1_hazard;

    logic c0_active_this_epoch;
    logic c1_active_this_epoch;
    logic c1_hazard_was_last;

    always_comb
    begin
        // Has the c0 address been seen in c1 already?
        c0_hazard = cci_mpf_c0TxIsReadReq(c0Tx[3]) &&
                    c0Tx[3].hdr.ext.checkLoadStoreOrder &&
                    ! c0_addr_inactive &&
                    (c1_active_this_epoch || c1_hazard_was_last);

        // Has the c1 address been seen in c0 already or are the new c0
        // and c1 requests to the same address?
        c1_hazard = cci_mpf_c1TxIsWriteReq(c1Tx[3]) &&
                    c1Tx[3].hdr.ext.checkLoadStoreOrder &&
                    ((! c1_addr_inactive && (c0_active_this_epoch ||
                                             cci_mpf_c0TxIsReadReq(c0Tx[3]))) ||
                     (cci_mpf_c0TxIsReadReq(c0Tx[3]) &&
                      c0Tx[3].hdr.ext.checkLoadStoreOrder &&
                      (c0_filter_idx[3] == c1_filter_idx[3])));
    end

    always_ff @(posedge clk)
    begin
        if (c0_hazard)
        begin
            c1_hazard_was_last <= 1'b0;
        end
        if (c1_hazard)
        begin
            c1_hazard_was_last <= 1'b1;
        end

        c0_active_this_epoch <=
            (cci_mpf_c0TxIsReadReq(c0Tx[3]) && ! c1_hazard) ||
            (! c0_hazard && ! c1_hazard && c0_active_this_epoch);

        c1_active_this_epoch <=
            cci_mpf_c1TxIsWriteReq(c1Tx[3]) ||
            (! c0_hazard && ! c1_hazard && c1_active_this_epoch);

        if (reset)
        begin
            c1_hazard_was_last <= 1'b0;

            c0_active_this_epoch <= 1'b0;
            c1_active_this_epoch <= 1'b0;
        end
    end

    always_comb
    begin
        // Increment epoch if any hazards are detected
        c0Tx_incr_epoch = c0_hazard;
        c1Tx_incr_epoch = c1_hazard;

        c0Tx_en = cci_mpf_c0TxIsValid(c0Tx[3]);
        c1Tx_en = cci_mpf_c1TxIsValid(c1Tx[3]);
    end

endmodule // cci_mpf_shim_buffer_afu_epoch_hazards
