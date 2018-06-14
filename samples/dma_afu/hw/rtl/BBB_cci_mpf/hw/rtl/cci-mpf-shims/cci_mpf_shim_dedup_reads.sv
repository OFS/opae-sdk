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


//
// Deduplicate nearby reads that are repetions of the same read request into
// a single CCI read.
//


module cci_mpf_shim_dedup_reads
  #(
    // Maximum number in-flight requests per channel.
    parameter MAX_ACTIVE_REQS = 128
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu
    );

    localparam MAX_CHAIN_LEN = 32;
    localparam MAX_CHAINED_REQS = (MAX_ACTIVE_REQS >> 1);

    localparam DEBUG_MESSAGES = 0;

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    typedef logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] t_heap_idx;
    typedef logic [$clog2(MAX_CHAIN_LEN)-1 : 0] t_chain_len;
    typedef logic [$clog2(MAX_CHAINED_REQS)-1 : 0] t_chained_req_cnt;

    logic chain_rsp_buffer_almFull;

    // Pass-through channels
    assign fiu.c1Tx = afu.c1Tx;
    assign fiu.c2Tx = afu.c2Tx;
    assign afu.c1Rx = fiu.c1Rx;
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull || chain_rsp_buffer_almFull;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull;

    //
    // Track read requests and detect duplicates
    //
    logic c0Tx_match_q, c0Tx_match_qq;
    t_heap_idx c0Tx_match_head_idx_q;
    t_heap_idx c0Tx_match_tail_idx_q, c0Tx_match_tail_idx_qq;
    t_chain_len c0Tx_match_chain_len_q;

    cci_mpf_shim_dedup_reads_tracker
      #(
        .MAX_ACTIVE_REQS(MAX_ACTIVE_REQS),
        .MAX_CHAIN_LEN(MAX_CHAIN_LEN),
        .DEBUG_MESSAGES(DEBUG_MESSAGES)
        )
      tracker
       (
        .clk,
        .reset,
        .c0Tx(afu.c0Tx),
        .c0Rx(fiu.c0Rx),
        .c0Tx_match_q,
        .c0Tx_match_head_idx_q,
        .c0Tx_match_tail_idx_q,
        .c0Tx_match_chain_len_q
        );

    //
    // Forward only unique requests to the FIU.
    //
    t_if_cci_mpf_c0_Tx c0Tx_q, c0Tx_qq;

    always_ff @(posedge clk)
    begin
        c0Tx_q <= afu.c0Tx;
        fiu.c0Tx <= cci_mpf_c0TxMaskValids(c0Tx_q, ! c0Tx_match_q);

        // c0Tx_qq is used internally by some updates that have delayed actions
        c0Tx_qq <= c0Tx_q;
        c0Tx_match_qq <= c0Tx_match_q;
        c0Tx_match_tail_idx_qq <= c0Tx_match_tail_idx_q;

        if (reset)
        begin
            c0Tx_q <= cci_mpf_c0Tx_clearValids();
            c0Tx_qq <= cci_mpf_c0Tx_clearValids();
            fiu.c0Tx <= cci_mpf_c0Tx_clearValids();
        end
    end


    // ====================================================================
    //
    //  Store chains as a linked list of heap index values originating at
    //  the heap index of the base read -- the read that is sent to the
    //  FIU.
    //
    //  The chains are stored in inferred RAMs instead of the usual MPF
    //  explicit RAM primitives because they will be read, synchronously,
    //  into a registered c0Rx type that is forwarded to the AFU.
    //
    // ====================================================================

    // rd_chain_mdata acts as both a holder of the full mdata of each
    // chained request and a next pointer.  The unique heap index of
    // every read request is stored by MPF in an upstream module
    // in t_heap_idx'(mdata).
    t_cci_mdata rd_chain_mdata[0 : MAX_ACTIVE_REQS-1];
    t_cci_mdata c0Tx_mdata_qq;

    always_ff @(posedge clk)
    begin
        // Add a link to a chain when a deduplication match is detected
        if (c0Tx_match_qq)
        begin
            rd_chain_mdata[c0Tx_match_tail_idx_qq] <= c0Tx_qq.hdr.base.mdata;
        end
    end


    t_heap_idx rd_chain_len_wr_idx;
    t_chain_len rd_chain_len_wr_data;

    t_heap_idx rd_chain_len_idx;
    t_chain_len rd_chain_len_data;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(MAX_ACTIVE_REQS),
        .N_DATA_BITS($bits(t_chain_len)),
        .N_OUTPUT_REG_STAGES(1),
        .BYPASS_FULL_PIPELINE(1)
        )
      rd_len
       (
        .clk,

        .wen(cci_mpf_c0TxIsReadReq(c0Tx_q)),
        .waddr(rd_chain_len_wr_idx),
        .wdata(rd_chain_len_wr_data),

        .raddr(rd_chain_len_idx),
        .rdata(rd_chain_len_data)
        );

    always_comb
    begin
        rd_chain_len_wr_idx = c0Tx_match_q ? c0Tx_match_head_idx_q :
                                             t_heap_idx'(c0Tx_q.hdr.base.mdata);

        rd_chain_len_wr_data = c0Tx_match_q ? c0Tx_match_chain_len_q :
                                              t_chain_len'(0);
    end

    //
    // Count chained reads to avoid overflowing the response pipeline.
    //
    t_chained_req_cnt num_chained_reqs;
    logic chain_rsp_active;

    // dedup_rsp_fifo must have enough space for all extra cycles caused
    // by chained duplication.
    always_ff @(posedge clk)
    begin
        chain_rsp_buffer_almFull <=
            (num_chained_reqs >=
             t_chained_req_cnt'(MAX_CHAINED_REQS -
                                (CCI_TX_ALMOST_FULL_THRESHOLD + 1) *
                                CCI_MAX_MULTI_LINE_BEATS));

        if (reset)
        begin
            chain_rsp_buffer_almFull <= 1'b0;
        end
    end

    always_ff @(posedge clk)
    begin
        if (c0Tx_match_qq && chain_rsp_active)
        begin
            // Both incr and decr
            num_chained_reqs <= num_chained_reqs +
                                t_chained_req_cnt'(c0Tx_qq.hdr.base.cl_len);
        end
        else if (c0Tx_match_qq)
        begin
            // Just incr
            num_chained_reqs <= num_chained_reqs + t_chained_req_cnt'(1) +
                                t_chained_req_cnt'(c0Tx_qq.hdr.base.cl_len);
        end
        else if (chain_rsp_active)
        begin
            // Just decr
            num_chained_reqs <= num_chained_reqs - t_chained_req_cnt'(1);
        end

        if (reset)
        begin
            num_chained_reqs <= t_chained_req_cnt'(0);
        end
    end


    // ====================================================================
    //
    // Read response pipeline
    //
    // ====================================================================

    // Pipeline stages before the c0Rx FIFO delay the pipeline so
    // that there is time for the tracker to close the chain of
    // deduplicated requests.
    //
    t_if_cci_c0_Rx c0Rx[0:2];

    assign c0Rx[0] = fiu.c0Rx;

    genvar s;
    generate
        for (s = 1; s < 3; s = s + 1)
        begin : cp
            always_ff @(posedge clk)
            begin
                c0Rx[s] <= c0Rx[s-1];

                if (reset)
                begin
                    c0Rx[s] <= cci_c0Rx_clearValids();
                end
            end
        end
    endgenerate

    assign rd_chain_len_idx = t_heap_idx'(c0Rx[0].hdr.mdata);


    //
    // FIFO holding deduplicated responses.
    //
    t_if_cci_c0_Rx c0Rx_first;
    t_chain_len c0Rx_first_len;
    logic c0Rx_deq, c0Rx_deq_q;
    logic c0Rx_notEmpty;

    cci_mpf_prim_fifo_bram
      #(
        .N_DATA_BITS($bits(t_if_cci_c0_Rx) + $bits(t_chain_len)),
        .N_ENTRIES(MAX_CHAINED_REQS)
        )
      dedup_rsp_fifo
       (
        .clk,
        .reset,
        .enq_data({ c0Rx[2], rd_chain_len_data }),
        .enq_en(cci_c0Rx_isValid(c0Rx[2])),
        // The FIFO is large enough to hold all outstanding requests.  Requests
        // are limited by tracking num_chained_reqs.
        .notFull(),
        .almostFull(),
        .first({ c0Rx_first, c0Rx_first_len }),
        .deq_en(c0Rx_deq),
        .notEmpty(c0Rx_notEmpty)
        );

    always_ff @(posedge clk)
    begin
        c0Rx_deq_q <= c0Rx_deq;
        if (reset)
        begin
            c0Rx_deq_q <= 1'b0;
        end
    end


    //
    // Track active response chains
    //
    t_chain_len chain_rsp_links_rem;
    assign chain_rsp_active = (chain_rsp_links_rem !== t_chain_len'(0));

    always_ff @(posedge clk)
    begin
        if (chain_rsp_active)
        begin
            chain_rsp_links_rem <= chain_rsp_links_rem - t_chain_len'(1);
        end

        if (c0Rx_deq && cci_c0Rx_isReadRsp(c0Rx_first))
        begin
            chain_rsp_links_rem <= c0Rx_first_len;
        end

        if (reset)
        begin
            chain_rsp_links_rem <= t_chain_len'(0);
        end
    end


    //
    // Generate responses, walking chains as needed.
    //
    assign c0Rx_deq = c0Rx_notEmpty && ! chain_rsp_active;

    always_ff @(posedge clk)
    begin
        if (chain_rsp_active)
        begin
            // Set the next mdata in a chain
            afu.c0Rx.hdr.mdata <= rd_chain_mdata[t_heap_idx'(afu.c0Rx.hdr.mdata)];
        end
        else
        begin
            afu.c0Rx <= c0Rx_first;
            if (! c0Rx_notEmpty)
            begin
                afu.c0Rx <= cci_c0Rx_clearValids();
            end
        end

        if (reset)
        begin
            afu.c0Rx <= cci_c0Rx_clearValids();
        end
    end

    // synthesis translate_off

    logic chain_rsp_buffer_almFull_q;

    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES && cci_c0Rx_isReadRsp(afu.c0Rx))
        begin
            if (c0Rx_deq_q && chain_rsp_active)
            begin
                $display("Dedup reads [%0t]: rsp head idx 0x%x cl_num %0d len %0d",
                         $time,
                         t_heap_idx'(afu.c0Rx.hdr.mdata),
                         afu.c0Rx.hdr.cl_num,
                         chain_rsp_links_rem);
            end
            else if (! c0Rx_deq_q)
            begin
                $display("Dedup reads [%0t]: rsp chain next idx 0x%x cl_num %0d rem %0d",
                         $time,
                         t_heap_idx'(afu.c0Rx.hdr.mdata),
                         afu.c0Rx.hdr.cl_num,
                         chain_rsp_links_rem);
            end
        end

        if (! reset && DEBUG_MESSAGES)
        begin
            if (chain_rsp_buffer_almFull != chain_rsp_buffer_almFull_q)
            begin
                $display("Dedup reads [%0t]: rsp chain %s FULL (%0d)",
                         $time,
                         (chain_rsp_buffer_almFull ? "ALMOST" : "NOT"),
                         num_chained_reqs);
            end
        end

        chain_rsp_buffer_almFull_q <= chain_rsp_buffer_almFull;
        if (reset)
        begin
            chain_rsp_buffer_almFull_q <= 1'b0;
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_shim_dedup_reads


//
// Track active reads and indicate when a new read request is a duplicate
// of an active read.  Note that write requests do not have to be tracked
// at all.  The lifetime of a tracker entry ends when a read response
// is returned.  CCI makes no guarantees about ordering when requests
// are in flight, so all in-flight reads are free to ignore write activity.
//
module cci_mpf_shim_dedup_reads_tracker
  #(
    // Maximum number in-flight requests per channel.
    parameter MAX_ACTIVE_REQS = 128,

    // Maximum length of a deduplicated chain
    parameter MAX_CHAIN_LEN = 8,

    parameter DEBUG_MESSAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // New requests from AFU
    input  t_if_cci_mpf_c0_Tx c0Tx,

    // Responses from FIU
    input  t_if_cci_c0_Rx c0Rx,

    // True the cycle following c0Tx if read request matches an earlier
    // active read request.
    output logic c0Tx_match_q,
    // Index of the head and tail of the chain connecting deduplicated reads.
    output logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] c0Tx_match_head_idx_q,
    output logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] c0Tx_match_tail_idx_q,
    // Length of the chain of deduplicated reads.
    output logic [$clog2(MAX_CHAIN_LEN)-1 : 0] c0Tx_match_chain_len_q
    );

    // Number of entries in the tracker (must be a power of 2)
    localparam RADIX_TRACKER_ENTRIES = 4;
    localparam N_TRACKER_ENTRIES = (1 << RADIX_TRACKER_ENTRIES);

    typedef logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] t_heap_idx;
    typedef logic [$clog2(MAX_CHAIN_LEN)-1 : 0] t_chain_len;


    //
    // Index and tag in the tracker.  The sizes of the two add up to a full
    // address.
    //
    typedef logic [RADIX_TRACKER_ENTRIES-1 : 0] t_tracker_idx;
    typedef logic [CCI_CLADDR_WIDTH-RADIX_TRACKER_ENTRIES-1 : 0] t_tracker_tag;

    // Valid bits tracker entries
    logic tracker_valid[0 : N_TRACKER_ENTRIES-1];
    logic tracker_end_lifetime[0 : N_TRACKER_ENTRIES-1];

    // Length of a tracker's deduplicated chain
    t_chain_len tracker_chain_len[0 : N_TRACKER_ENTRIES-1] /* synthesis ramstyle = "logic" */;

    // Heap index of the base request -- the original request that was
    // sent to the FIU that may then be replicated
    t_heap_idx tracker_base_heap_idx[0 : N_TRACKER_ENTRIES-1] /* synthesis ramstyle = "logic" */;

    // What is being tracked?
    typedef struct packed
    {
        // Tag and cl_len must match for a request to be deduplicated
        t_tracker_tag tag;
        t_cci_clLen cl_len;

        // Heap index of the most recently seen read of the address
        t_heap_idx latest_heap_idx;
    }
    t_tracker_state;

    t_tracker_idx rd_tracker_idx;
    t_tracker_state rd_tracker_state;

    t_tracker_idx wr_tracker_idx;
    logic wr_tracker_en;
    t_tracker_state wr_tracker_state;

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(N_TRACKER_ENTRIES),
        .N_DATA_BITS($bits(t_tracker_state))
        )
      tracker
       (
        .clk,
        .reset,

        .raddr(rd_tracker_idx),
        .rdata(rd_tracker_state),

        .waddr(wr_tracker_idx),
        .wen(wr_tracker_en),
        .wdata(wr_tracker_state)
        );


    // Tracker index (direct mapped) and tag to prove that the requested
    // address is being tracked.
    t_tracker_tag tag;

    always_comb
    begin
        { tag, rd_tracker_idx } = cci_mpf_c0_getReqAddr(c0Tx.hdr);
    end

    //
    // Update the tracker valid bits in stage 0 -- the stage in which c0Tx
    // and c0Rx arrive.
    //
    genvar t;
    generate
        for (t = 0; t < N_TRACKER_ENTRIES; t = t + 1)
        begin : v
            always_comb
            begin
                tracker_end_lifetime[t] = 
                    (cci_c0Rx_isReadRsp(c0Rx) &&
                     tracker_base_heap_idx[t] == t_heap_idx'(c0Rx.hdr.mdata));
            end

            always_ff @(posedge clk)
            begin
                // New read request
                if (cci_mpf_c0TxIsReadReq(c0Tx) &&
                    t_tracker_idx'(t) == rd_tracker_idx)
                begin
                    tracker_valid[t] <= 1'b1;
                end

                // Disable tracker when the CCI read response arrives
                if (tracker_end_lifetime[t])
                begin
                    tracker_valid[t] <= 1'b0;
                end

                if (reset)
                begin
                    tracker_valid[t] <= 1'b0;
                end
            end
        end
    endgenerate


    //
    // Read tracker state in stage 0 and register for use in stage 1.
    // The bypass embedded in cci_mpf_prim_lutram ensures that writes
    // in stage 1 are visible to stage 0 reads.  The pipeline is thus
    // arranged so that tracker_valid is consistent with other tracker
    // state.
    //

    t_if_cci_mpf_c0_Tx c0Tx_q;

    logic valid_q;
    t_tracker_state state_q;
    t_tracker_tag tag_cmp_q;
    t_tracker_tag tag_q;
    t_tracker_idx rd_tracker_idx_q;
    logic cl_len_cmp_q;
    t_cci_clLen cl_len_q;
    logic chain_full_q;

    // Forward tracker state to stage 1
    always_ff @(posedge clk)
    begin
        valid_q <= tracker_valid[rd_tracker_idx] &&
                   cci_mpf_c0TxIsReadReq(c0Tx);
        c0Tx_q <= c0Tx;

        // Read current tracker state with bypass for updates from stage 1.
        state_q <= rd_tracker_state;
        tag_cmp_q <= (tag == rd_tracker_state.tag);
        cl_len_cmp_q <= (c0Tx.hdr.base.cl_len == rd_tracker_state.cl_len);

        if (wr_tracker_en && (wr_tracker_idx == rd_tracker_idx))
        begin
            state_q <= wr_tracker_state;
            tag_cmp_q <= (tag == wr_tracker_state.tag);
            cl_len_cmp_q <= (c0Tx.hdr.base.cl_len == wr_tracker_state.cl_len);
        end

        // The tracker is full if the chain is almost full.  There is one
        // cycle in the pipeline not yet accounted for, so the low bit is
        // ignored in this test to leave space.
        chain_full_q <= (&(tracker_chain_len[rd_tracker_idx][$bits(t_chain_len)-1 : 1]));
        if (wr_tracker_en && (wr_tracker_idx == rd_tracker_idx) && ! c0Tx_match_q)
        begin
            // New chain just started
            chain_full_q <= 1'b0;
        end

        if (reset)
        begin
            valid_q <= 1'b0;
            c0Tx_q <= cci_mpf_c0Tx_clearValids();
        end
    end

    always_comb
    begin
        { tag_q, rd_tracker_idx_q } = cci_mpf_c0_getReqAddr(c0Tx_q.hdr);
        cl_len_q = c0Tx_q.hdr.base.cl_len;

        c0Tx_match_q = valid_q && ! chain_full_q &&
                       tag_cmp_q &&
                       cl_len_cmp_q;
        c0Tx_match_head_idx_q = tracker_base_heap_idx[rd_tracker_idx_q];
        c0Tx_match_tail_idx_q = state_q.latest_heap_idx;
        c0Tx_match_chain_len_q = tracker_chain_len[rd_tracker_idx_q] +
                                 t_chain_len'(1);

        wr_tracker_en = cci_mpf_c0TxIsReadReq(c0Tx_q);
        wr_tracker_idx = rd_tracker_idx_q;

        wr_tracker_state = 'x;
        wr_tracker_state.tag = tag_q;
        wr_tracker_state.cl_len = cl_len_q;
        wr_tracker_state.latest_heap_idx = t_heap_idx'(c0Tx_q.hdr.base.mdata);
    end

    generate
        for (t = 0; t < N_TRACKER_ENTRIES; t = t + 1)
        begin : y
            always_ff @(posedge clk)
            begin
                if (wr_tracker_en &&
                    t_tracker_idx'(t) == wr_tracker_idx)
                begin
                    if (! c0Tx_match_q)
                    begin
                        // Read doesn't match an existing request
                        tracker_base_heap_idx[t] <=
                            t_heap_idx'(c0Tx_q.hdr.base.mdata);
                        tracker_chain_len[t] <= t_chain_len'(0);
                    end
                    else
                    begin
                        tracker_chain_len[t] <= c0Tx_match_chain_len_q;
                    end
                end
            end
        end
    endgenerate


    // synthesis translate_off

    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES)
        begin
            if (c0Tx_match_q)
            begin
                if (tracker_base_heap_idx[wr_tracker_idx] == c0Tx_match_tail_idx_q)
                begin
                    $display("Dedup reads [%0t]: trk %0d start addr 0x%h cl_len %0d idx 0x%h matches base idx 0x%h len %0d",
                             $time,
                             wr_tracker_idx,
                             { tag_q, rd_tracker_idx_q },
                             c0Tx_q.hdr.base.cl_len,
                             wr_tracker_state.latest_heap_idx,
                             c0Tx_match_head_idx_q,
                             c0Tx_match_chain_len_q);
                end
                else
                begin
                    $display("Dedup reads [%0t]: trk %0d chain addr 0x%h cl_len %0d idx 0x%h matches base idx 0x%h prev idx 0x%h len %0d",
                             $time,
                             wr_tracker_idx,
                             { tag_q, rd_tracker_idx_q },
                             c0Tx_q.hdr.base.cl_len,
                             wr_tracker_state.latest_heap_idx,
                             c0Tx_match_head_idx_q,
                             c0Tx_match_tail_idx_q,
                             c0Tx_match_chain_len_q);
                end
            end
            else if (wr_tracker_en)
            begin
                $display("Dedup reads [%0t]: trk %0d note addr 0x%h idx 0x%h",
                         $time,
                         wr_tracker_idx,
                         { tag_q, rd_tracker_idx_q },
                         wr_tracker_state.latest_heap_idx);
            end

            for (int i = 0; i < N_TRACKER_ENTRIES; i = i + 1)
            begin
                if (tracker_end_lifetime[i])
                begin
                    $display("Dedup reads [%0t]: trk %0d end", $time, i);
                end
            end
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_shim_dedup_reads_tracker
