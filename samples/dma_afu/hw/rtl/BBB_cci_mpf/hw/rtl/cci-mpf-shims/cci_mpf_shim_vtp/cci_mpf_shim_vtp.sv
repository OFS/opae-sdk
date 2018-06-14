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
`include "cci_mpf_shim_vtp.vh"
`include "cci_mpf_prim_hash.vh"

`include "cci_mpf_config.vh"

//
// Virtual to physical pipeline shim performs address translation in
// an AFU -> FIU stream by forwarding translation requests to the VTP
// service.
//

module cci_mpf_shim_vtp
  #(
    parameter AFU_BUF_THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD + 4
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    // VTP service translation ports - one for each channel
    cci_mpf_shim_vtp_svc_if.client vtp_svc[0 : 1],

    // CSRs
    cci_mpf_csrs.vtp csrs
    );

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end


    // ====================================================================
    //
    //  Instantiate a buffer on the AFU request port, making it latency
    //  insensitive.
    //
    // ====================================================================

    cci_mpf_if afu_buf (.clk);
    assign afu_buf.reset = reset;

    // Latency-insensitive ports need explicit dequeue (enable).
    logic deqC0Tx;
    logic deqC1Tx;

    cci_mpf_shim_buffer_afu
      #(
        .THRESHOLD(AFU_BUF_THRESHOLD),
        .ENABLE_C0_BYPASS(1),
        .REGISTER_OUTPUT(1)
        )
      b
        (
         .clk,
         .afu_raw(afu),
         .afu_buf(afu_buf),
         .deqC0Tx(deqC0Tx),
         .deqC1Tx(deqC1Tx)
         );

    //
    // Almost full signals in the buffered input are ignored --
    // replaced by deq signals and the buffer state.  Set them
    // to 1 to be sure they are ignored.
    //
    assign afu_buf.c0TxAlmFull = 1'b1;
    assign afu_buf.c1TxAlmFull = 1'b1;


    // ====================================================================
    //
    //  Channel 0 (reads)
    //
    // ====================================================================

    logic c0chan_notFull;

    logic c0chan_outValid;
    t_if_cci_mpf_c0_Tx c0chan_outTx;
    t_tlb_4kb_pa_page_idx c0chan_outAddr;
    logic c0chan_outAddrIsBigPage;

    // Pass TX requests through a translation pipeline
    cci_mpf_shim_vtp_chan
      #(
        .N_META_BITS($bits(t_if_cci_mpf_c0_Tx)),
        .N_LOCAL_4KB_CACHE_ENTRIES(`VTP_N_C0_L1_4KB_CACHE_ENTRIES),
        .N_LOCAL_2MB_CACHE_ENTRIES(`VTP_N_C0_L1_2MB_CACHE_ENTRIES)
        )
      c0_vtp
       (
        .clk,
        .reset,

        .notFull(c0chan_notFull),
        .notEmpty(),

        .cTxValid(deqC0Tx),
        .cTx(afu_buf.c0Tx),
        .cTxAddr(vtp4kbPageIdxFromVA(cci_mpf_c0_getReqAddr(afu_buf.c0Tx.hdr))),
        .cTxAddrIsVirtual(cci_mpf_c0_getReqAddrIsVirtual(afu_buf.c0Tx.hdr)),

        .cTxValid_out(c0chan_outValid),
        .cTx_out(c0chan_outTx),
        .cTxAddr_out(c0chan_outAddr),
        .cTxAddrIsBigPage_out(c0chan_outAddrIsBigPage),
        .cTxAlmostFull(fiu.c0TxAlmFull),

        .vtp_svc(vtp_svc[0]),
        .csrs
        );

    // Send the next request when the channel has space
    assign deqC0Tx = cci_mpf_c0TxIsValid(afu_buf.c0Tx) && c0chan_notFull;

    // Route translated requests to the FIU
    always_ff @(posedge clk)
    begin
        fiu.c0Tx <= cci_mpf_c0TxMaskValids(c0chan_outTx, c0chan_outValid);

        // Set the physical address.  The page comes from the TLB and the
        // offset from the original memory request.
        fiu.c0Tx.hdr.ext.addrIsVirtual <= 1'b0;
        if (cci_mpf_c0_getReqAddrIsVirtual(c0chan_outTx.hdr))
        begin
            if (c0chan_outAddrIsBigPage)
            begin
                // 2MB page
                fiu.c0Tx.hdr.base.address <=
                    t_cci_clAddr'({ vtp4kbTo2mbPA(c0chan_outAddr),
                                    vtp2mbPageOffsetFromVA(cci_mpf_c0_getReqAddr(c0chan_outTx.hdr)) });
            end
            else
            begin
                // 4KB page
                fiu.c0Tx.hdr.base.address <=
                    t_cci_clAddr'({ c0chan_outAddr,
                                    vtp4kbPageOffsetFromVA(cci_mpf_c0_getReqAddr(c0chan_outTx.hdr)) });
            end
        end
    end


    //
    // Responses
    //
    assign afu_buf.c0Rx = fiu.c0Rx;


    // ====================================================================
    //
    //  Channel 1 (writes)
    //
    // ====================================================================

    logic c1chan_notFull;
    logic c1chan_notEmpty;

    // Channel 1 has more logic controlling the input pipeline flow, such
    // as whether a request is a WrFence.  Add a stage in which control
    // state is reduced.
    t_if_cci_mpf_c1_Tx c1chan_inTx;
    logic c1chan_inBlocked;

    logic c1chan_inTx_canFwd;
    assign c1chan_inTx_canFwd = ! c1chan_inBlocked && c1chan_notFull;

    // Next request is either from the AFU or is sitting in c1chan_inTx
    // waiting for permission to fire.
    t_if_cci_mpf_c1_Tx c1chan_inTx_next;
    assign c1chan_inTx_next =
        (deqC1Tx ? afu_buf.c1Tx :
                   cci_mpf_c1TxMaskValids(c1chan_inTx, ! c1chan_inTx_canFwd));

    always_ff @(posedge clk)
    begin
        c1chan_inTx <= c1chan_inTx_next;

        if (reset)
        begin
            c1chan_inTx.valid <= 1'b0;
        end
    end

    // Block order-sensitive requests until all previous translations are
    // complete so that they aren't reordered in the VTP channel pipeline.
    logic c1_order_sensitive;
    assign c1_order_sensitive =
        cci_mpf_c1TxIsWriteFenceReq(deqC1Tx ? afu_buf.c1Tx : c1chan_inTx);

    always_ff @(posedge clk)
    begin
        c1chan_inBlocked <= (c1_order_sensitive && c1chan_notEmpty);

        if (reset)
        begin
            c1chan_inBlocked <= 1'b0;
        end
    end


    logic c1chan_outValid;
    t_if_cci_mpf_c1_Tx c1chan_outTx;
    t_tlb_4kb_pa_page_idx c1chan_outAddr;
    logic c1chan_outAddrIsBigPage;

    // Pass TX requests through a translation pipeline
    cci_mpf_shim_vtp_chan
      #(
        .N_META_BITS($bits(t_if_cci_mpf_c1_Tx)),
        .N_LOCAL_4KB_CACHE_ENTRIES(`VTP_N_C1_L1_4KB_CACHE_ENTRIES),
        .N_LOCAL_2MB_CACHE_ENTRIES(`VTP_N_C1_L1_2MB_CACHE_ENTRIES)
        )
      c1_vtp
       (
        .clk,
        .reset,

        .notFull(c1chan_notFull),
        .notEmpty(c1chan_notEmpty),

        .cTxValid(cci_mpf_c1TxIsValid(c1chan_inTx) && c1chan_inTx_canFwd),
        .cTx(c1chan_inTx),
        .cTxAddr(vtp4kbPageIdxFromVA(cci_mpf_c1_getReqAddr(c1chan_inTx.hdr))),
        .cTxAddrIsVirtual(cci_mpf_c1_getReqAddrIsVirtual(c1chan_inTx.hdr)),

        .cTxValid_out(c1chan_outValid),
        .cTx_out(c1chan_outTx),
        .cTxAddr_out(c1chan_outAddr),
        .cTxAddrIsBigPage_out(c1chan_outAddrIsBigPage),
        .cTxAlmostFull(fiu.c1TxAlmFull),

        .vtp_svc(vtp_svc[1]),
        .csrs
        );

    // Ready for next request?
    assign deqC1Tx = cci_mpf_c1TxIsValid(afu_buf.c1Tx) && c1chan_inTx_canFwd;

    // Route translated requests to the FIU
    always_ff @(posedge clk)
    begin
        fiu.c1Tx <= cci_mpf_c1TxMaskValids(c1chan_outTx, c1chan_outValid);


        // Set the physical address.  The page comes from the TLB and the
        // offset from the original memory request.
        fiu.c1Tx.hdr.ext.addrIsVirtual <= 1'b0;
        if (cci_mpf_c1_getReqAddrIsVirtual(c1chan_outTx.hdr))
        begin
            if (c1chan_outAddrIsBigPage)
            begin
                // 2MB page
                fiu.c1Tx.hdr.base.address <=
                    t_cci_clAddr'({ vtp4kbTo2mbPA(c1chan_outAddr),
                                    vtp2mbPageOffsetFromVA(cci_mpf_c1_getReqAddr(c1chan_outTx.hdr)) });
            end
            else
            begin
                // 4KB page
                fiu.c1Tx.hdr.base.address <=
                    t_cci_clAddr'({ c1chan_outAddr,
                                    vtp4kbPageOffsetFromVA(cci_mpf_c1_getReqAddr(c1chan_outTx.hdr)) });
            end
        end
    end


    //
    // Responses
    //
    assign afu_buf.c1Rx = fiu.c1Rx;


    // ====================================================================
    //
    //  MMIO (c2Tx)
    //
    // ====================================================================

    assign fiu.c2Tx = afu_buf.c2Tx;

endmodule // cci_mpf_shim_vtp


//
// TLB lookup for a single channel.  The code is independent of the request
// channel data structures so many be instantiated for either.
//
// A simple direct mapped cache is maintained as a first level TLB.
// The L1 TLB here filters translation requests in order to relieve
// pressure on the shared VTP TLB service.
//
module cci_mpf_shim_vtp_chan
  #(
    parameter N_META_BITS = 0,
    parameter N_LOCAL_4KB_CACHE_ENTRIES = 512,
    parameter N_LOCAL_2MB_CACHE_ENTRIES = 512
    )
   (
    input  logic clk,
    input  logic reset,

    // Flow control
    output logic notFull,
    output logic notEmpty,

    // Abstraction of a TX channel
    input  logic cTxValid,
    input  logic [N_META_BITS-1 : 0] cTx,
    input  t_tlb_4kb_va_page_idx cTxAddr,
    input  logic cTxAddrIsVirtual,

    // Outbound TX channel
    output logic cTxValid_out,
    output logic [N_META_BITS-1 : 0] cTx_out,
    output t_tlb_4kb_pa_page_idx cTxAddr_out,
    output logic cTxAddrIsBigPage_out,
    input  logic cTxAlmostFull,

    // Translation service connection
    cci_mpf_shim_vtp_svc_if.client vtp_svc,

    // CSRs
    cci_mpf_csrs.vtp csrs
    );

    // ====================================================================
    //
    //  State to be recorded through the pipeline
    //
    // ====================================================================

    // Struct for passing state through the pipeline
    typedef struct
    {
        // Input state
        logic cTxValid;
        t_tlb_4kb_va_page_idx cTxAddr;
        logic cTxAddrIsVirtual;

        // Heap state
        t_cci_mpf_shim_vtp_req_tag allocIdx;
    }
    t_vtp_shim_chan_state;

    localparam MAX_STAGE = 3;
    t_vtp_shim_chan_state state[0 : MAX_STAGE];


    logic tlb_lookup_rsp_rdy;

    logic [N_META_BITS-1 : 0] cTx_q;
    always_ff @(posedge clk)
    begin
        cTx_q <= cTx;
    end

    always_comb
    begin
        state[0].cTxValid = cTxValid;
        state[0].cTxAddr = cTxAddr;
        state[0].cTxAddrIsVirtual = cTxAddrIsVirtual;
    end

    genvar s;
    generate
        for (s = 1; s <= MAX_STAGE; s = s + 1)
        begin : st
            always_ff @(posedge clk)
            begin
                state[s].cTxValid <= state[s - 1].cTxValid;

                state[s].cTxAddr <= state[s - 1].cTxAddr;
                state[s].cTxAddrIsVirtual <= state[s - 1].cTxAddrIsVirtual;
                state[s].allocIdx <= state[s - 1].allocIdx;

                if (reset)
                begin
                    state[s].cTxValid <= 1'b0;
                end
            end
        end
    endgenerate


    // ====================================================================
    //
    //  L1 TLB caches
    //
    // ====================================================================

    logic l1_caches_rdy;

    logic l1_hit_4kb;
    logic l1_hit_2mb;
    logic l1_hit;
    t_tlb_4kb_pa_page_idx l1_hit_pa;

    logic l1_en_insert_4kb;
    logic l1_en_insert_2mb;
    t_tlb_4kb_va_page_idx l1_insert_va;
    t_tlb_4kb_pa_page_idx l1_insert_pa;
    logic l1_insert_is_valid;

    cci_mpf_shim_vtp_chan_l1_caches
      #(
        .N_LOCAL_4KB_CACHE_ENTRIES(N_LOCAL_4KB_CACHE_ENTRIES),
        .N_LOCAL_2MB_CACHE_ENTRIES(N_LOCAL_2MB_CACHE_ENTRIES)
        )
      l1_caches
       (
        .clk,
        .reset,

        .rdy(l1_caches_rdy),

        .lookupVA(state[0].cTxAddr),
        .T3_hit_4kb(l1_hit_4kb),
        .T3_hit_2mb(l1_hit_2mb),
        .T3_hit(l1_hit),
        .T3_hitPA(l1_hit_pa),

        .insertVA(l1_insert_va),
        .insertPA(l1_insert_pa),
        .en_insert_4kb(l1_en_insert_4kb),
        .en_insert_2mb(l1_en_insert_2mb),
        .insertAddrIsValid(l1_insert_is_valid),

        .csrs
        );


    // ====================================================================
    //
    //  Heap for holding TX state
    //
    // ====================================================================

    t_cci_mpf_shim_vtp_req_tag freeIdx;
    logic heap_notFull;
    t_tlb_4kb_va_page_idx cTxAddr_va_out;

    // Some of the notFull logic can be registered for timing
    logic notFull_reg;
    assign notFull = heap_notFull && notFull_reg && ! cTxAlmostFull;

    always_ff @(posedge clk)
    begin
        notFull_reg <= vtp_svc.lookupRdy && l1_caches_rdy &&
                       ! tlb_lookup_rsp_rdy;

        if (reset)
        begin
            notFull_reg <= 1'b0;
        end
    end

    // Heap indices are allocated in cycle 0
    cci_mpf_prim_heap_ctrl
      #(
        .N_ENTRIES(CCI_MPF_SHIM_VTP_MAX_SVC_REQS)
        )
      heap_ctrl
       (
        .clk,
        .reset,

        .enq(state[0].cTxValid),
        .notFull(heap_notFull),
        .allocIdx(state[0].allocIdx),

        .free(cTxValid_out),
        .freeIdx
        );

    // Heap data is written in cycle 1. It is available in cycle 0 but
    // not needed yet, so waiting a cycle simplifies timing.
    t_cci_mpf_shim_vtp_req_tag readIdx;
    logic [N_META_BITS-1 : 0] read_cTx_out;
    t_tlb_4kb_va_page_idx read_cTxAddr_va_out;

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(CCI_MPF_SHIM_VTP_MAX_SVC_REQS),
        .N_DATA_BITS(N_META_BITS)
        )
      heap_ctx
       (
        .clk,
        .reset,

        .raddr(readIdx),
        .rdata(read_cTx_out),

        .waddr(state[1].allocIdx),
        .wen(state[1].cTxValid),
        .wdata(cTx_q)
        );

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(CCI_MPF_SHIM_VTP_MAX_SVC_REQS),
        .N_DATA_BITS($bits(t_tlb_4kb_va_page_idx))
        )
      heap_addr
       (
        .clk,
        .reset,

        .raddr(readIdx),
        .rdata(read_cTxAddr_va_out),

        .waddr(state[1].allocIdx),
        .wen(state[1].cTxValid),
        .wdata(state[1].cTxAddr)
        );

    always_ff @(posedge clk)
    begin
        cTx_out <= read_cTx_out;
        cTxAddr_va_out <= read_cTxAddr_va_out;

        freeIdx <= readIdx;
    end


    // ====================================================================
    //
    //  Send translation requests to VTP server
    //
    // ====================================================================

    // Request TLB lookup if no translation found locally (cycle 3)
    always_ff @(posedge clk)
    begin
        vtp_svc.lookupEn <= state[3].cTxValid && state[3].cTxAddrIsVirtual &&
                            ! l1_hit;

        vtp_svc.lookupReq.pageVA <= state[3].cTxAddr;
        vtp_svc.lookupReq.tag <= state[3].allocIdx;

        if (reset)
        begin
            vtp_svc.lookupEn <= 1'b0;
        end
    end

    //
    // TLB response timing is latency insensitive.  This FIFO collects
    // responses until they can be merged into the pipeline.
    //
    t_cci_mpf_shim_vtp_lookup_rsp tlb_lookup_rsp;
    t_cci_mpf_shim_vtp_lookup_rsp tlb_lookup_rsp_q;
    logic tlb_lookup_deq;
    logic tlb_lookup_deq_q;
    logic tlb_lookup_deq_qq;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_cci_mpf_shim_vtp_lookup_rsp)),
        .N_ENTRIES(CCI_MPF_SHIM_VTP_MAX_SVC_REQS)
        )
      tlb_fifo_out
       (
        .clk,
        .reset,

        .enq_data(vtp_svc.lookupRsp),
        .enq_en(vtp_svc.lookupRspValid),
        .notFull(),
        .almostFull(),

        .first(tlb_lookup_rsp),
        .deq_en(tlb_lookup_deq),
        .notEmpty(tlb_lookup_rsp_rdy)
        );

    //
    // Inject the TLB response when there is a bubble in the main pipeline.
    // Look for the bubble in cycle 2 and register the TLB response so it
    // is merged in cycle 3.
    //
    // A bubble is guaranteed to happen eventually since tlb_lookup_rsp_rdy
    // causes almostFull to be asserted on the channel.
    //
    assign tlb_lookup_deq = tlb_lookup_rsp_rdy && ! state[2].cTxValid &&
                            ! cTxAlmostFull;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            tlb_lookup_deq_q <= 1'b0;
            tlb_lookup_deq_qq <= 1'b0;
        end
        else
        begin
            tlb_lookup_deq_q <= tlb_lookup_deq;
            tlb_lookup_deq_qq <= tlb_lookup_deq_q;
        end

        tlb_lookup_rsp_q <= tlb_lookup_rsp;
    end


    // ====================================================================
    //
    //  Responses.
    //
    // ====================================================================

    //
    // Pick the main pipeline if the request doesn't need address
    // translation or the local cache held the translation.
    // If nothing is flowing on the main path then then there might be
    // a response from the TLB.  The TLB response logic above decided
    // in cycle 1 whether the main path was busy when it set
    // tlb_lookup_deq.
    //
    logic pick_main_path;
    assign pick_main_path = state[3].cTxValid &&
                            (! state[3].cTxAddrIsVirtual || l1_hit);

    // Read the full request from the heap
    always_ff @(posedge clk)
    begin
        readIdx <= tlb_lookup_deq ? tlb_lookup_rsp.tag : state[2].allocIdx;
    end

    always_ff @(posedge clk)
    begin
        cTxValid_out <= pick_main_path || tlb_lookup_deq_q;
        if (reset)
        begin
            cTxValid_out <= 1'b0;
        end

        if (pick_main_path)
        begin
            cTxAddr_out <= l1_hit_pa;
            cTxAddrIsBigPage_out <= l1_hit_2mb;
        end
        else
        begin
            cTxAddr_out <= tlb_lookup_rsp_q.pagePA;
            cTxAddrIsBigPage_out <= tlb_lookup_rsp_q.isBigPage;
        end
    end

    //
    // Set values for updating the local cache.
    //
    always_ff @(posedge clk)
    begin
        l1_en_insert_4kb <= tlb_lookup_deq_qq && ! cTxAddrIsBigPage_out;
        l1_en_insert_2mb <= tlb_lookup_deq_qq && cTxAddrIsBigPage_out;

        l1_insert_is_valid <= 1'b1;
        l1_insert_va <= cTxAddr_va_out;
        l1_insert_pa <= cTxAddr_out;

        //
        // Invalidation requested by host?  Invalidation has higher priority
        // than TLB fill.
        //
        if (csrs.vtp_in_inval_page_valid)
        begin
            l1_insert_is_valid <= 1'b0;
            l1_insert_va <= vtp4kbPageIdxFromVA(csrs.vtp_in_inval_page);

            l1_en_insert_4kb <= 1'b1;
            l1_en_insert_2mb <= 1'b1;
        end

        if (reset)
        begin
            l1_en_insert_4kb <= 1'b0;
            l1_en_insert_2mb <= 1'b0;
        end
    end


    // ====================================================================
    //
    //  Track notEmpty by counting transactions
    //
    // ====================================================================

    logic [$clog2(CCI_MPF_SHIM_VTP_MAX_SVC_REQS+1)-1 : 0] n_active;
    logic [$clog2(CCI_MPF_SHIM_VTP_MAX_SVC_REQS+1)-1 : 0] n_active_next;

    always_comb
    begin
        if ((state[0].cTxValid ^ cTxValid_out) == 1'b0)
        begin
            // No change
            n_active_next = n_active;
        end
        else if (state[0].cTxValid)
        begin
            // Only a new entry
            n_active_next = n_active + 1'b1;
        end
        else
        begin
            // Only completed an old entry
            n_active_next = n_active - 1'b1;
        end
    end

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            notEmpty <= 1'b0;
            n_active <= 0;
        end
        else
        begin
            notEmpty <= (n_active_next != 0);
            n_active <= n_active_next;
        end
    end


    // ====================================================================
    //
    //  Assertions
    //
    // ====================================================================

    always_ff @(posedge clk)
    begin
        if (! reset)
        begin
            assert (! pick_main_path || ! tlb_lookup_deq_q) else
                $fatal("cci_mpf_shim_vtp: main path and TLB path collission!");
        end
    end

endmodule


module cci_mpf_shim_vtp_chan_l1_caches
  #(
    parameter N_LOCAL_4KB_CACHE_ENTRIES = 512,
    parameter N_LOCAL_2MB_CACHE_ENTRIES = 512
    )
   (
    input  logic clk,
    input  logic reset,

    // Flow control
    output logic rdy,

    // Lookup address.  The incoming address is the index of a 4KB page.
    // Larger page size lookups will just drop low address bits.
    input  t_tlb_4kb_va_page_idx lookupVA,
    output logic T3_hit_4kb,
    output logic T3_hit_2mb,
    // Or of all size-specific hits
    output logic T3_hit,
    output t_tlb_4kb_pa_page_idx T3_hitPA,

    // Insert translation in cache.  Like lookupVA, these addresses are
    // transformed internally for page sizes larger than 4KB.
    input  t_tlb_4kb_va_page_idx insertVA,
    input  t_tlb_4kb_pa_page_idx insertPA,
    input  en_insert_4kb,
    input  en_insert_2mb,
    // Entries in the cache are marked invalid by passing en_insert when
    // insertAddrIsValid is 0.
    input  insertAddrIsValid,

    // CSRs
    cci_mpf_csrs.vtp csrs
    );

    //
    // The local cache is direct mapped.  Break a VA into cache index
    // and tag.
    //
    typedef logic [$clog2(N_LOCAL_4KB_CACHE_ENTRIES)-1 : 0] t_vtp_tlb_4kb_cache_idx;
    typedef logic [$bits(t_tlb_4kb_va_page_idx)-$bits(t_vtp_tlb_4kb_cache_idx)-1 : 0]
        t_vtp_tlb_4kb_cache_tag;

    typedef logic [$clog2(N_LOCAL_2MB_CACHE_ENTRIES)-1 : 0] t_vtp_tlb_2mb_cache_idx;
    typedef logic [$bits(t_tlb_2mb_va_page_idx)-$bits(t_vtp_tlb_2mb_cache_idx)-1 : 0]
        t_vtp_tlb_2mb_cache_tag;


    //
    // Functions to extract cache index and tag from a 4KB virtual page address
    //
    function automatic t_vtp_tlb_4kb_cache_idx cacheIdx4KB(t_tlb_4kb_va_page_idx va);
        t_vtp_tlb_4kb_cache_tag tag;
        t_vtp_tlb_4kb_cache_idx idx;
        {tag, idx} = va;
        return idx;
    endfunction

    function automatic t_vtp_tlb_4kb_cache_tag cacheTag4KB(t_tlb_4kb_va_page_idx va);
        t_vtp_tlb_4kb_cache_tag tag;
        t_vtp_tlb_4kb_cache_idx idx;
        {tag, idx} = va;
        return tag;
    endfunction

    function automatic t_vtp_tlb_2mb_cache_idx cacheIdx2MB(t_tlb_4kb_va_page_idx va);
        t_vtp_tlb_2mb_cache_tag tag;
        t_vtp_tlb_2mb_cache_idx idx;
        {tag, idx} = vtp4kbTo2mbVA(va);
        return idx;
    endfunction

    function automatic t_vtp_tlb_2mb_cache_tag cacheTag2MB(t_tlb_4kb_va_page_idx va);
        t_vtp_tlb_2mb_cache_tag tag;
        t_vtp_tlb_2mb_cache_idx idx;
        {tag, idx} = vtp4kbTo2mbVA(va);
        return tag;
    endfunction


    // ====================================================================
    //
    // Reset (invalidate) the TLB when requested by SW.
    // inval_translation_cache is held for only one cycle.
    //
    // ====================================================================

    logic n_reset_tlb[0:1];
    always @(posedge clk)
    begin
        n_reset_tlb[1] <= ~csrs.vtp_in_mode.inval_translation_cache;
        n_reset_tlb[0] <= n_reset_tlb[1];

        if (reset)
        begin
            n_reset_tlb[1] <= 1'b0;
            n_reset_tlb[0] <= 1'b0;
        end
    end


    // ====================================================================
    //
    //  Lookup state pipeline
    //
    // ====================================================================

    t_tlb_4kb_va_page_idx lookup_va[1:2];

    always_ff @(posedge clk)
    begin
        lookup_va[1] <= lookupVA;
        lookup_va[2] <= lookup_va[1];
    end


    // ====================================================================
    //
    //  Local L1 cache of 4KB page translations.
    //
    // ====================================================================

    logic cache_4kb_rdy;
    t_tlb_4kb_pa_page_idx cache_4kb_pa, T3_cache_4kb_pa;
    t_vtp_tlb_4kb_cache_tag cache_4kb_tag;
    logic cache_4kb_valid;

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(N_LOCAL_4KB_CACHE_ENTRIES),
        .N_DATA_BITS($bits(t_tlb_4kb_pa_page_idx) + $bits(t_vtp_tlb_4kb_cache_tag) + 1),
        .INIT_VALUE({ t_tlb_4kb_pa_page_idx'('x), t_vtp_tlb_4kb_cache_tag'('x), 1'b0 }),
        .N_OUTPUT_REG_STAGES(1)
        )
      cache4kb
       (
        .clk,
        .reset(~n_reset_tlb[0]),
        .rdy(cache_4kb_rdy),

        .wen(en_insert_4kb),
        .waddr(cacheIdx4KB(insertVA)),
        .wdata({ insertPA, cacheTag4KB(insertVA), insertAddrIsValid }),

        // Cache read is initiated in pipeline cycle 0
        .raddr(cacheIdx4KB(lookupVA)),
        .rdata({ cache_4kb_pa, cache_4kb_tag, cache_4kb_valid })
        );

    // Cache read data arrives in cycle 2
    always_ff @(posedge clk)
    begin
        T3_hit_4kb <= (cacheTag4KB(lookup_va[2]) == cache_4kb_tag) &&
                      cache_4kb_valid;
        T3_cache_4kb_pa <= cache_4kb_pa;
    end


    // ====================================================================
    //
    //  Local L1 cache of 2MB page translations.
    //
    // ====================================================================

    logic cache_2mb_rdy;
    t_tlb_2mb_pa_page_idx cache_2mb_pa, T3_cache_2mb_pa;
    t_vtp_tlb_2mb_cache_tag cache_2mb_tag;
    logic cache_2mb_valid;

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(N_LOCAL_2MB_CACHE_ENTRIES),
        .N_DATA_BITS($bits(t_tlb_2mb_pa_page_idx) + $bits(t_vtp_tlb_2mb_cache_tag) + 1),
        .INIT_VALUE({ t_tlb_2mb_pa_page_idx'('x), t_vtp_tlb_2mb_cache_tag'('x), 1'b0 }),
        .N_OUTPUT_REG_STAGES(1)
        )
      cache2mb
       (
        .clk,
        .reset(~n_reset_tlb[0]),
        .rdy(cache_2mb_rdy),

        .wen(en_insert_2mb),
        .waddr(cacheIdx2MB(insertVA)),
        .wdata({ vtp4kbTo2mbPA(insertPA), cacheTag2MB(insertVA), insertAddrIsValid }),

        // Cache read is initiated in pipeline cycle 0
        .raddr(cacheIdx2MB(lookupVA)),
        .rdata({ cache_2mb_pa, cache_2mb_tag, cache_2mb_valid })
        );

    // Cache read data arrives in cycle 2
    always_ff @(posedge clk)
    begin
        T3_hit_2mb <= (cacheTag2MB(lookup_va[2]) == cache_2mb_tag) &&
                      cache_2mb_valid;
        T3_cache_2mb_pa <= cache_2mb_pa;
    end


    // ====================================================================
    //
    //  Merged lookup response
    //
    // ====================================================================

    always_comb
    begin
        T3_hit = T3_hit_4kb || T3_hit_2mb;

        if (T3_hit_4kb)
        begin
            T3_hitPA = T3_cache_4kb_pa;
        end
        else
        begin
            T3_hitPA = vtp2mbTo4kbPAx(T3_cache_2mb_pa);
        end
    end

    always_ff @(posedge clk)
    begin
        rdy <= cache_4kb_rdy && cache_2mb_rdy;
        if (reset)
        begin
            rdy <= 1'b0;
        end
    end


    // ====================================================================
    //
    //  Debug
    //
    // ====================================================================

    localparam DEBUG_MESSAGES = 0;
    always_ff @(posedge clk)
    begin
        if (DEBUG_MESSAGES && ! reset)
        begin
            if (en_insert_4kb)
            begin
                if (insertAddrIsValid)
                begin
                    $display("%m: 4KB: Insert idx %0d, VA 0x%x, PA 0x%x",
                             cacheIdx4KB(insertVA),
                             {insertVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0},
                             {insertPA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0});
                end
                else
                begin
                    $display("%m: 4KB: Remove idx %0d",
                             cacheIdx4KB(insertVA));
                end
            end

            if (en_insert_2mb)
            begin
                if (insertAddrIsValid)
                begin
                    $display("%m: 2MB: Insert idx %0d, VA 0x%x, PA 0x%x",
                             cacheIdx2MB(insertVA),
                             {vtp4kbTo2mbVA(insertVA), CCI_PT_2MB_PAGE_OFFSET_BITS'(0), 6'b0},
                             {insertPA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0});
                end
                else
                begin
                    $display("%m: 2MB: Remove idx %0d",
                             cacheIdx2MB(insertVA));
                end
            end
        end
    end

endmodule // cci_mpf_shim_vtp_chan_l1_caches
