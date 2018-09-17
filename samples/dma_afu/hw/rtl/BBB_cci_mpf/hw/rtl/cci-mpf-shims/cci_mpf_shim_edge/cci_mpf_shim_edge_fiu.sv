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
`include "cci_mpf_shim_edge.vh"
`include "cci_mpf_shim_pwrite.vh"

//
// This is a mandatory connection at the tail of an MPF pipeline.
// It canonicalizes input and output and manages the flow of data through
// MPF.
//

module cci_mpf_shim_edge_fiu
  #(
    parameter ENABLE_PARTIAL_WRITES = 0,
    parameter N_WRITE_HEAP_ENTRIES = 0,

    // The VTP page table walker needs to generate loads in order to read
    // the page table.  The reserved bit in Mdata is a location offered
    // to the page table walker to tag internal loads.  The Mdata location
    // must be zero on all requests flowing into this module through
    // the MPF pipeline.
    //
    // Some shims (e.g. cci_mpf_shim_sort_responses) already manage Mdata and
    // guarantee that some high bits will be zero.
    parameter RESERVED_MDATA_IDX = -1
    )
   (
    input  logic clk,

    // External connection to the FIU
    cci_mpf_if.to_fiu fiu,

    // Connection toward the AFU
    cci_mpf_if.to_afu afu,

    // Interface to the MPF AFU edge module
    cci_mpf_shim_edge_if.edge_fiu afu_edge,

    // Interface to the VTP page table walker.  The page table requests
    // host memory page table reads through this connection.
    cci_mpf_shim_vtp_pt_walk_if.mem_read pt_walk,

    // Interface to the partial write emulator
    cci_mpf_shim_pwrite_if.pwrite_edge_fiu pwrite
    );

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    cci_mpf_if afu_buf (.clk);
    assign afu_buf.reset = reset;

    // Insert a buffer and flow control
    logic deqC0Tx;
    logic deqC1Tx;

    cci_mpf_shim_buffer_afu
      #(
        .THRESHOLD(CCI_TX_ALMOST_FULL_THRESHOLD + 4),
        .ENABLE_C0_BYPASS(1)
        )
      b
       (
        .clk,
        .afu_raw(afu),
        .afu_buf,
        .deqC0Tx,
        .deqC1Tx
        );

    // Almost full signals in the buffered input are ignored.
    assign afu_buf.c0TxAlmFull = 1'b1;
    assign afu_buf.c1TxAlmFull = 1'b1;

    //
    // The AFU edge forwards write data to this module, routing the data
    // around the MPF pipeline.  The AFU edge manages the indices used
    // to store the data.  The data storage (block RAM) is here.
    //
    typedef logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] t_write_heap_idx;

    logic wr_heap_deq_en;
    t_write_heap_idx wr_heap_deq_idx;
    t_cci_clNum wr_heap_deq_clNum;
    t_cci_clData wr_data;

    //
    // The true number of write heap entries is larger than
    // N_WRITE_HEAP_ENTRIES because only one logical slot is used, even
    // for multi-beat writes.  Multi-beat writes share a heap index and
    // the index and cl_num for a flit are concatenated to form the
    // data's heap address.
    //
    localparam N_UNIQUE_WRITE_HEAP_ENTRIES =
        N_WRITE_HEAP_ENTRIES * CCI_MAX_MULTI_LINE_BEATS;

    // Heap data, addressed using the indices handed out by wr_heap_ctrl.
    typedef logic [(CCI_CLDATA_WIDTH / 8)-1 : 0] t_cldata_byteena;

    //
    // The write port is multiplexed between normal write data and, when
    // enabled, partial write updates.  When partial writes are disabled
    // the write updates are sent directly to the wr_heap_data write port.
    // When partial writes are enabled a module is instantiated to manage
    // the multiplexing.
    //
    // Store the full line coming from the AFU in the local buffer whether
    // or not it is a partial write.  Partial writes will trigger a
    // read-modify-write, with the read-modify ultimately resulting in
    // a masked write to port 1 of wr_heap_data.
    //
    logic heap_wen;
    t_cldata_byteena heap_wbyteena;
    logic [$clog2(N_UNIQUE_WRITE_HEAP_ENTRIES)-1 : 0] heap_waddr;
    t_cci_clData heap_wdata;

    generate
        if (! ENABLE_PARTIAL_WRITES)
        begin : npw
            // No partial write support.  Connect incoming write data directly
            // to the heap.
            always_ff @(posedge clk)
            begin
                heap_wen <= afu_edge.wen;
                heap_waddr <= { afu_edge.widx, afu_edge.wclnum };
                heap_wdata <= afu_edge.wdata;

                // Free slots once write exits
                afu_edge.free <= wr_heap_deq_en;
                afu_edge.freeidx <= wr_heap_deq_idx;

                assert (reset || ! pwrite.upd_en) else
                    $fatal("cci_mpf_shim_edge_fiu.sv: Partial write request but feature is disabled!");

                if (reset)
                begin
                    heap_wen <= 1'b0;
                    afu_edge.free <= 1'b0;
                end
            end

            assign heap_wbyteena = ~ (t_cldata_byteena'(0));
            assign afu_edge.wAlmFull = 1'b0;
        end
        else
        begin : pw
            // Partial writes enabled.  Build a MUX to combine normal write
            // data and partial write updates into a single port.
            cci_mpf_shim_edge_fiu_pwrite_mux
              #(
                .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES),
                .N_UNIQUE_WRITE_HEAP_ENTRIES(N_UNIQUE_WRITE_HEAP_ENTRIES)
                )
              pw_mux
               (
                .clk,
                .reset,
                .afu_edge,
                .pwrite,
                .wr_heap_deq_en,
                .wr_heap_deq_idx,

                .heap_wen,
                .heap_wbyteena,
                .heap_waddr,
                .heap_wdata
                );
        end
    endgenerate

    logic [$clog2(N_UNIQUE_WRITE_HEAP_ENTRIES)-1 : 0] wr_heap_data_addr1;

    cci_mpf_prim_ram_dualport_byteena
      #(
        .N_ENTRIES(N_UNIQUE_WRITE_HEAP_ENTRIES),
        .N_DATA_BITS(CCI_CLDATA_WIDTH),
        .N_OUTPUT_REG_STAGES(1),
        .OPERATION_MODE("DUAL_PORT")
        )
      wr_heap_data
       (
        .clk0(clk),
        .clk1(clk),

        .wen0(heap_wen),
        .byteena0(heap_wbyteena),
        .addr0(heap_waddr),
        .wdata0(heap_wdata),
        .rdata0(),

        .addr1({ wr_heap_deq_idx, wr_heap_deq_clNum }),
        .rdata1(wr_data),
        .wen1(),
        .byteena1(),
        .wdata1()
        );


    // ====================================================================
    //
    //   FIU edge flow
    //
    // ====================================================================

    //
    // Channel 0 (reads)
    //

    // Reads flow through undisturbed except for the need to inject reads
    // requested by the page table walker here.

    logic pt_walk_read_req;
    logic pt_walk_emit_req;
    t_cci_clAddr pt_walk_read_addr;

    assign pt_walk.readRdy = ! pt_walk_read_req;

    //
    // Track requests to read a line in the page table.
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            pt_walk_read_req <= 1'b0;
        end
        else
        begin
            // Either a request completed or a new one arrived
            pt_walk_read_req <= (pt_walk_read_req ^ pt_walk_emit_req) ||
                                pt_walk.readEn;
        end

        // Register requested address
        if (pt_walk.readEn)
        begin
            pt_walk_read_addr <= pt_walk.readAddr;
        end
    end

    // Emit the read for PT walk if a request is outstanding.
    assign pt_walk_emit_req = pt_walk_read_req && ! fiu.c0TxAlmFull;

    // Request header for PT walk reads
    t_cci_mpf_c0_ReqMemHdr pt_walk_read_hdr;
    always_comb
    begin
        t_cci_mdata m;
        m = cci_mpf_setShimMdataTag(RESERVED_MDATA_IDX, CCI_MPF_SHIM_TAG_VTP);

        pt_walk_read_hdr = cci_mpf_c0_genReqHdr(eREQ_RDLINE_I,
                                                pt_walk_read_addr,
                                                m,
                                                cci_mpf_defaultReqHdrParams(0));
    end

    //
    // Forward read requests
    //
    always_comb
    begin
        fiu.c0Tx = cci_mpf_c0TxMaskValids(cci_mpf_updC0TxCanonical(afu_buf.c0Tx),
                                          ! fiu.c0TxAlmFull);

        deqC0Tx = cci_mpf_c0TxIsValid(afu_buf.c0Tx) &&
                  ! fiu.c0TxAlmFull &&
                  ! pt_walk_emit_req;

        if (pt_walk_emit_req)
        begin
            fiu.c0Tx = cci_mpf_genC0TxReadReq(pt_walk_read_hdr, 1'b1);
        end
    end

    // Is the read response for the page table walker?
    logic is_pt_rsp;
    always_comb
    begin
        is_pt_rsp = cci_mpf_testShimMdataTag(RESERVED_MDATA_IDX,
                                             CCI_MPF_SHIM_TAG_VTP,
                                             fiu.c0Rx.hdr.mdata);

        pt_walk.readDataEn = cci_c0Rx_isReadRsp(fiu.c0Rx) && is_pt_rsp;
        pt_walk.readData = fiu.c0Rx.data;
    end

    always_comb
    begin
        afu_buf.c0Rx = fiu.c0Rx;

        // Only forward client-generated read responses
        afu_buf.c0Rx.rspValid = fiu.c0Rx.rspValid && ! is_pt_rsp;
    end


    //
    // Channel 1 (writes)
    //
    //   Multi-beat writes complete by synthesizing new packets and reading
    //   data from wr_heap.
    //

    // Responses
    assign afu_buf.c1Rx = fiu.c1Rx;

    //
    // Pick the next write request to forward.  Either the request is
    // a synthesized request to complete a multi-beat write or it is
    // a new request from the FIFO.
    //
    t_ccip_clNum stg1_fiu_wr_beat_idx;
    t_ccip_clNum stg1_fiu_wr_beats_rem;
    logic stg1_fiu_c1Tx_sop;
    logic stg1_packet_done;
    logic stg1_packet_is_new;
    logic stg1_flit_en;
    logic wr_req_may_fire;

    // Pipeline stages
    t_if_cci_mpf_c1_Tx stg1_fiu_c1Tx;
    t_if_cci_mpf_c1_Tx stg2_fiu_c1Tx;
    t_if_cci_mpf_c1_Tx stg3_fiu_c1Tx;

    always_comb
    begin
        // Write request may fire if the downstream pipeline isn't full and
        // if the wr_heap_data read port is available to retrieve the store
        // data.  The wr_heap_data read port is used as a write port for
        // updates to partial write data, which are given priority.
        wr_req_may_fire = ! fiu.c1TxAlmFull;

        // Processing is complete when all beats have been emitted.
        stg1_packet_done = wr_req_may_fire && (stg1_fiu_wr_beats_rem == 0);

        // Ready to process a write flit?
        stg1_flit_en = wr_req_may_fire && cci_mpf_c1TxIsWriteReq(stg1_fiu_c1Tx);
        wr_heap_deq_clNum = stg1_fiu_wr_beat_idx;

        // Release the write data heap entry when a write retires
        wr_heap_deq_en = stg1_packet_done && cci_mpf_c1TxIsWriteReq(stg1_fiu_c1Tx);

        // Take the next request from the buffering FIFO when the current
        // packet is done or there is no packet being processed.
        deqC1Tx = cci_mpf_c1TxIsValid(afu_buf.c1Tx) &&
                  (stg1_packet_done || ! cci_mpf_c1TxIsValid(stg1_fiu_c1Tx));
    end


    // Pipeline c1 Tx requests, waiting for heap data.
    t_if_cci_mpf_c1_Tx c1Tx;
    assign c1Tx = cci_mpf_updC1TxCanonical(afu_buf.c1Tx);

    always_ff @(posedge clk)
    begin
        stg1_packet_is_new <= deqC1Tx;

        // Head of the pipeline
        if (deqC1Tx)
        begin
            // Pipeline is moving and a new request is available
            stg1_fiu_c1Tx <= c1Tx;

            // The heap data for the SOP is definitely available
            // since it arrived with the header.
            stg1_fiu_c1Tx_sop <= 1'b1;
            stg1_fiu_wr_beat_idx <= 0;
            stg1_fiu_wr_beats_rem <= c1Tx.hdr.base.cl_len;

            wr_heap_deq_idx <= t_write_heap_idx'(c1Tx.data);
        end
        else if (stg1_packet_done)
        begin
            // Pipeline is moving but no new request is available
            stg1_fiu_c1Tx <= cci_c1Tx_clearValids();
        end
        else if (stg1_flit_en)
        begin
            // In the middle of a multi-beat request
            stg1_fiu_c1Tx_sop <= 1'b0;
            stg1_fiu_wr_beat_idx <= stg1_fiu_wr_beat_idx + 1;
            stg1_fiu_wr_beats_rem <= stg1_fiu_wr_beats_rem - 1;
        end

        if (wr_req_may_fire)
        begin
            // Write request this cycle
            stg2_fiu_c1Tx <= stg1_fiu_c1Tx;
            // SOP set only first first beat in a multi-beat packet.
            // Only write requests use SOP.
            stg2_fiu_c1Tx.hdr.base.sop <=
                cci_mpf_c1TxIsWriteReq_noCheckValid(stg1_fiu_c1Tx) &&
                stg1_fiu_c1Tx_sop;
            // Low bits of aligned address reflect the beat
            stg2_fiu_c1Tx.hdr.base.address[$bits(t_ccip_clNum)-1 : 0] <=
                stg1_fiu_c1Tx.hdr.base.address[$bits(t_ccip_clNum)-1 : 0] |
                stg1_fiu_wr_beat_idx;
        end
        else
        begin
            // Nothing starting this cycle
            stg2_fiu_c1Tx <= cci_c1Tx_clearValids();
        end

        stg3_fiu_c1Tx <= stg2_fiu_c1Tx;

        if (reset)
        begin
            stg1_fiu_c1Tx_sop <= 1'b1;
            stg1_fiu_wr_beat_idx <= 1'b0;
            stg1_fiu_wr_beats_rem <= 1'b0;

            stg1_packet_is_new <= 1'b0;
            stg1_fiu_c1Tx <= cci_c1Tx_clearValids();
            stg2_fiu_c1Tx <= cci_c1Tx_clearValids();
            stg3_fiu_c1Tx <= cci_c1Tx_clearValids();
        end
    end


    // Merge FIU-bound data with request
    always_comb
    begin
        //
        // It ought to be possible to write this block as:
        //
        //     fiu.c1Tx = stg3_fiu_c1Tx;
        //     fiu.c1Tx.data = wr_data;
        //
        // unfortunately, QuestaSim 10.6 ignores the data write.  Separating
        // the field assignments works, so:
        //
        fiu.c1Tx.valid = stg3_fiu_c1Tx.valid;
        fiu.c1Tx.hdr = stg3_fiu_c1Tx.hdr;
        fiu.c1Tx.data = wr_data;
    end


    //
    // Channel 2 (MMIO)
    //

    always_ff @(posedge clk)
    begin
        fiu.c2Tx <= afu_buf.c2Tx;
    end


    //
    // Validate parameter settings and that the Mdata VTP reservation is free
    // on all incoming read requests.
    //
    always_ff @(posedge clk)
    begin
        assert ((RESERVED_MDATA_IDX >= $bits(t_cci_mpf_shim_tag)) &&
                (RESERVED_MDATA_IDX < CCI_MDATA_WIDTH)) else
            $fatal("cci_mpf_shim_edge_fiu.sv: Illegal RESERVED_MDATA_IDX value: %d", RESERVED_MDATA_IDX);

        if (! reset)
        begin
            assert(! cci_mpf_c0TxIsValid(afu.c0Tx) ||
                   ! cci_mpf_testShimMdataTag(RESERVED_MDATA_IDX,
                                              CCI_MPF_SHIM_TAG_VTP,
                                              afu.c0Tx.hdr.base.mdata))
            else
                $fatal("cci_mpf_shim_edge_fiu.sv: AFU C0 VTP tag already used!");
        end
    end

endmodule // cci_mpf_shim_edge_fiu


//
// Manage write buffer's write port MUX between new write data and partial
// write updates from the pwrite port.
//
module cci_mpf_shim_edge_fiu_pwrite_mux
  #(
    parameter N_WRITE_HEAP_ENTRIES = 0,
    parameter N_UNIQUE_WRITE_HEAP_ENTRIES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    cci_mpf_shim_edge_if.edge_fiu afu_edge,
    cci_mpf_shim_pwrite_if.pwrite_edge_fiu pwrite,

    input  logic wr_heap_deq_en,
    input  logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] wr_heap_deq_idx,

    output logic heap_wen,
    output logic [(CCI_CLDATA_WIDTH / 8)-1 : 0] heap_wbyteena,
    output logic [$clog2(N_UNIQUE_WRITE_HEAP_ENTRIES)-1 : 0] heap_waddr,
    output t_cci_clData heap_wdata
    );

    typedef logic [$clog2(N_WRITE_HEAP_ENTRIES)-1 : 0] t_write_heap_idx;
    typedef logic [$clog2(N_UNIQUE_WRITE_HEAP_ENTRIES)-1 : 0] t_unique_write_heap_idx;
    typedef logic [(CCI_CLDATA_WIDTH / 8)-1 : 0] t_cldata_byteena;


    //
    // Priority in the MUX goes to partial write updates.  Normal write
    // data is buffered in this FIFO and the usual almost full protocol
    // prevents overflow.
    //
    logic fifo_notEmpty;
    t_unique_write_heap_idx fifo_waddr;
    t_cci_clData fifo_wdata;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_unique_write_heap_idx) + $bits(t_cci_clData)),
        .N_ENTRIES(CCI_TX_ALMOST_FULL_THRESHOLD + 6),
        // In normal conditions the FIFO drains immediately.  Signal almost
        // full as soon as partial writes cause the FIFO to block.
        .THRESHOLD(CCI_TX_ALMOST_FULL_THRESHOLD + 4)
        )
      afu_wdata_fifo
       (
        .clk,
        .reset,

        .enq_data({ afu_edge.widx, afu_edge.wclnum, afu_edge.wdata }),
        .enq_en(afu_edge.wen),
        .notFull(),
        .almostFull(afu_edge.wAlmFull),

        .first({ fifo_waddr, fifo_wdata }),
        .deq_en(fifo_notEmpty && ! pwrite.upd_en),
        .notEmpty(fifo_notEmpty)
        );


    //
    // Choose between pwrite and normal data write.
    //
    always_ff @(posedge clk)
    begin
        heap_wen <= pwrite.upd_en || fifo_notEmpty;

        if (pwrite.upd_en)
        begin
            heap_wbyteena <= pwrite.upd_mask;
            heap_waddr <= { pwrite.upd_idx, pwrite.upd_clNum };
            heap_wdata <= pwrite.upd_data;
        end
        else
        begin
            heap_wbyteena <= ~ (t_cldata_byteena'(0));
            heap_waddr <= fifo_waddr;
            heap_wdata <= fifo_wdata;
        end

        if (reset)
        begin
            heap_wen <= 1'b0;
        end
    end


    //
    // Free slots once write exits
    //
    always_ff @(posedge clk)
    begin
        afu_edge.free <= wr_heap_deq_en;
        afu_edge.freeidx <= wr_heap_deq_idx;

        if (reset)
        begin
            afu_edge.free <= 1'b0;
        end
    end

endmodule // cci_mpf_shim_edge_fiu_pwrite_mux
