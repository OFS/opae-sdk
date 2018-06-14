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
`include "cci_mpf_shim.vh"
`include "cci_mpf_shim_pwrite.vh"

//
// Partial write emulation using read-modify-write.  This module does not
// guarantee atomic access to the updated line.  On the FPGA side it relies
// on the WRO shim for managing read/write access to a line during an update.
// Conflicting CPU updates are not detected.
//

module cci_mpf_shim_pwrite
  #(
    parameter N_WRITE_HEAP_ENTRIES = 0,

    // mdata bit for tagging MPF-generated requests
    parameter RESERVED_MDATA_IDX = -1
    )
   (
    input  logic clk,

    // Connection toward the FIU (the end of the MPF pipeline nearest the AFU)
    cci_mpf_if.to_fiu fiu,

    // External connections to the AFU
    cci_mpf_if.to_afu afu,

    // Interface to the MPF FIU edge module
    cci_mpf_shim_pwrite_if.pwrite pwrite,

    cci_mpf_csrs.pwrite_events events
    );

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    assign afu.reset = fiu.reset;
    assign fiu.c2Tx = afu.c2Tx;

    localparam DEBUG = 0;


    // ====================================================================
    //
    // Set some flags that recognize read-modify traffic for partial
    // writes.
    //
    // ====================================================================

    t_if_cci_mpf_c1_Tx c1Tx;

    // Process the request in c1Tx this cycle?
    logic c1Tx_deq;

    // Is new write request a partial write?
    logic c1Tx_is_pwrite;
    logic c1Tx_is_pwrite_q;
    assign c1Tx_is_pwrite = cci_mpf_c1TxIsWriteReq(c1Tx) &&
                            c1Tx.hdr.pwrite.isPartialWrite;

    always_ff @(posedge clk)
    begin
        c1Tx_is_pwrite_q <= c1Tx_is_pwrite;
    end


    // Is the read response the read data for a partial write?
    logic c0Rx_is_pread_rsp;
    assign c0Rx_is_pread_rsp = (cci_c0Rx_isReadRsp(fiu.c0Rx) &&
                                cci_mpf_testShimMdataTag(RESERVED_MDATA_IDX,
                                                         CCI_MPF_SHIM_TAG_PWRITE,
                                                         fiu.c0Rx.hdr.mdata));


    // ====================================================================
    //
    // Storage for the write mask and header.
    //
    // ====================================================================

    typedef logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] t_write_heap_idx;

    // State stored in mdata.
    typedef struct packed
    {
        t_write_heap_idx idx;
        t_ccip_clLen cl_len;
    }
    t_pwrite_mdata_state;

    t_pwrite_mdata_state pw_state;
    assign pw_state =
        fiu.c0Rx.hdr.mdata[$bits(t_cci_mpf_shim_tag) +: $bits(t_pwrite_mdata_state)];

    t_write_heap_idx pw_idx;
    assign pw_idx = pw_state.idx;

    t_cci_clNum pw_clnum;
    assign pw_clnum = fiu.c0Rx.hdr.cl_num;

    t_cci_mpf_clDataByteMask mask_rdata;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(N_WRITE_HEAP_ENTRIES * CCI_MAX_MULTI_LINE_BEATS),
        .N_DATA_BITS($bits(t_cci_mpf_clDataByteMask)),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0)
        )
      mask
       (
        .clk,

        // Write mask arrives directly from the AFU edge module, bypassing
        // most of the MPF pipeline in order to reduce register consumption.
        .wen(pwrite.wen),
        .waddr({ pwrite.widx, pwrite.wclnum }),
        .wdata(pwrite.wpartial.mask),

        .raddr({ pw_idx, pw_clnum }),
        .rdata(mask_rdata)
        );


    // Save partial write requests until the read of the unmodified portion
    // returns.
    t_write_heap_idx pw_fifo_idx;
    t_if_cci_mpf_c1_Tx pw_wreq;

    t_write_heap_idx c1Tx_pwrite_idx;
    assign c1Tx_pwrite_idx = t_write_heap_idx'(c1Tx.data);

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(N_WRITE_HEAP_ENTRIES),
        .N_DATA_BITS($bits(t_if_cci_mpf_c1_Tx)),
        .N_OUTPUT_REG_STAGES(1)
        )
      wr_req
       (
        .clk,

        .wen(c1Tx_is_pwrite),
        // MPF stores a write index instead of actual data in the low bits of
        // the write request data field.  The actual data is held in a heap
        // in the FIU edge module.
        .waddr(c1Tx_pwrite_idx),
        .wdata(c1Tx),

        .raddr(pw_fifo_idx),
        .rdata(pw_wreq)
        );


    // ====================================================================
    //
    //  Reads
    //
    // ====================================================================

    // Block reads if a read for modify is pending in order to get
    // a read slot.
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull || c1Tx_is_pwrite_q;

    logic eop_tracker_rdy;
    logic may_inject_read;
    logic ready_for_read;
    assign may_inject_read = ! cci_mpf_c0TxIsValid(afu.c0Tx) && ready_for_read;

    always_ff @(posedge clk)
    begin
        ready_for_read <= ! fiu.c0TxAlmFull && eop_tracker_rdy;

        if (reset)
        begin
            ready_for_read <= 1'b0;
        end
    end

    // Read requests flow straight through.  Inject read for modify
    // as needed.
    always_comb
    begin
        fiu.c0Tx = afu.c0Tx;

        if (c1Tx_is_pwrite && may_inject_read)
        begin
            // Generate the read for modify
            t_cci_mpf_c0_ReqMemHdr h;
            t_pwrite_mdata_state s;

            h = t_cci_mpf_c0_ReqMemHdr'(0);
            h.base.vc_sel = c1Tx.hdr.base.vc_sel;

            h.base.cl_len = c1Tx.hdr.base.cl_len;
            h.base.req_type = eREQ_RDLINE_S;
            h.base.address = c1Tx.hdr.base.address;

            // Tag the read so we find the response
            h.base.mdata = cci_mpf_setShimMdataTag(RESERVED_MDATA_IDX,
                                                   CCI_MPF_SHIM_TAG_PWRITE);
            // Store the index in which the write metadata is saved
            s.idx = c1Tx_pwrite_idx;
            s.cl_len = c1Tx.hdr.base.cl_len;
            h.base.mdata[$bits(t_cci_mpf_shim_tag) +: $bits(t_pwrite_mdata_state)] = s;

            h.ext = c1Tx.hdr.ext;

            fiu.c0Tx = cci_mpf_genC0TxReadReq(h, 1'b1);
        end
    end

    always_ff @(posedge clk)
    begin
        if (c1Tx_is_pwrite && may_inject_read)
        begin
            if (DEBUG && ! reset)
            begin
                $display("PWRITE: Read req addr 0x%x, widx 0x%x, mdata 0x%x",
                         fiu.c0Tx.hdr.base.address,
                         c1Tx_pwrite_idx,
                         fiu.c0Tx.hdr.base.mdata);
            end
        end
    end


    //
    // Read response.
    //
    always_comb
    begin
        // Defaut: forward response toward AFU
        afu.c0Rx = fiu.c0Rx;

        // Don't forward partial read response.  It will be consumed in
        // this module.
        if (c0Rx_is_pread_rsp)
        begin
            afu.c0Rx.rspValid = 1'b0;
        end
    end


    //
    // Validate parameter settings and that the Mdata reserved bit is free
    // on all incoming read requests.
    //
    always_ff @(posedge clk)
    begin
        // Reserved bit must leave room below it for shim tag and the
        // heap index for storing the write request metadata.
        assert ((RESERVED_MDATA_IDX >= ($bits(t_write_heap_idx) + $bits(t_cci_mpf_shim_tag))) &&
                (RESERVED_MDATA_IDX < CCI_MDATA_WIDTH)) else
            $fatal("cci_mpf_shim_pwrite.sv: Illegal RESERVED_MDATA_IDX value: %d", RESERVED_MDATA_IDX);

        if (! reset)
        begin
            assert(! cci_mpf_c0TxIsValid(afu.c0Tx) ||
                   ! cci_mpf_testShimMdataTag(RESERVED_MDATA_IDX,
                                              CCI_MPF_SHIM_TAG_PWRITE,
                                              afu.c0Tx.hdr.base.mdata))
            else
                $fatal("cci_mpf_shim_pwrite.sv: AFU C0 PWRITE tag already used!");
        end
    end


    // ====================================================================
    //
    //  Writes
    //
    // ====================================================================

    //
    // Buffer incoming write requests.  The write pipeline may need to
    // wait for a read pipeline slot in order to emit a read for partial
    // updates.
    //

    t_if_cci_mpf_c1_Tx c1_first;
    logic c1_notEmpty;

    always_comb
    begin
        c1Tx = c1_first;
        c1Tx.valid = c1_notEmpty;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_if_cci_mpf_c1_Tx)),
        .N_ENTRIES(CCI_TX_ALMOST_FULL_THRESHOLD + 4),
        .THRESHOLD(CCI_TX_ALMOST_FULL_THRESHOLD),
        .REGISTER_OUTPUT(1)
        )
      c1_fifo
       (.clk,
        .reset(reset),

        .enq_data(afu.c1Tx),
        .enq_en(afu.c1Tx.valid),
        .notFull(),
        .almostFull(afu.c1TxAlmFull),

        .first(c1_first),
        .deq_en(c1Tx_deq),
        .notEmpty(c1_notEmpty)
        );


    always_ff @(posedge clk)
    begin
        events.pwrite_out_event_pwrite <= (c1Tx_is_pwrite && c1Tx_deq);

        if (reset)
        begin
            events.pwrite_out_event_pwrite <= 1'b0;
        end
    end

    //
    // Track partial read responses and the data retrieved from the write
    // mask heap.
    //
    logic c0Rx_is_pread_rsp_q;
    logic c0Rx_is_pread_rsp_qq;

    t_if_cci_c0_Rx c0Rx_q;
    t_if_cci_c0_Rx c0Rx_qq;

    t_pwrite_mdata_state c0Rx_qq_mdata_state;
    assign c0Rx_qq_mdata_state = 
        c0Rx_qq.hdr.mdata[$bits(t_cci_mpf_shim_tag) +: $bits(t_pwrite_mdata_state)];

    always_ff @(posedge clk)
    begin
        c0Rx_is_pread_rsp_q <= c0Rx_is_pread_rsp;
        c0Rx_is_pread_rsp_qq <= c0Rx_is_pread_rsp_q;

        c0Rx_q <= fiu.c0Rx;
        c0Rx_qq <= c0Rx_q;

        if (reset)
        begin
            c0Rx_is_pread_rsp_q <= 1'b0;
            c0Rx_is_pread_rsp_qq <= 1'b0;
        end
    end

    // Update partial writes with the current state of the line
    always_ff @(posedge clk)
    begin
        pwrite.upd_en <= c0Rx_is_pread_rsp_qq;
        pwrite.upd_idx <= c0Rx_qq_mdata_state.idx;
        pwrite.upd_clNum <= c0Rx_qq.hdr.cl_num;
        pwrite.upd_data <= c0Rx_qq.data;
        // Send the inverse of the write mask so that existing data is filled
        // in where the partial write doesn't supply new data.
        pwrite.upd_mask <= ~ mask_rdata;

        if (DEBUG && pwrite.upd_en)
        begin
            $display("PWRITE: Read rsp widx 0x%x cl_num %0d", pwrite.upd_idx, pwrite.upd_clNum);
        end

        if (reset)
        begin
            pwrite.upd_en <= 1'b0;
        end
    end


    // Track multi-beat read responses and only fire the write when all
    // read beats have arrived.
    logic is_pread_eop;
    logic is_pread_eop_q;

    cci_mpf_shim_pwrite_eop_tracker
      #(
        .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES)
        )
      eop_tracker
       (
        .clk,
        .reset,
        .rdy(eop_tracker_rdy),
        .rsp_en(c0Rx_is_pread_rsp),
        .rsp_idx(pw_state.idx),
        .rsp_len(pw_state.cl_len),
        .isEOP(is_pread_eop)
        );

    //
    // Store read response indices in a FIFO in order to trigger the
    // requested write now that old and new data are merged.  We can't
    // just generate the write since the write request port may be full.
    //
    logic pw_fifo_notEmpty;
    logic pw_fifo_deq;
    assign pw_fifo_deq = pw_fifo_notEmpty && ! fiu.c1TxAlmFull;

    // Only enq to the FIFO when all read beats have returned
    t_write_heap_idx pw_idx_q;

    always_ff @(posedge clk)
    begin
        pw_idx_q <= pw_idx;
        is_pread_eop_q <= c0Rx_is_pread_rsp && is_pread_eop;

        if (reset)
        begin
            is_pread_eop_q <= 1'b0;
        end
    end

    cci_mpf_prim_fifo_bram
      #(
        .N_DATA_BITS($bits(t_write_heap_idx)),
        .N_ENTRIES(N_WRITE_HEAP_ENTRIES)
        )
      pw_fifo
       (
        .clk,
        .reset,

        .enq_data(pw_idx_q),
        .enq_en(is_pread_eop_q),
        // The FIFO is as large as the maximum number of outstanding reads
        .notFull(),
        .almostFull(),

        .first(pw_fifo_idx),
        .deq_en(pw_fifo_deq),
        .notEmpty(pw_fifo_notEmpty)
        );

    // Track partial write request coming from wr_req RAM and pw_fifo.
    logic pw_fifo_deq_q;
    logic pw_fifo_deq_qq;

    always_ff @(posedge clk)
    begin
        pw_fifo_deq_q <= pw_fifo_deq;
        pw_fifo_deq_qq <= pw_fifo_deq_q;

        if (reset)
        begin
            pw_fifo_deq_q <= 1'b0;
            pw_fifo_deq_qq <= 1'b0;
        end
    end


    //
    // Determine when head of c1_fifo can be processed
    //
    always_comb
    begin
        if (! c1Tx_is_pwrite)
        begin
            // Normal write request case
            c1Tx_deq = cci_mpf_c1TxIsValid(c1Tx) && ! fiu.c1TxAlmFull &&
                       // Partial writes with valid read data have priority
                       ! pw_fifo_deq_qq;
        end
        else
        begin
            // c1Tx is a partial write.  It will go to the read pipeline
            // to fill in missing data and to the write request heap to
            // be resubmitted when the read returns.
            c1Tx_deq = may_inject_read;
        end
    end


    // Forward write requests toward the FIU.  Don't forward partial write
    // requests.  They will go out as reads as part of the read-modify-write
    // sequence.
    always_ff @(posedge clk)
    begin
        fiu.c1Tx <= cci_mpf_c1TxMaskValids(c1Tx,
                                           ! fiu.c1TxAlmFull &&
                                           ! c1Tx_is_pwrite);

        // Has the read completed to fill in the remainder of a partial write?
        // If so, send the full line write toward the FIU.
        if (pw_fifo_deq_qq)
        begin
            fiu.c1Tx <= pw_wreq;

            if (DEBUG && ! reset)
            begin
                $display("PWRITE: Write addr 0x%x, widx 0x%x, mdata 0x%x",
                         pw_wreq.hdr.base.address,
                         t_write_heap_idx'(pw_wreq.data),
                         pw_wreq.hdr.base.mdata);
            end
        end

        // Partial write has been handled
        fiu.c1Tx.hdr.pwrite.isPartialWrite <= 1'b0;

        if (reset)
        begin
            fiu.c1Tx.valid <= 1'b0;
        end
    end

    assign afu.c1Rx = fiu.c1Rx;

endmodule // cci_mpf_shim_pwrite


module cci_mpf_shim_pwrite_eop_tracker
  #(
    parameter N_WRITE_HEAP_ENTRIES = 0
    )
   (
    input  logic clk,
    input  logic reset,
    output logic rdy,

    // Note a new response
    input  logic rsp_en,
    input  logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] rsp_idx,
    input  t_ccip_clLen rsp_len,

    // Is the response the last in the packet?  Output generated the
    // same cycle as the EOP rsp_en is valid.
    output logic isEOP
    );

    t_ccip_clNum num_recvd;
    assign isEOP = (num_recvd === t_cci_clLen'(rsp_len));

    cci_mpf_prim_lutram_init
      #(
        .N_ENTRIES(N_WRITE_HEAP_ENTRIES),
        .N_DATA_BITS($bits(t_ccip_clNum))
        )
      ram
       (
        .clk,
        .reset,
        .rdy,

        .raddr(rsp_idx),
        .rdata(num_recvd),

        .waddr(rsp_idx),
        .wen(rsp_en),
        // If response is last then reinitialize the counter to 0.  Otherwise,
        // increment the flit counter.
        .wdata(isEOP ? t_cci_clNum'(0) : num_recvd + t_cci_clNum'(1))
        );

endmodule // cci_mpf_shim_pwrite_eop_tracker
