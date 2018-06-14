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
`include "cci_mpf_csrs.vh"


//
// Manage response ordering and Mdata initialization.  This module is
// typically instantiated closest to the AFU in the MPF hierarchy.
//
// Many MPF shims depend on some Mdata bits being either zero or
// ignored by other layers.  This module is responsible for configuring
// Mdata in requests for use by other shims while preserving the AFU's
// metadata.
//
// The module has multiple options:
//
//   -- SORT_READ_RESPONSES:  Optionally sort read responses so they are
//      returned in the same order they were requested.
//
//   -- PRESERVE_WRITE_MDATA:  Optionally preserve Mdata for write
//      request/response pairs.  When false the Mdata returned with write
//      responses is always 0.  Read Mdata is always preserved.
//
//

// ********************************************************************
//
//  Heap buffer sizes in this module must be set large enough to
//  accommodate an extra cycle of latency of almFull signals in the
//  direction of the AFU in order to permit registering the almFull
//  signals on exit from MPF to the AFU.
//
// ********************************************************************


module cci_mpf_shim_rsp_order
  #(
    // Sort read responses?  CCI returns responses out of order.  This
    // module can instantiate a reorder buffer to return responses
    // in the order they were requested.
    //
    // Note: Mdata for read responses is ALWAYS preserved.  When sorting
    // is disabled Mdata is the only method for the client to match
    // responses.  When sorting is enabled the size of Mdata relative to
    // the size of the reorder buffer is irrelevant and thus not worth
    // the extra logic to optionally drop Mdata.  Quartus will likely
    // delete dead code on its own.
    parameter SORT_READ_RESPONSES = 1,

    // Preserve Mdata field in write requests?  Clients that merely count
    // active writes might not use Mdata for writes, so the block RAM
    // required for preservation can be saved.
    parameter PRESERVE_WRITE_MDATA = 1,

    // Maximum number in-flight requests per channel.
    parameter MAX_ACTIVE_REQS = 128,

    // Extra stages to add to usual threshold
    parameter THRESHOLD_EXTRA = 6
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu
    );

    // MPF treats MAX_ACTIVE_REQS as both the maximum number of requests
    // of any size and the maximum number of lines outstanding.  It turns
    // out that maximum throughput is typically reached with the same
    // number of lines outstanding, independent of request size.  Of
    // course the bandwidth may vary as a function of request size.
    localparam MAX_ACTIVE_LINES = MAX_ACTIVE_REQS;

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    // Index of a request
    localparam N_REQ_IDX_BITS = $clog2(MAX_ACTIVE_REQS);
    typedef logic [N_REQ_IDX_BITS-1 : 0] t_req_idx;

    // Full signals that will come from the ROB and heap used to
    // sort responses.
    logic rd_not_full;
    logic wr_not_full;

    // ====================================================================
    //
    //  The ROB is allocated with enough reserve space so that
    //  it honors the almost full semantics. No other buffering is
    //  required.
    //
    // ====================================================================

    assign afu.c0TxAlmFull = fiu.c0TxAlmFull || ! rd_not_full;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull || ! wr_not_full;


    // ====================================================================
    //
    //  Allocate a heap for preserving Mdata fields in writes.  This
    //  could logically be a separate shim but doing so would cost an
    //  extra cycle in load responses in order to read the Mdata out of
    //  block RAM.
    //
    //   *** The heap is allocated even when PRESERVE_WRITE_MDATA is
    //   *** false because downstream code depends on requests being
    //   *** uniquely numbered in the low N_REQ_IDX_BITS of Mdata.
    //   *** The heap control logic guarantees this.  When
    //   *** PRESERVE_WRITE_MDATA is false the heap's memory
    //   *** is never read, allowing synthesis tools to delete
    //   *** the memory but preserve the control logic.
    //
    // ====================================================================

    t_req_idx wr_heap_allocIdx;
    t_req_idx wr_heap_readIdx;
    t_cci_mdata wr_heap_readMdata;

    logic wr_heap_alloc;
    logic wr_heap_free;

    cci_mpf_prim_heap
      #(
        .N_ENTRIES(MAX_ACTIVE_REQS),
        .N_DATA_BITS(CCI_MDATA_WIDTH),
        .MIN_FREE_SLOTS(CCI_TX_ALMOST_FULL_THRESHOLD + THRESHOLD_EXTRA),
        .N_OUTPUT_REG_STAGES(1)
        )
      wr_heap
       (
        .clk,
        .reset,

        .enq(wr_heap_alloc),
        .enqData(afu.c1Tx.hdr.base.mdata),
        .notFull(wr_not_full),
        .allocIdx(wr_heap_allocIdx),

        .readReq(wr_heap_readIdx),
        .readRsp(wr_heap_readMdata),
        .free(wr_heap_free),
        .freeIdx(wr_heap_readIdx)
        );


    // ====================================================================
    //
    // Buffer for responses to allow time for heap lookup.  If we aren't
    // sorting reads or preserving Mdata for writes then the buffer isn't
    // needed.  In that case just make it an alias for the incoming data.
    //
    // ====================================================================

    cci_mpf_if fiu_buf (.clk);

    cci_mpf_shim_buffer_fiu
      #(
        // Both the ROB and heaps depend on 2 cycle latency
        .N_RX_REG_STAGES(2),
        .REGISTER_OUTBOUND(1)
        )
      buf_rx
       (
        .clk,
        .fiu_raw(fiu),
        .fiu_buf
        );


    // ====================================================================
    //
    //  Channel 0 (read)
    //
    // ====================================================================

    t_req_idx rd_rob_allocIdx;

    logic rd_rob_deq_en;
    logic rd_rob_notEmpty;
    logic rd_rob_data_rdy;
    t_cci_mdata rd_rob_mdata;
    t_cci_mdata rd_heap_readMdata;

    logic rd_rob_sop;
    logic rd_rob_eop;
    t_cci_clNum rd_rob_cl_num;
    t_cci_clData rd_rob_out_data;

    // Number of read buffer entries to allocate.  More than one must be
    // allocated to hold multi-beat read responses. CCI-P allows
    // up to 4 lines per request, one line per beat.
    logic [2:0] n_alloc;
    assign n_alloc =
        cci_mpf_c0TxIsReadReq(afu.c0Tx) ?
            3'(afu.c0Tx.hdr.base.cl_len) + 3'(1) :
            3'(0);

    generate
        if (SORT_READ_RESPONSES)
        begin : gen_rd_rob
            // Read response index is the base index allocated by the
            // c0Tx read request plus the beat offset for multi-line reads.
            logic rd_rob_rsp_en;
            t_req_idx rd_rob_rsp_idx;
            t_cci_clNum rd_rob_rsp_cl_num;
            t_cci_clData rd_rob_rsp_data;

            always_comb
            begin
                rd_rob_rsp_en = cci_c0Rx_isReadRsp(fiu.c0Rx);
                rd_rob_rsp_idx = t_req_idx'(fiu.c0Rx.hdr.mdata) +
                                 t_req_idx'(fiu.c0Rx.hdr.cl_num);
                rd_rob_rsp_cl_num = fiu.c0Rx.hdr.cl_num;
                rd_rob_rsp_data = fiu.c0Rx.data;
            end

            t_cci_mdata rd_sop_mdata;
            t_cci_mdata rd_beat_mdata;

            t_cci_clNum rd_packet_len;
            t_cci_clNum rd_sop_packet_len;
            t_cci_clNum rd_beat_packet_len;

            //
            // Read responses are sorted.  Allocate a ROB as
            // a reorder buffer.
            //
            cci_mpf_prim_rob
              #(
                // MAX_ACTIVE_LINES is used here for clarity, since the ROB
                // is line-based.  However, in MPF MAX_ACTIVE_LINES is equal
                // to MAX_ACTIVE_REQS.
                .N_ENTRIES(MAX_ACTIVE_LINES),
                .N_DATA_BITS($bits(t_cci_clNum) + CCI_CLDATA_WIDTH),
                .N_META_BITS($bits(t_cci_clNum) + CCI_MDATA_WIDTH),
                .MIN_FREE_SLOTS((CCI_TX_ALMOST_FULL_THRESHOLD + THRESHOLD_EXTRA) * CCI_MAX_MULTI_LINE_BEATS),
                .MAX_ALLOC_PER_CYCLE(CCI_MAX_MULTI_LINE_BEATS)
                )
              rd_rob
               (
                .clk,
                .reset,

                .alloc(n_alloc),
                .allocMeta({ afu.c0Tx.hdr.base.cl_len, afu.c0Tx.hdr.base.mdata }),
                .notFull(rd_not_full),
                .allocIdx(rd_rob_allocIdx),

                .enqData_en(rd_rob_rsp_en),
                .enqDataIdx(rd_rob_rsp_idx),
                .enqData({ rd_rob_rsp_cl_num, rd_rob_rsp_data }),

                .deq_en(rd_rob_deq_en),
                .notEmpty(rd_rob_notEmpty),
                .T2_first({ rd_rob_cl_num, rd_rob_out_data }),
                .T2_firstMeta({ rd_beat_packet_len, rd_beat_mdata })
                );


            // ROB data appears 2 cycles after notEmpty is asserted
            logic rd_rob_deq_en_q;
            always_ff @(posedge clk)
            begin
                if (reset)
                begin
                    rd_rob_deq_en_q <= 1'b0;
                    rd_rob_data_rdy <= 1'b0;
                end
                else
                begin
                    rd_rob_deq_en_q <= rd_rob_deq_en;
                    rd_rob_data_rdy <= rd_rob_deq_en_q;
                end
            end

            // Responses are now ordered.  Mark EOP when the last flit is
            // forwarded.
            assign rd_rob_eop = (rd_rob_cl_num == rd_packet_len);

            // SOP must follow EOP
            always_ff @(posedge clk)
            begin
                if (reset)
                begin
                    rd_rob_sop <= 1'b1;
                end
                else if (rd_rob_data_rdy)
                begin
                    rd_rob_sop <= rd_rob_eop;

                    assert (rd_rob_sop == (rd_rob_cl_num == 0)) else
                        $fatal("cci_mpf_shim_rsp_order: Incorrect rd_rob_sop calculation!");
                end
            end

            // The Mdata field stored in the ROB is valid only for the first
            // beat in multi-line responses.  Since responses are ordered
            // we can preserve valid Mdata and return it with the remaining
            // flits in a packet.
            always_comb
            begin
                if (rd_rob_sop)
                begin
                    rd_rob_mdata = rd_beat_mdata;
                    rd_packet_len = rd_beat_packet_len;
                end
                else
                begin
                    rd_rob_mdata = rd_sop_mdata;
                    rd_packet_len = rd_sop_packet_len;
                end
            end

            always_ff @(posedge clk)
            begin
                if (rd_rob_data_rdy && rd_rob_sop)
                begin
                    rd_sop_mdata <= rd_beat_mdata;
                    rd_sop_packet_len <= rd_beat_packet_len;
                end
            end

            assign rd_heap_readMdata = 'x;
        end
        else
        begin
            //
            // Read responses are not sorted.  Allocate a heap to
            // preserve Mdata.
            //
            logic heap_notFull;

            cci_mpf_prim_heap
              #(
                .N_ENTRIES(MAX_ACTIVE_REQS),
                .N_DATA_BITS(CCI_MDATA_WIDTH),
                // Almost full is tracked with rd_lines_active below, not
                // this heap.  The heap can never fill because rd_lines_active
                // will cut off requests.
                .MIN_FREE_SLOTS(0),
                .N_OUTPUT_REG_STAGES(1)
                )
              rd_heap
               (
                .clk,
                .reset,

                .enq(cci_mpf_c0TxIsReadReq(afu.c0Tx)),
                .enqData(afu.c0Tx.hdr.base.mdata),
                .notFull(heap_notFull),
                .allocIdx(rd_rob_allocIdx),

                .readReq(t_req_idx'(fiu.c0Rx.hdr.mdata)),
                .readRsp(rd_heap_readMdata),
                .free(cci_c0Rx_isReadRsp(fiu.c0Rx) &&
                      cci_mpf_c0Rx_isEOP(fiu.c0Rx)),
                .freeIdx(t_req_idx'(fiu.c0Rx.hdr.mdata))
                );

            assign rd_rob_eop = 'x;
            assign rd_rob_cl_num = 'x;
            assign rd_rob_out_data = 'x;
            assign rd_rob_mdata = 'x;
            assign rd_rob_notEmpty = 'x;
            assign rd_rob_data_rdy = 'x;

            //
            // Count the number of lines in flight.  This is needed to make
            // sure we don't overflow any clock crossing FIFOs by exceeding
            // the read response buffer.
            //
            t_req_idx rd_lines_active;

            always_ff @(posedge clk)
            begin
                // Update active lines.  The number active grows by the number
                // of beats requested and shrinks by at most one per cycle.
                rd_lines_active <= rd_lines_active +
                                   t_req_idx'(n_alloc) -
                                   t_req_idx'(cci_c0Rx_isReadRsp(fiu.c0Rx));

                rd_not_full <=
                    heap_notFull &&
                    (rd_lines_active <
                     t_req_idx'(MAX_ACTIVE_REQS - (CCI_TX_ALMOST_FULL_THRESHOLD + THRESHOLD_EXTRA) * CCI_MAX_MULTI_LINE_BEATS));

                if (reset)
                begin
                    rd_lines_active <= t_req_idx'(0);
                end
            end
        end
    endgenerate


    // Forward requests toward the FIU.  Replace the Mdata entry with the
    // ROB index.  The original Mdata is saved in the rob
    // and restored when the response is returned.
    always_comb
    begin
        fiu_buf.c0Tx = afu.c0Tx;
        fiu_buf.c0Tx.hdr.base.mdata = t_cci_mdata'(rd_rob_allocIdx);
    end


    logic c0_non_rd_valid;

    //
    // Responses
    //

    // The ROB has a 2 cycle latency.  When the ROB is not empty decide when
    // to deq based on whether fiu is empty.  The ROB response will be merged
    // into afu two cycles later, when the empty fiu slot reaches fiu_buf.
    always_comb
    begin
        // Is there a non-read response active?
        c0_non_rd_valid = cci_c0Rx_isValid(fiu.c0Rx) &&
                          ! cci_c0Rx_isReadRsp(fiu.c0Rx);

        rd_rob_deq_en = rd_rob_notEmpty && ! c0_non_rd_valid;
    end


    always_comb
    begin
        afu.c0Rx = fiu_buf.c0Rx;

        // Either forward the header from the FIU for non-read responses or
        // reconstruct the read response header.  The CCI-E header has the same
        // low bits as CCI-S so we always construct CCI-E and truncate when
        // in CCI-S mode.
        if (SORT_READ_RESPONSES && rd_rob_data_rdy)
        begin
            afu.c0Rx.hdr = cci_c0_genRspHdr(eRSP_RDLINE, rd_rob_mdata);
            afu.c0Rx.hdr.cl_num = rd_rob_cl_num;
            afu.c0Rx = cci_mpf_c0Rx_updEOP(afu.c0Rx, rd_rob_eop);
            afu.c0Rx.data = rd_rob_out_data;
            afu.c0Rx.rspValid = 1'b1;
        end
        else if (SORT_READ_RESPONSES && cci_c0Rx_isReadRsp(fiu_buf.c0Rx))
        begin
            // Read response comes from the ROB, not the FIU directly
            afu.c0Rx.rspValid = 1'b0;
        end
        else
        begin
            afu.c0Rx.hdr = fiu_buf.c0Rx.hdr;

            // Return preserved Mdata
            if (cci_c0Rx_isReadRsp(afu.c0Rx))
            begin
                // This path reached only when SORT_READ_RESPONSES == 0.
                afu.c0Rx.hdr.mdata = rd_heap_readMdata;
            end
        end
    end


    // ====================================================================
    //
    //  Channel 1 (write) flows straight through.
    //
    // ====================================================================

    // Requests: replace the Mdata field with the heap index that holds
    // the preserved value.
    always_comb
    begin
        fiu_buf.c1Tx = afu.c1Tx;

        wr_heap_alloc = cci_mpf_c1TxIsWriteReq(afu.c1Tx) ||
                        cci_mpf_c1TxIsWriteFenceReq(afu.c1Tx);
        if (wr_heap_alloc)
        begin
            fiu_buf.c1Tx.hdr.base.mdata = t_cci_mdata'(wr_heap_allocIdx);
        end
    end

    // Responses
    always_comb
    begin
        afu.c1Rx = fiu_buf.c1Rx;

        // If a write response return then preserved Mdata if the
        // configuration requires it.
        if (PRESERVE_WRITE_MDATA)
        begin
            if (cci_c1Rx_isWriteRsp(afu.c1Rx) ||
                cci_c1Rx_isWriteFenceRsp(afu.c1Rx))
            begin
                afu.c1Rx.hdr.mdata = wr_heap_readMdata;
            end
        end
    end

    // Lookup write heap to restore Mdata
    assign wr_heap_readIdx = t_req_idx'(fiu.c1Rx.hdr.mdata);
    assign wr_heap_free = cci_c1Rx_isWriteRsp(fiu.c1Rx) ||
                          cci_c1Rx_isWriteFenceRsp(fiu.c1Rx);


    // ====================================================================
    //
    // Channel 2 Tx (MMIO read response) flows straight through.
    //
    // ====================================================================

    assign fiu_buf.c2Tx = afu.c2Tx;

endmodule // cci_mpf_shim_rsp_order

