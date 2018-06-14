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
`include "cci_mpf_shim_wro.vh"
`include "cci_mpf_prim_hash.vh"


//
// Requests come from the epoch tagging pipeline (cci_mpf_shim_buffer_afu_epoch)
// with inter-channel read/write hazards marked.  Intra-channel write/write
// hazards are not marked, mostly because the data structures used in the
// epoch tagger can't handle overlapping multi-line address ranges.
//
// This module detects active write/write request hazards for requests that
// have not yet been sent to the FIU.
//

module cci_mpf_shim_wro_epoch_order
  #(
    parameter AFU_BUF_THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,
    parameter ADDRESS_HASH_BITS = 12,

    // Depth of the pipeline being tracked.  Hashed addresses of new
    // requests are compared against the hashed addresses of existing
    // requests in the pipeline.
    parameter PIPE_DEPTH = 0
    )
   (
    input  logic clk,

    // This flow matches cci_mpf_shim_buffer_afu_epoch.  Tx requests flow
    // afu -> client_afu and client_fiu -> fiu.
    cci_mpf_if.to_afu afu,
    cci_mpf_if.to_fiu client_afu,
    cci_mpf_if.to_afu client_fiu,
    cci_mpf_if.to_fiu fiu,

    // General state of the active pipeline (downstream requests still in WRO)
    input  logic active_notEmpty,
    input  logic active_c0Tx_notEmpty,
    input  logic active_c1Tx_notEmpty,

    // States of existing requests in the write pipeline
    input  logic active_valid [0 : PIPE_DEPTH-1],
    input  logic [ADDRESS_HASH_BITS-1 : 0] active_addrHash [0 : PIPE_DEPTH-1],
    input  t_cci_mpf_wro_line_mask active_lineMask [0 : PIPE_DEPTH-1],

    // Client accepts requests from this module's client_afu.Tx
    input  logic deqC0Tx,
    input  logic deqC1Tx
    );

    logic reset;
    assign reset = fiu.reset;

    cci_mpf_if afu_epoch (.clk);

    assign afu_epoch.reset = client_afu.reset;
    assign afu_epoch.c0Rx = client_afu.c0Rx;
    assign afu_epoch.c1Rx = client_afu.c1Rx;

    assign client_afu.c2Tx = afu_epoch.c2Tx;

    logic local_c1Tx_notEmpty;

    // Latency-insensitive ports need explicit dequeue (enable).
    logic c0_afu_deq;
    logic c1_afu_deq;

    // Epoch logic tells us when conflicting requests have been found, forcing
    // the Tx pipeline here to drain to avoid reordering stores.
    logic c0_epoch_unchanged;
    logic c1_epoch_unchanged;

    cci_mpf_shim_buffer_afu_epoch
      #(
        .THRESHOLD(AFU_BUF_THRESHOLD),
        .CLIENT_PIPE_STAGES(PIPE_DEPTH+5),
        .REGISTER_OUTPUT(1),
        .FILTER_ADDRESS_HASH_BITS(12)
        )
      buf_epoch
       (
        .clk,

        // Incoming from AFU
        .afu,
        // Buffered and epochs tagged
        .client_afu(afu_epoch),
        // Addresses filtered (from pipeline here toward bufafu)
        .client_fiu,
        // FIU-bound processing in bufafu complete (from bufafu toward fiu) 
        .fiu,

        .deqC0Tx(c0_afu_deq),
        .deqC1Tx(c1_afu_deq),

        .c0Tx_epoch_unchanged(c0_epoch_unchanged),
        .c1Tx_epoch_unchanged(c1_epoch_unchanged)
        );

    //
    // Almost full signals in the buffered input are ignored --
    // replaced by deq signals and the buffer state.  Set them
    // to 1 to be sure they are ignored.
    //
    assign afu_epoch.c0TxAlmFull = 1'b1;
    assign afu_epoch.c1TxAlmFull = 1'b1;

    t_if_cci_mpf_c1_Tx c1Tx_stg1;
    t_if_cci_mpf_c1_Tx c1Tx_stg2;
    logic c1_was_wrFence;

    // Control for write request FIFOs defined below
    logic c1Tx_stg1_notFull;
    logic c1Tx_stg1_notEmpty;
    logic c1Tx_stg1_deq;
    logic c1Tx_stg2_notFull;
    logic c1Tx_stg2_notEmpty;
    logic c1Tx_stg2_deq;

    // Include write request FIFO state in notEmpty state
    logic all_c1Tx_notEmpty;
    logic all_notEmpty;

    assign all_c1Tx_notEmpty = active_c1Tx_notEmpty ||
                               c1Tx_stg1_notEmpty ||
                               c1Tx_stg2_notEmpty;
    assign all_notEmpty = active_notEmpty ||
                          c1Tx_stg1_notEmpty ||
                          c1Tx_stg2_notEmpty;

    always_comb
    begin
        //
        // Reads requests flow directly to the main WRO pipeline, gated only
        // by the epoch.  When the epoch changes the WRO pipeline must first
        // drain since the pipeline contains a circular buffer that may
        // reorder requests.
        //
        c0_afu_deq = deqC0Tx;
        client_afu.c0Tx =
            cci_mpf_c0TxMaskValids(afu_epoch.c0Tx,
                                   c0_epoch_unchanged || ! all_notEmpty);

        //
        // Write requests require more processing in this module.  The
        // first gate is here and is similar to the read request gate above,
        // though WrFence also is held until the WRO pipeline drains.
        //
        c1Tx_stg1 =
            cci_mpf_c1TxMaskValids(afu_epoch.c1Tx,
                                   // Either all older requests have exited WRO
                                   // (only reads matter because writes will be
                                   // checked before stg2)
                                   ! all_notEmpty ||
                                   // or no epoch change
                                   (c1_epoch_unchanged &&
                                    // and the last one wasn't a write fence
                                    ! c1_was_wrFence &&
                                    // and this one it isn't a write fence
                                    ! cci_mpf_c1TxIsWriteFenceReq_noCheckValid(afu_epoch.c1Tx)));
    end

    always_ff @(posedge clk)
    begin
        if (c1_afu_deq)
        begin
            c1_was_wrFence <=
                cci_mpf_c1TxIsWriteFenceReq_noCheckValid(client_afu.c1Tx);
        end

        if (reset)
        begin
            c1_was_wrFence <= 1'b0;
        end
    end


    //
    // Write requests are more complicated.  The epochs describe inter-channel
    // conflicts.  Writes have intra-channel conflicts since two writes to the
    // same address must be emitted in order.
    //

    //
    // First stage:  calculate the address hash.
    //
    typedef logic [ADDRESS_HASH_BITS-1 : 0] t_hash;

    function automatic t_hash hashAddr(t_cci_clAddr addr);
        return t_hash'(hash32(addr[$bits(t_cci_clLen) +: 32]));
    endfunction


    t_hash c1Tx_stg2_hash;

    // Move next request to fifo_stg1 when the FIFO has space and there is
    // a new request available.
    assign c1_afu_deq = c1Tx_stg1_notFull && cci_mpf_c1TxIsValid(c1Tx_stg1);

    cci_mpf_prim_fifo2
      #(
        .N_DATA_BITS($bits(t_hash) + $bits(t_if_cci_mpf_c1_Tx))
        )
      fifo_stg1
       (
        .clk,
        .reset,

        .enq_data({ hashAddr(cci_mpf_c1_getReqAddr(c1Tx_stg1.hdr)), c1Tx_stg1 }),
        .enq_en(c1_afu_deq),
        .notFull(c1Tx_stg1_notFull),

        .first({ c1Tx_stg2_hash, c1Tx_stg2 }),
        .deq_en(c1Tx_stg1_deq),
        .notEmpty(c1Tx_stg1_notEmpty)
        );


    //
    // Second stage: Record whether incoming requests have intra-channel
    // conflicts.  This is computed only once as requests arrive since the
    // computation is expensive.  When a conflict is detected the request is
    // held until the WRO c1Tx pipeline drains.
    //

    logic c1Tx_wr_conflict_in;

    t_cci_mpf_wro_line_mask c1Tx_stg2_lineMask;
    assign c1Tx_stg2_lineMask = wroWrFilterLineMask(c1Tx_stg2.hdr);

    typedef struct packed
    {
        logic wr_conflict;
        t_hash hash;
        t_cci_mpf_wro_line_mask lineMask;
        t_if_cci_mpf_c1_Tx c1Tx;
    }
    t_c1Tx_stg2_fifo;

    t_c1Tx_stg2_fifo c1Tx_stg2_in;
    t_c1Tx_stg2_fifo c1Tx_stg2_out;

    always_comb
    begin
        c1Tx_stg2_in.wr_conflict = c1Tx_wr_conflict_in &&
                                   cci_mpf_c1TxIsWriteReq(c1Tx_stg2) &&
                                   c1Tx_stg2.hdr.ext.checkLoadStoreOrder;
        c1Tx_stg2_in.hash = c1Tx_stg2_hash;
        c1Tx_stg2_in.lineMask = c1Tx_stg2_lineMask;
        c1Tx_stg2_in.c1Tx = c1Tx_stg2;
    end

    // Peek into fifo_stg2 state -- needed for active request address comparison
    logic [1:0] c1Tx_stg2_peek_valid;
    t_c1Tx_stg2_fifo [1:0] c1Tx_stg2_peek_value;

    // Compare new request address against all active write requests
    always_comb
    begin
        c1Tx_wr_conflict_in = 1'b0;
        for (int i = 0; i < PIPE_DEPTH; i = i + 1)
        begin
            c1Tx_wr_conflict_in =
                c1Tx_wr_conflict_in ||
                (active_valid[i] &&
                 // Hashed addresses refer to groups that can be addressed by
                 // the largest multi-beat request.  LineMask separates
                 // smaller requests to unique addresses within the multi-beat
                 // region.
                 (c1Tx_stg2_hash === active_addrHash[i]) &&
                 (|(c1Tx_stg2_lineMask & active_lineMask[i])));
        end

        // Check the entries in fifo_stg2
        for (int i = 0; i < 2; i = i + 1)
        begin
            c1Tx_wr_conflict_in =
                c1Tx_wr_conflict_in ||
                (c1Tx_stg2_peek_valid[i] &&
                 (c1Tx_stg2_hash === c1Tx_stg2_peek_value[i].hash) &&
                 (|(c1Tx_stg2_lineMask & c1Tx_stg2_peek_value[i].lineMask)));
        end
    end

    assign c1Tx_stg1_deq = c1Tx_stg1_notEmpty && c1Tx_stg2_notFull;

    cci_mpf_prim_fifo2_peek
      #(
        .N_DATA_BITS($bits(t_c1Tx_stg2_fifo))
        )
      fifo_stg2
       (
        .clk,
        .reset,

        .enq_data(c1Tx_stg2_in),
        .enq_en(c1Tx_stg1_deq),
        .notFull(c1Tx_stg2_notFull),

        .first(c1Tx_stg2_out),
        .deq_en(c1Tx_stg2_deq),
        .notEmpty(c1Tx_stg2_notEmpty),

        .peek_valid(c1Tx_stg2_peek_valid),
        .peek_value(c1Tx_stg2_peek_value)
        );

    assign c1Tx_stg2_deq = deqC1Tx;
    assign client_afu.c1Tx =
        cci_mpf_c1TxMaskValids(c1Tx_stg2_out.c1Tx,
                               // Ready if a request exists
                               c1Tx_stg2_notEmpty &&
                               // and it doesn't conflict with other active
                               // write requests or there are no other requests.
                               (! c1Tx_stg2_out.wr_conflict || ! active_c1Tx_notEmpty));

endmodule // cci_mpf_shim_wro_epoch_order
