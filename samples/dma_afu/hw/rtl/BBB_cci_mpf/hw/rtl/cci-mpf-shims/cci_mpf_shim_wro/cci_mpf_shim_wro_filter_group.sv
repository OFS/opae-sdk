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
`include "cci_mpf_shim_wro.vh"
`include "cci_mpf_prim_hash.vh"


//
// Manage filters associated with a group of requests that must be kept
// ordered.  Individual virtual channels are one example of a potential
// group.  In this case, distinct CCI virtual channels have no order
// guarantees, even when tracking write responses.  It thus makes sense
// to limit filter sizes by tracking virtual channels as separate groups.
//
module cci_mpf_shim_wro_filter_group
  #(
    // Size of an address hash entry. Smaller sizes take less space but
    // increase the probability of address collisions.
    parameter ADDRESS_HASH_BITS = 12,

    parameter AFU_BUF_THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,

    // MPF guarantees that the low mdata bits in read and write requests
    // are unique in a space large enough to represent MAX_ACTIVE_REQS.
    // This module uses the indices to store metadata for use in
    // processing responses and thus avoids having to manage heap
    // space allocation.
    parameter MAX_ACTIVE_REQS = 128
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    cci_mpf_csrs.wro csrs,
    cci_mpf_csrs.wro_events events
    );

    logic reset;
    assign reset = fiu.reset;

    localparam FILTER_PIPE_DEPTH = 4;

    // ====================================================================
    //
    //  Instantiate a buffer on the AFU request port, making it latency
    //  insensitive.  The buffer also enforces order such that no two
    //  conflicting requests are active in the outbound WRO pipeline at
    //  the same time.
    //
    // ====================================================================

    cci_mpf_if afu_buf (.clk);
    cci_mpf_if afu_filtered (.clk);
    cci_mpf_if fiu_buf (.clk);

    // Does the WRO pipeline here have active requests?
    logic pipe_notEmpty;
    logic c0_pipe_notEmpty;
    logic c1_pipe_notEmpty;

    // Latency-insensitive ports need explicit dequeue (enable).
    logic c0_afu_deq;
    logic c1_afu_deq;

    // Addresses active in the write request pipeline
    logic c1_pipe_valid [0 : FILTER_PIPE_DEPTH-1];
    logic [ADDRESS_HASH_BITS-1 : 0] c1_pipe_addrHash [0 : FILTER_PIPE_DEPTH-1];
    t_cci_mpf_wro_line_mask c1_pipe_lineMask [0 : FILTER_PIPE_DEPTH-1];

    cci_mpf_shim_wro_epoch_order
      #(
        .AFU_BUF_THRESHOLD(AFU_BUF_THRESHOLD),
        .ADDRESS_HASH_BITS(ADDRESS_HASH_BITS),
        .PIPE_DEPTH(FILTER_PIPE_DEPTH)
        )
      epoch_order
       (
        .clk,

        // Incoming from AFU
        .afu(afu),
        // Buffered and epoch's tagged (from bufafu)
        .client_afu(afu_buf),
        // Addresses filtered (from pipeline here toward bufafu)
        .client_fiu(afu_filtered),
        // FIU-bound processing in bufafu complete (from bufafu toward fiu) 
        .fiu(fiu_buf),

        .active_notEmpty(pipe_notEmpty),
        .active_c0Tx_notEmpty(c0_pipe_notEmpty),
        .active_c1Tx_notEmpty(c1_pipe_notEmpty),

        // States of existing requests in the write pipeline
        .active_valid(c1_pipe_valid),
        .active_addrHash(c1_pipe_addrHash),
        .active_lineMask(c1_pipe_lineMask),

        .deqC0Tx(c0_afu_deq),
        .deqC1Tx(c1_afu_deq)
        );

    assign afu_buf.reset = afu_filtered.reset;

    //
    // Almost full signals in the buffered input are ignored --
    // replaced by deq signals and the buffer state.  Set them
    // to 1 to be sure they are ignored.
    //
    assign afu_buf.c0TxAlmFull = 1'b1;
    assign afu_buf.c1TxAlmFull = 1'b1;


    // ====================================================================
    //
    // Hash addresses into smaller values to reduce storage and comparison
    // overhead.
    //
    // ====================================================================

    typedef logic [ADDRESS_HASH_BITS-1 : 0] t_hash;

    function automatic t_hash hashAddr(t_cci_clAddr addr);
        return t_hash'(hash32(addr[$bits(t_cci_clLen) +: 32]));
    endfunction

    t_hash c0_hash;
    t_hash c1_hash;
    t_cci_mpf_wro_line_mask c1_line_mask;

    assign c0_hash = hashAddr(cci_mpf_c0_getReqAddr(afu_buf.c0Tx.hdr));
    assign c1_hash = hashAddr(cci_mpf_c1_getReqAddr(afu_buf.c1Tx.hdr));
    assign c1_line_mask = wroWrFilterLineMask(afu_buf.c1Tx.hdr);


    // ====================================================================
    //
    //  Instantiate a buffer on the FIU response port to give time to
    //  read local state in block RAMs before forwarding the response
    //  toward the AFU.
    //
    // ====================================================================

    cci_mpf_shim_buffer_fiu
      #(
        // Add a register on output for timing
        .REGISTER_OUTBOUND(1),
        .N_RX_REG_STAGES(2)
        )
      buffiu
       (
        .clk,
        .fiu_raw(fiu),
        .fiu_buf(fiu_buf)
        );


    // ====================================================================
    //
    //  Filter to track busy addresses.
    //
    // ====================================================================

    // Final stage in the pipeline
    localparam FILTER_PIPE_LAST = FILTER_PIPE_DEPTH-1;
    // Stage in which filter lookups are tested
    localparam FILTER_PIPE_TEST = FILTER_PIPE_DEPTH-2;

    typedef logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] t_heap_idx;

    // There are two sets of filters: one for reads and one for writes.
    logic [0 : 1] rd_filter_test_notPresent;
    logic [0 : 1] wr_filter_test_notPresent;

    // One hash for each request channel
    t_hash [0 : 1] filter_test_req;
    logic  [0 : 1] filter_test_req_en;

    // Insert new active reads and writes in the filter.
    t_hash rd_filter_insert_hash;
    logic rd_filter_insert_en;

    t_hash wr_filter_insert_hash;
    t_cci_mpf_wro_line_mask wr_filter_insert_mask;
    logic wr_filter_insert_en;

    // Read response handling on channel 0.
    t_hash rd_filter_remove_hash;
    logic rd_filter_remove_en;

    // Write response handling on channel 1.
    t_hash wr_filter_remove_hash;
    t_cci_mpf_wro_line_mask wr_filter_remove_mask;
    logic wr_filter_remove_en;

    logic rd_filter_rdy;
    logic wr_filter_rdy;
    logic filter_rdy;

    always_ff @(posedge clk)
    begin
        filter_rdy <= rd_filter_rdy && wr_filter_rdy;
    end

    //
    // Generate the read and write filters.
    //

    // Selecting the number of filter bits is a tension between block RAM
    // usage and false positives due to the counters being too small.
    //
    // The filter must be at least 3 bits in order to avoid the risk
    // of overflowing!  This is because multiple reads may be in the pipeline
    // and all checking whether they can be added to the filter.  If all
    // reads are to the same address then the counter may be incremented up
    // to FILTER_PIPE_DEPTH-1 above the current state.  Using the counter
    // this way is far easier than address matching and detecting multipel
    // references to the same address in the pipeline.
    localparam RD_FILTER_BUCKET_BITS = 5;
    logic [RD_FILTER_BUCKET_BITS-1 : 0] rd_filter_test_cnt;

    // Consider the filter full if less than FILTER_PIPE_DEPTH slots are
    // available.
    localparam RD_FILTER_BUCKET_LIMIT =
        (1 << RD_FILTER_BUCKET_BITS) - (FILTER_PIPE_DEPTH + 1);
    assign rd_filter_test_notPresent[0] =
        (rd_filter_test_cnt[RD_FILTER_BUCKET_BITS-1] <
         (RD_FILTER_BUCKET_BITS'(RD_FILTER_BUCKET_LIMIT)));

    //
    // A counting filter allows multiple reads to the same address to be
    // in flight.
    //
    cci_mpf_prim_filter_banked_counting
      #(
        .N_BANKS_RADIX(1),
        .N_BUCKETS(1 << ADDRESS_HASH_BITS),
        .BITS_PER_BUCKET(RD_FILTER_BUCKET_BITS),
        .N_TEST_VALUE_CLIENTS(1),
        .N_TEST_IS_ZERO_CLIENTS(1),
        .N_REMOVE_FIFO_ENTRIES(MAX_ACTIVE_REQS)
        )
      rdFilter
       (
        .clk,
        .reset,
        .rdy(rd_filter_rdy),

        .test_value_req(filter_test_req[0]),
        .test_value_en(filter_test_req_en[0]),
        .T2_test_value(rd_filter_test_cnt),
        .test_isZero_req(filter_test_req[1]),
        .test_isZero_en(filter_test_req_en[1]),
        .T2_test_isZero(rd_filter_test_notPresent[1]),

        .insert_en(rd_filter_insert_en),
        .insert(rd_filter_insert_hash),

        .remove_en(rd_filter_remove_en),
        .remove(rd_filter_remove_hash),

        // N_REMOVE_FIFO_ENTRIES is large enough that we can ignore notFull
        .remove_notFull(),
        .remove_almostFull()
        );

    //
    // A decode filter allows at most one store per address at a time.
    // We consider only addresses that can be accessed in the largest
    // multi-line request, forcing multi-line requests to lock out accesses
    // of any size to the same region.  The filter has multiple bits that
    // can be used to mask single lines within an aligned multi-line group.
    // This allows multiple small stores within what could have been a
    // larger multi-line store to be active simultaneously while still
    // locking out all loads to the region.
    //
    t_cci_mpf_wro_line_mask wr_filter_test_active_mask;

    cci_mpf_prim_filter_banked_decode
      #(
        .N_BANKS_RADIX(1),
        .N_BUCKETS(1 << ADDRESS_HASH_BITS),
        // CCI permits up to 4 beat write requests
        .BITS_PER_BUCKET(4),
        .N_TEST_VALUE_CLIENTS(1),
        .N_TEST_IS_ZERO_CLIENTS(1),
        .N_REMOVE_FIFO_ENTRIES(MAX_ACTIVE_REQS)
        )
      wrFilter
       (
        .clk,
        .reset,
        .rdy(wr_filter_rdy),

        .test_value_req(filter_test_req[1]),
        .test_value_en(filter_test_req_en[1]),
        .T2_test_value(wr_filter_test_active_mask),
        .test_isZero_req(filter_test_req[0]),
        .test_isZero_en(filter_test_req_en[0]),
        .T2_test_isZero(wr_filter_test_notPresent[0]),

        .insert_en(wr_filter_insert_en),
        .insert_idx(wr_filter_insert_hash),
        .insert_mask(wr_filter_insert_mask),

        .remove_en(wr_filter_remove_en),
        .remove_idx(wr_filter_remove_hash),
        .remove_mask(wr_filter_remove_mask),

        // N_REMOVE_FIFO_ENTRIES is large enough that we can ignore notFull
        .remove_notFull()
        );


    // Hold the hashed address associated with the buffered test result.
    // This is used only in assertions below and should be dropped
    // during dead code elimination when synthesized.
    t_hash [0 : 1] filter_verify_req[1 : FILTER_PIPE_TEST];
    logic  [0 : 1] filter_verify_req_en[1 : FILTER_PIPE_TEST];

    always_ff @(posedge clk)
    begin
        filter_verify_req[1] <= filter_test_req;
        filter_verify_req_en[1] <= filter_test_req_en;

        for (int i = 2; i <= FILTER_PIPE_TEST; i = i + 1)
        begin
            filter_verify_req[i] <= filter_verify_req[i - 1];
            filter_verify_req_en[i] <= filter_verify_req_en[i - 1];
        end

        if (reset)
        begin
            for (int i = 1; i <= FILTER_PIPE_TEST; i = i + 1)
            begin
                filter_verify_req_en[i] <= 2'b0;
            end
        end
    end


    // ====================================================================
    //
    //  Heaps hold state to remove entries in the filters as responses
    //  arrive.
    //
    // ====================================================================

    typedef struct packed
    {
        // Hash is the index in the decode filter
        t_hash addrHash;

        // Enforce order?
        logic enforceOrder;
    }
    t_c0_heap_entry;

    t_c0_heap_entry c0_heap_enqData;
    t_heap_idx c0_heap_reqIdx;
    assign c0_heap_reqIdx = t_heap_idx'(afu_filtered.c0Tx.hdr.base.mdata);

    t_heap_idx c0_heap_readReq;
    t_c0_heap_entry c0_heap_readRsp;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(MAX_ACTIVE_REQS),
        .N_DATA_BITS($bits(t_c0_heap_entry)),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0)
        )
      c0_heap
       (
        .clk,

        .wen(cci_mpf_c0TxIsReadReq(afu_filtered.c0Tx)),
        .waddr(c0_heap_reqIdx),
        .wdata(c0_heap_enqData),

        .raddr(c0_heap_readReq),
        .rdata(c0_heap_readRsp)
        );


    //
    // The channel 1 (write request) heap.
    //

    typedef struct packed
    {
        // Hash is the index in the decode filter
        t_hash addrHash;

        // Which lines in the aligned multi-line group are written?
        t_cci_mpf_wro_line_mask lineMask;

        // Enforce order?
        logic enforceOrder;
    }
    t_c1_heap_entry;

    t_c1_heap_entry c1_heap_enqData;
    t_heap_idx c1_heap_reqIdx;
    assign c1_heap_reqIdx = t_heap_idx'(afu_filtered.c1Tx.hdr.base.mdata);

    t_heap_idx c1_heap_readReq;
    t_c1_heap_entry c1_heap_readRsp;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(MAX_ACTIVE_REQS),
        .N_DATA_BITS($bits(t_c1_heap_entry)),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0)
        )
      c1_heap
       (
        .clk,

        .wen(cci_mpf_c1TxIsWriteReq(afu_filtered.c1Tx)),
        .waddr(c1_heap_reqIdx),
        .wdata(c1_heap_enqData),

        .raddr(c1_heap_readReq),
        .rdata(c1_heap_readRsp)
        );


    // ====================================================================
    //
    //  Filtering pipeline
    //
    // ====================================================================

    //
    // Request data flowing through filtering pipeline
    //
    typedef struct packed
    {
        t_if_cci_mpf_c0_Tx c0Tx;
        t_hash             c0AddrHash;
    }
    t_c0_request_pipe;

    typedef struct packed
    {
        t_if_cci_mpf_c1_Tx c1Tx;
        t_hash c1AddrHash;
        t_cci_mpf_wro_line_mask c1LineMask;
    }
    t_c1_request_pipe;

    // Pipeline stage storage
    t_c0_request_pipe c0_afu_pipe[0 : FILTER_PIPE_LAST];
    t_c1_request_pipe c1_afu_pipe[0 : FILTER_PIPE_LAST];

    logic c0_process_requests;
    logic c1_process_requests;

    //
    // Export some state to the epoch filter
    //
    always_comb
    begin
        for (int i = 0; i < FILTER_PIPE_DEPTH; i = i + 1)
        begin
            c1_pipe_valid[i] = cci_mpf_c1TxIsValid(c1_afu_pipe[i].c1Tx) &&
                               c1_afu_pipe[i].c1Tx.hdr.ext.checkLoadStoreOrder;
            c1_pipe_addrHash[i] = c1_afu_pipe[i].c1AddrHash;
            c1_pipe_lineMask[i] = c1_afu_pipe[i].c1LineMask;
        end
    end

    // ====================================================================
    //
    // Consume the filter lookups and register their results for use in
    // the next cycle.
    //
    // ====================================================================

    // Does the request want order to be enforced?
    logic c0_enforce_order;
    assign c0_enforce_order = cci_mpf_c0_getReqCheckOrder(c0_afu_pipe[FILTER_PIPE_TEST].c0Tx.hdr);
    logic c1_enforce_order;
    assign c1_enforce_order = cci_mpf_c1_getReqCheckOrder(c1_afu_pipe[FILTER_PIPE_TEST].c1Tx.hdr);

    //
    // Compute whether new requests can be inserted into the filters.
    // c0 is read requests, c1 is write requests.
    //
    logic c0_filter_may_insert;
    always_ff @(posedge clk)
    begin
        c0_filter_may_insert <= (rd_filter_test_notPresent[0] &&
                                 wr_filter_test_notPresent[0]) ||
                                ! c0_enforce_order;
    end

    // The write filter is indexed by 4 line aligned addresses.  The filter
    // holds 4 bits corresponding to the lines within the 4 line group.
    // A new entry is considered not present if the current filter state
    // shows no activity in the lines written by the new request.  c1LineMask
    // indicates the lines within the group written by the new request.
    assign wr_filter_test_notPresent[1] =
        ~(|(wr_filter_test_active_mask & c1_afu_pipe[FILTER_PIPE_TEST].c1LineMask));

    logic c1_filter_may_insert;
    always_ff @(posedge clk)
    begin
        c1_filter_may_insert <= (rd_filter_test_notPresent[1] &&
                                 wr_filter_test_notPresent[1]) ||
                                ! c1_enforce_order;
    end

    // Events
    always_ff @(posedge clk)
    begin
        events.wro_out_event_rr_conflict <=
            cci_mpf_c0TxIsValid(c0_afu_pipe[FILTER_PIPE_TEST].c0Tx) &&
            c0_enforce_order &&
            ! rd_filter_test_notPresent[0];
        events.wro_out_event_rw_conflict <=
            cci_mpf_c0TxIsValid(c0_afu_pipe[FILTER_PIPE_TEST].c0Tx) &&
            c0_enforce_order &&
            ! wr_filter_test_notPresent[0];
        events.wro_out_event_wr_conflict <=
            cci_mpf_c1TxIsValid(c1_afu_pipe[FILTER_PIPE_TEST].c1Tx) &&
            c1_enforce_order &&
            ! rd_filter_test_notPresent[1];
        events.wro_out_event_ww_conflict <=
            cci_mpf_c1TxIsValid(c1_afu_pipe[FILTER_PIPE_TEST].c1Tx) &&
            c1_enforce_order &&
            ! wr_filter_test_notPresent[1];
    end


    //
    // This pipeline has complicated control flow.  Confirm that
    // decisions made this cycle were based on the correct addresses.
    //

    // synthesis translate_off
    always_ff @(posedge clk)
    begin
        if (! reset && c0_process_requests)
        begin
            assert ((! filter_verify_req_en[FILTER_PIPE_TEST][0] || (filter_verify_req[FILTER_PIPE_TEST][0] === c0_afu_pipe[FILTER_PIPE_TEST].c0AddrHash)) &&
                    (filter_verify_req_en[FILTER_PIPE_TEST][0] == cci_mpf_c0TxIsReadReq(c0_afu_pipe[FILTER_PIPE_TEST].c0Tx))) else
                $fatal("cci_mpf_shim_wro: Incorrect c0 pipeline control");
        end
        if (! reset && c1_process_requests)
        begin
            assert ((! filter_verify_req_en[FILTER_PIPE_TEST][1] || (filter_verify_req[FILTER_PIPE_TEST][1] === c1_afu_pipe[FILTER_PIPE_TEST].c1AddrHash)) &&
                    (filter_verify_req_en[FILTER_PIPE_TEST][1] == cci_mpf_c1TxIsWriteReq(c1_afu_pipe[FILTER_PIPE_TEST].c1Tx))) else
                $fatal("cci_mpf_shim_wro: Incorrect c1 pipeline control");
        end
    end
    // synthesis translate_on


    // ====================================================================
    //
    // Work backwards in the pipeline.  First decide whether the oldest
    // request can fire.  If it can (or there is no request) then younger
    // requests will ripple through the pipeline.
    //
    // ====================================================================

    // Is either AFU making a request?
    logic c0_request_rdy;
    assign c0_request_rdy = cci_mpf_c0TxIsValid(c0_afu_pipe[FILTER_PIPE_LAST].c0Tx);

    logic c1_request_rdy;
    assign c1_request_rdy = cci_mpf_c1TxIsValid(c1_afu_pipe[FILTER_PIPE_LAST].c1Tx);


    // Is a request blocked by inability to forward it to the FIU or a
    // conflict?
    logic c0_blocked;
    logic c0_full_downstream;
    assign c0_blocked = (c0_full_downstream || ! c0_filter_may_insert);

    logic c1_blocked;
    logic c1_full_downstream;
    assign c1_blocked = (c1_full_downstream || ! c1_filter_may_insert);

    always_ff @(posedge clk)
    begin
        c0_full_downstream <= (fiu.c0TxAlmFull || ! filter_rdy);
        c1_full_downstream <= (fiu.c1TxAlmFull || ! filter_rdy);
    end

    // Process requests if one exists on either channel.  Requests are only
    // allowed to enter afu_pipe if they are independent of all other
    // requests active in afu_pipe.  We can thus reorder requests in
    // afu_pipe arbitrarily without violating inter-line ordering.
    assign c0_process_requests = (c0_request_rdy && ! c0_blocked);
    assign c1_process_requests = (c1_request_rdy && ! c1_blocked);

    // Set the hashed value to insert in the filter when requests are
    // processed.
    assign rd_filter_insert_hash = c0_afu_pipe[FILTER_PIPE_LAST].c0AddrHash;
    assign rd_filter_insert_en = c0_process_requests &&
                                 cci_mpf_c0_getReqCheckOrder(c0_afu_pipe[FILTER_PIPE_LAST].c0Tx.hdr);

    assign wr_filter_insert_hash = c1_afu_pipe[FILTER_PIPE_LAST].c1AddrHash;
    assign wr_filter_insert_mask = c1_afu_pipe[FILTER_PIPE_LAST].c1LineMask;
    assign wr_filter_insert_en = c1_process_requests &&
                                 cci_mpf_c1_getReqCheckOrder(c1_afu_pipe[FILTER_PIPE_LAST].c1Tx.hdr);

    //
    // Now that we know whether the oldest request was processed we can
    // manage flow through the pipeline.
    //

    // Advance if the oldest request was processed or the last stage is empty.
    logic c0_advance_pipeline;
    assign c0_advance_pipeline = (! c0_blocked || ! c0_request_rdy);
    logic c1_advance_pipeline;
    assign c1_advance_pipeline = (! c1_blocked || ! c1_request_rdy);

    // Update the pipeline
    t_c0_request_pipe c0_afu_pipe_init;
    t_c1_request_pipe c1_afu_pipe_init;
    logic c0_swap_entries;
    logic c1_swap_entries;

    // Is the incoming pipeline moving?
    assign c0_afu_deq = c0_advance_pipeline &&
                        cci_mpf_c0TxIsValid(c0_afu_pipe_init.c0Tx);
    assign c1_afu_deq = c1_advance_pipeline &&
                        cci_mpf_c1TxIsValid(c1_afu_pipe_init.c1Tx);

    always_ff @(posedge clk)
    begin
        if (c0_advance_pipeline)
        begin
            c0_afu_pipe[0] <= c0_afu_pipe_init;
            for (int i = 1; i < FILTER_PIPE_DEPTH; i = i + 1)
            begin
                c0_afu_pipe[i] <= c0_afu_pipe[i - 1];
            end
        end
        else if (c0_swap_entries)
        begin
            // Oldest was blocked.  Try moving a newer entry around the
            // oldest.  They have been proven to be independent.
            c0_afu_pipe[0] <= c0_afu_pipe[FILTER_PIPE_LAST];
            for (int i = 1; i < FILTER_PIPE_DEPTH; i = i + 1)
            begin
                c0_afu_pipe[i] <= c0_afu_pipe[i - 1];
            end
        end
        else if (c0_process_requests)
        begin
            // Pipeline restarted after a bubble. Drop the request
            // that left the pipeline but don't advance yet so the
            // filter pipeline can catch up.
            c0_afu_pipe[FILTER_PIPE_LAST].c0Tx <= cci_mpf_c0Tx_clearValids();
        end

        // Same as c0 algorithm above...
        if (c1_advance_pipeline)
        begin
            c1_afu_pipe[0] <= c1_afu_pipe_init;
            for (int i = 1; i < FILTER_PIPE_DEPTH; i = i + 1)
            begin
                c1_afu_pipe[i] <= c1_afu_pipe[i - 1];
            end
        end
        else if (c1_swap_entries)
        begin
            // Oldest was blocked.  Try moving a newer entry around the
            // oldest.  They have been proven to be independent.
            c1_afu_pipe[0] <= c1_afu_pipe[FILTER_PIPE_LAST];
            for (int i = 1; i < FILTER_PIPE_DEPTH; i = i + 1)
            begin
                c1_afu_pipe[i] <= c1_afu_pipe[i - 1];
            end
        end
        else if (c1_process_requests)
        begin
            // Pipeline restarted after a bubble. Drop the request
            // that left the pipeline but don't advance yet so the
            // filter pipeline can catch up.
            c1_afu_pipe[FILTER_PIPE_LAST].c1Tx <= cci_mpf_c1Tx_clearValids();
        end

        if (reset)
        begin
            for (int i = 0; i < FILTER_PIPE_DEPTH; i = i + 1)
            begin
                c0_afu_pipe[i].c0Tx <= cci_mpf_c0Tx_clearValids();
                c1_afu_pipe[i].c1Tx <= cci_mpf_c1Tx_clearValids();
            end
        end
    end

    //
    // Will the pipeline be empty next cycle?
    //
    logic c0_pipe_notEmpty_next;
    logic c1_pipe_notEmpty_next;

    always_ff @(posedge clk)
    begin
        pipe_notEmpty <= c0_pipe_notEmpty_next || c1_pipe_notEmpty_next;
        c0_pipe_notEmpty <= c0_pipe_notEmpty_next;
        c1_pipe_notEmpty <= c1_pipe_notEmpty_next;
    end

    always_comb
    begin
        // New request coming into the pipeline?
        c0_pipe_notEmpty_next = c0_afu_deq;
        for (int i = 0; i < FILTER_PIPE_DEPTH; i = i + 1)
        begin
            // Check all pipeline slots
            c0_pipe_notEmpty_next = c0_pipe_notEmpty_next ||
                                    cci_mpf_c0TxIsValid(c0_afu_pipe[i].c0Tx);
        end

        // Same for c1
        c1_pipe_notEmpty_next = c1_afu_deq;
        for (int i = 0; i < FILTER_PIPE_DEPTH; i = i + 1)
        begin
            c1_pipe_notEmpty_next = c1_pipe_notEmpty_next ||
                                    cci_mpf_c1TxIsValid(c1_afu_pipe[i].c1Tx);
        end
    end


    always_comb
    begin
        //
        // Hold back new requests when the epoch changes until the request
        // pipeline drains.
        //
        c0_afu_pipe_init.c0Tx = afu_buf.c0Tx;
        c0_afu_pipe_init.c0Tx.hdr.ext.checkLoadStoreOrder =
            afu_buf.c0Tx.valid && afu_buf.c0Tx.hdr.ext.checkLoadStoreOrder;
        c0_afu_pipe_init.c0AddrHash = c0_hash;

        c1_afu_pipe_init.c1Tx = afu_buf.c1Tx;
        c1_afu_pipe_init.c1Tx.hdr.ext.checkLoadStoreOrder =
            afu_buf.c1Tx.valid && afu_buf.c1Tx.hdr.ext.checkLoadStoreOrder;
        c1_afu_pipe_init.c1AddrHash = c1_hash;
        c1_afu_pipe_init.c1LineMask = c1_line_mask;
    end


    //
    // Update the addresses being tested in the filter.  The test value
    // for the filter is stored in a register.  Combinational logic could
    // be used but it would add a mux to the head of the already expensive
    // filter.
    //

    // If the pipeline is about to block try swapping the entries to avoid
    // blocking.
    assign c0_swap_entries = c0_request_rdy && c0_blocked;
    assign c1_swap_entries = c1_request_rdy && c1_blocked;

    always_ff @(posedge clk)
    begin
        if (c0_swap_entries)
        begin
            // The oldest entry is moving to the position of the newest.
            // Put its filtering request back in the pipeline.
            filter_test_req[0] <= c0_afu_pipe[FILTER_PIPE_LAST].c0AddrHash;
            filter_test_req_en[0] <= cci_mpf_c0TxIsReadReq(c0_afu_pipe[FILTER_PIPE_LAST].c0Tx);
        end
        else
        begin
            // Normal, pipelined flow.  Next cycle we will test the value
            // being written to the first stage of the pipeline.
            filter_test_req[0] <= c0_hash;
            filter_test_req_en[0] <= cci_mpf_c0TxIsReadReq(c0_afu_pipe_init.c0Tx);
        end

        if (c1_swap_entries)
        begin
            // The oldest entry is moving to the position of the newest.
            // Put its filtering request back in the pipeline.
            filter_test_req[1] <= c1_afu_pipe[FILTER_PIPE_LAST].c1AddrHash;
            filter_test_req_en[1] <= cci_mpf_c1TxIsWriteReq(c1_afu_pipe[FILTER_PIPE_LAST].c1Tx);
        end
        else
        begin
            // Normal, pipelined flow.  Next cycle we will test the value
            // being written to the first stage of the pipeline.
            filter_test_req[1] <= c1_hash;
            filter_test_req_en[1] <= cci_mpf_c1TxIsWriteReq(c1_afu_pipe_init.c1Tx);
        end

        if (reset)
        begin
            filter_test_req_en <= 0;
        end
    end


    // ====================================================================
    //
    //  Channel 0 (read)
    //
    // ====================================================================

    assign afu_filtered.c0Tx =
        cci_mpf_c0TxMaskValids(c0_afu_pipe[FILTER_PIPE_LAST].c0Tx,
                               c0_process_requests);

    // Save state that will be used when the response is returned.
    always_comb
    begin
        c0_heap_enqData.addrHash = rd_filter_insert_hash;
        c0_heap_enqData.enforceOrder = cci_mpf_c0_getReqCheckOrder(c0_afu_pipe[FILTER_PIPE_LAST].c0Tx.hdr);
    end

    // Request heap read as fiu responses arrive.  The heap's value will be
    // available the cycle fiu_buf is read.
    always_comb
    begin
        c0_heap_readReq = t_heap_idx'(fiu.c0Rx.hdr.mdata);
    end

    // Responses
    assign afu_buf.c0Rx = fiu.c0Rx;

    // Remove the entry from the filter
    always_ff @(posedge clk)
    begin
        rd_filter_remove_hash <= c0_heap_readRsp.addrHash;
        rd_filter_remove_en <= cci_c0Rx_isReadRsp(fiu_buf.c0Rx) &&
                               cci_mpf_c0Rx_isEOP(fiu_buf.c0Rx) &&
                               c0_heap_readRsp.enforceOrder;

        if (reset)
        begin
            rd_filter_remove_en <= 1'b0;
        end
    end


    // ====================================================================
    //
    //  Channel 1 (write)
    //
    // ====================================================================

    assign afu_filtered.c1Tx =
        cci_mpf_c1TxMaskValids(c1_afu_pipe[FILTER_PIPE_LAST].c1Tx,
                               c1_process_requests);

    // Save state that will be used when the response is returned.
    always_comb
    begin
        c1_heap_enqData.addrHash = wr_filter_insert_hash;
        c1_heap_enqData.lineMask = wr_filter_insert_mask;
        c1_heap_enqData.enforceOrder = cci_mpf_c1_getReqCheckOrder(c1_afu_pipe[FILTER_PIPE_LAST].c1Tx.hdr);
    end

    // Request heap read as fiu responses arrive. The heap's value will be
    // available the cycle fiu_buf is read.
    always_comb
    begin
        c1_heap_readReq = t_heap_idx'(fiu.c1Rx.hdr.mdata);
    end

    // Responses
    assign afu_buf.c1Rx = fiu.c1Rx;

    // Remove the entry from the filter
    always_ff @(posedge clk)
    begin
        wr_filter_remove_hash <= c1_heap_readRsp.addrHash;
        wr_filter_remove_mask <= c1_heap_readRsp.lineMask;
        wr_filter_remove_en <= cci_c1Rx_isWriteRsp(fiu_buf.c1Rx) &&
                               c1_heap_readRsp.enforceOrder;

        if (reset)
        begin
            wr_filter_remove_en <= 1'b0;
        end
    end


`ifdef DEBUG_MESSAGES
    t_heap_idx c0_prev_heap_reqIdx;
    t_heap_idx c1_prev_heap_reqIdx;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            c0_prev_heap_reqIdx <= 0;
            c1_prev_heap_reqIdx <= 0;
        end
        else
        begin
            if (c0_request_rdy && c0_blocked && ! c0_filter_may_insert && (c0_heap_reqIdx != c0_prev_heap_reqIdx))
            begin
                c0_prev_heap_reqIdx <= c0_heap_reqIdx;
                $display("C0 blocked %d", c0_heap_reqIdx);
            end
            if (c0_request_rdy && (c0_heap_reqIdx != c0_prev_heap_reqIdx))
            begin
                c0_prev_heap_reqIdx <= c0_heap_reqIdx;
                $display("C0 heap full %d", c0_heap_reqIdx);
            end

            if (c1_request_rdy && c1_blocked && ! c1_filter_may_insert && (c1_heap_reqIdx != c1_prev_heap_reqIdx))
            begin
                c1_prev_heap_reqIdx <= c1_heap_reqIdx;
                $display("C1 blocked %d", c1_heap_reqIdx);
            end
            if (c1_request_rdy && (c1_heap_reqIdx != c1_prev_heap_reqIdx))
            begin
                c1_prev_heap_reqIdx <= c1_heap_reqIdx;
                $display("C1 heap full %d", c1_heap_reqIdx);
            end

            if (cci_mpf_c0TxIsReadReq(afu_filtered.c0Tx))
            begin
                $display("XX A 0 %d %x %t",
                         c0_heap_reqIdx,
                         cci_mpf_c0_getReqAddr(afu_filtered.c0Tx.hdr),
                         $time);
            end
            if (cci_mpf_c1TxIsWriteReq(afu_filtered.c1Tx))
            begin
                $display("XX A 1 %d %x hash 0x%x %t",
                         c1_heap_reqIdx,
                         cci_mpf_c1_getReqAddr(afu_filtered.c1Tx.hdr),
                         wr_filter_insert_hash,
                         $time);
            end

            if (wr_filter_remove_en)
            begin
                $display("XX F 1 hash 0x%x %t",
                         wr_filter_remove_hash,
                         $time);
            end

            if (filter_test_req_en)
            begin
                $display("XX F T %0d hash 0x%x / %0d 0x%x",
                         filter_test_req_en[0],
                         filter_test_req[0],
                         filter_test_req_en[1],
                         filter_test_req[1]);
            end
        end
    end
`endif


    // ====================================================================
    //
    // Channel 2 Tx (MMIO read response) flows straight through.
    //
    // ====================================================================

    assign afu_filtered.c2Tx = afu_buf.c2Tx;

endmodule // cci_mpf_shim_wro_filter_group
