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

`include "cci_mpf_shim_vtp.vh"
`include "cci_mpf_config.vh"


//
// Construct the VTP translation service, instantiating a TLB and page table
// walker.  Each VTP client will get a pair of ports to this service: one
// for c0 and one for c1.  Though this service is pipelined, at most one
// address is translated each cycle.  Clients are expected to cache responses
// to reduce the number of requests to this service.
//

module cci_mpf_svc_vtp
  #(
    parameter N_VTP_PORTS = 0,
    parameter DEBUG_MESSAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // Clients
    cci_mpf_shim_vtp_svc_if.server vtp_svc[0 : N_VTP_PORTS-1],

    // Page table walker bus
    cci_mpf_shim_vtp_pt_walk_if.pt_walk pt_walk_walker,
    cci_mpf_shim_vtp_pt_walk_if.client pt_walk_client,

    // CSRs
    cci_mpf_csrs.vtp csrs,
    cci_mpf_csrs.vtp_events events
    );

    typedef logic [$clog2(N_VTP_PORTS)-1 : 0] t_cci_mpf_shim_vtp_port_idx;

    // ====================================================================
    //
    //   Turn multiple incoming request channels into a single stream.
    //
    // ====================================================================

    //
    // Buffer incoming requests in small FIFOs.
    //
    t_cci_mpf_shim_vtp_lookup_req new_req[0 : N_VTP_PORTS-1];
    logic [N_VTP_PORTS-1 : 0] arb_grant;
    logic [N_VTP_PORTS-1 : 0] arb_grant_q;
    logic [N_VTP_PORTS-1 : 0] new_req_rdy;
    logic merged_fifo_almFull;

    // Select a new request if granted arbitration and a request is ready.
    // The test for a request being ready is required because arbitration
    // results are registered and are thus a cycle out of date.
    logic [N_VTP_PORTS-1 : 0] new_req_sel;
    assign new_req_sel = arb_grant_q & new_req_rdy;

    genvar p;
    generate
        for (p = 0; p < N_VTP_PORTS; p = p + 1)
        begin : inp
            cci_mpf_prim_fifo_lutram
              #(
                .N_DATA_BITS($bits(t_cci_mpf_shim_vtp_lookup_req)),
                .N_ENTRIES(CCI_MPF_SHIM_VTP_MAX_SVC_REQS)
                )
              in_fifo
               (
                .clk,
                .reset,

                .enq_data(vtp_svc[p].lookupReq),
                .enq_en(vtp_svc[p].lookupEn),
                .notFull(vtp_svc[p].lookupRdy),

                .first(new_req[p]),
                .deq_en(new_req_sel[p]),
                .notEmpty(new_req_rdy[p]),
                .almostFull()
                );
        end
    endgenerate


    //
    // Fair arbitration for new requests
    //
    t_cci_mpf_shim_vtp_port_idx arb_grant_idx;
    t_cci_mpf_shim_vtp_port_idx arb_grant_idx_q;

    cci_mpf_prim_arb_rr
      #(
        .NUM_CLIENTS(N_VTP_PORTS)
        )
      arb
       (
        .clk,
        .reset,

        .ena(! merged_fifo_almFull),
        .request(new_req_rdy),
        .grant(arb_grant),
        .grantIdx(arb_grant_idx)
        );

    always_ff @(posedge clk)
    begin
        arb_grant_q <= arb_grant;
        arb_grant_idx_q <= arb_grant_idx;
    end


    //
    // Post-arbitration, unified FIFO
    //
    t_cci_mpf_shim_vtp_lookup_req winner_req;
    logic winner_req_en;
    t_cci_mpf_shim_vtp_port_idx winner_req_port_idx;

    t_cci_mpf_shim_vtp_lookup_req first;
    t_cci_mpf_shim_vtp_port_idx first_port_idx;
    logic first_deq;
    logic first_rdy;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            winner_req_en <= 1'b0;
        end
        else
        begin
            winner_req_en <= new_req_sel[arb_grant_idx_q];
        end

        winner_req <= new_req[arb_grant_idx_q];
        winner_req_port_idx <= arb_grant_idx_q;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_cci_mpf_shim_vtp_lookup_req) +
                     $bits(t_cci_mpf_shim_vtp_port_idx)),
        .N_ENTRIES(4),
        .THRESHOLD(2)
        )
      merged_fifo
       (
        .clk,
        .reset,

        .enq_data({ winner_req, winner_req_port_idx }),
        .enq_en(winner_req_en),
        .notFull(),
        .almostFull(merged_fifo_almFull),

        .first({ first, first_port_idx }),
        .deq_en(first_deq),
        .notEmpty(first_rdy)
        );


    // ====================================================================
    //
    //   Construct the pipeline that searches the TLB and send the
    //   merged request stream through it.
    //
    // ====================================================================

    // Interface to main VTP translation pipeline
    cci_mpf_shim_vtp_svc_if vtp_pipe();

    // Interface to the TLB
    cci_mpf_shim_vtp_tlb_if tlb_if();

    t_cci_mpf_shim_vtp_port_idx rsp_port_idx;

    cci_mpf_svc_vtp_pipe
      #(
        .N_VTP_PORTS(N_VTP_PORTS),
        .DEBUG_MESSAGES(DEBUG_MESSAGES)
        )
      pipe
       (
        .clk,
        .reset,
        .vtp_svc(vtp_pipe),
        .reqPortIdx(first_port_idx),
        .rspPortIdx(rsp_port_idx),
        .tlb_if
        );

    always_comb
    begin
        vtp_pipe.lookupEn = vtp_pipe.lookupRdy && first_rdy;
        vtp_pipe.lookupReq = first;

        first_deq = vtp_pipe.lookupEn;
    end


    // ====================================================================
    //
    //   Route responses to clients.
    //
    // ====================================================================

    generate
        for (p = 0; p < N_VTP_PORTS; p = p + 1)
        begin : rsp
            always_ff @(posedge clk)
            begin
                vtp_svc[p].lookupRspValid <=
                    vtp_pipe.lookupRspValid &&
                    (rsp_port_idx == t_cci_mpf_shim_vtp_port_idx'(p));

                vtp_svc[p].lookupRsp <= vtp_pipe.lookupRsp;
            end
        end
    endgenerate


    // ====================================================================
    //
    //  TLB: the VTP pipe was handed a TLB interface.  Construct the
    //  TLB here.
    //
    // ====================================================================

    //
    // Allocate two TLBs.  One manages 4KB pages and the other manages
    // 2MB pages.
    //

    cci_mpf_shim_vtp_tlb_if tlb_if_4kb();

    cci_mpf_svc_vtp_tlb
      #(
        .CCI_PT_PAGE_OFFSET_BITS(CCI_PT_4KB_PAGE_OFFSET_BITS),
        .NUM_TLB_SETS(`VTP_N_TLB_4KB_SETS),
        .NUM_TLB_SET_WAYS(`VTP_N_TLB_4KB_WAYS),
        .DEBUG_MESSAGES(DEBUG_MESSAGES),
        .DEBUG_NAME("4KB")
        )
      tlb4kb
       (
        .clk,
        .reset,
        .tlb_if(tlb_if_4kb),
        .csrs
        );


    cci_mpf_shim_vtp_tlb_if tlb_if_2mb();

    cci_mpf_svc_vtp_tlb
      #(
        .CCI_PT_PAGE_OFFSET_BITS(CCI_PT_2MB_PAGE_OFFSET_BITS),
        .NUM_TLB_SETS(`VTP_N_TLB_2MB_SETS),
        .NUM_TLB_SET_WAYS(`VTP_N_TLB_2MB_WAYS),
        .DEBUG_MESSAGES(DEBUG_MESSAGES),
        .DEBUG_NAME("2MB")
        )
      tlb2mb
       (
        .clk,
        .reset,
        .tlb_if(tlb_if_2mb),
        .csrs
        );

    // When the pipeline requests a TLB lookup do it on both pipelines.
    assign tlb_if_4kb.lookupPageVA = tlb_if.lookupPageVA;
    assign tlb_if_4kb.lookupEn = tlb_if.lookupEn;
    assign tlb_if_2mb.lookupPageVA = tlb_if.lookupPageVA;
    assign tlb_if_2mb.lookupEn = tlb_if.lookupEn;
    assign tlb_if.lookupRdy = tlb_if_4kb.lookupRdy && tlb_if_2mb.lookupRdy;

    // The TLB pipeline is fixed length, so responses arrive together.
    // At most one TLB should have a translation for a given address.
    assign tlb_if.lookupRspValid = tlb_if_4kb.lookupRspValid ||
                                   tlb_if_2mb.lookupRspValid;
    assign tlb_if.lookupRspIsBigPage = tlb_if_2mb.lookupRspValid;
    assign tlb_if.lookupRspPagePA =
        tlb_if_4kb.lookupRspValid ? tlb_if_4kb.lookupRspPagePA :
                                    tlb_if_2mb.lookupRspPagePA;

    // Read the page table if both TLBs miss
    assign tlb_if.lookupMiss = tlb_if_4kb.lookupMiss && tlb_if_2mb.lookupMiss;
    assign tlb_if.lookupMissVA = tlb_if_4kb.lookupMissVA;

    // Validation
    always_ff @(posedge clk)
    begin
        if (! reset)
        begin
            assert(! tlb_if_4kb.lookupRspValid || ! tlb_if_2mb.lookupRspValid) else
                $fatal("cci_mpf_svc_vtp: Both TLBs valid!");

            if (tlb_if.lookupMiss)
            begin
                assert(vtp4kbTo2mbVA(tlb_if_4kb.lookupMissVA) ==
                       vtp4kbTo2mbVA(tlb_if_2mb.lookupMissVA)) else
                    $fatal("cci_mpf_svc_vtp: Both TLBs missed but addresses different!");
            end
        end
    end


    //
    // Direct fills to the appropriate TLB depending on the page size
    //
    logic fill_en_q;

    always_ff @(posedge clk)
    begin
        tlb_if_4kb.fillEn <= tlb_if.fillEn && ! tlb_if.fillBigPage;
        tlb_if_2mb.fillEn <= tlb_if.fillEn && tlb_if.fillBigPage;
        fill_en_q <= tlb_if.fillEn;

        tlb_if_4kb.fillVA <= tlb_if.fillVA;
        tlb_if_4kb.fillPA <= tlb_if.fillPA;
        tlb_if_2mb.fillVA <= tlb_if.fillVA;
        tlb_if_2mb.fillPA <= tlb_if.fillPA;

        tlb_if.fillRdy <= tlb_if_4kb.fillRdy && tlb_if_2mb.fillRdy &&
                          ! tlb_if.fillEn && ! fill_en_q;

        if (reset)
        begin
            tlb_if_4kb.fillEn <= 1'b0;
            tlb_if_2mb.fillEn <= 1'b0;
            tlb_if.fillRdy <= 1'b0;
            fill_en_q <= 1'b0;
        end
    end


    // Statistics
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            events.vtp_out_event_4kb_hit <= 1'b0;
            events.vtp_out_event_2mb_hit <= 1'b0;

            events.vtp_out_event_4kb_miss <= 1'b0;
            events.vtp_out_event_2mb_miss <= 1'b0;
        end
        else
        begin
            events.vtp_out_event_4kb_hit <= tlb_if_4kb.lookupRspValid;
            events.vtp_out_event_2mb_hit <= tlb_if_2mb.lookupRspValid;

            events.vtp_out_event_4kb_miss <= tlb_if_4kb.fillEn;
            events.vtp_out_event_2mb_miss <= tlb_if_2mb.fillEn;
        end
    end


    // ====================================================================
    //
    //   Page walker.
    //
    // ====================================================================

    //
    // Both pt_walk_client and pt_walk_walker passed in to this module are
    // interfaces to the same bus.  pt_walk_client is used here to generate
    // requests to the page table walker.  pt_walk_walker is passed to
    // the walker itself and combines both the pt_walk_client request bus and
    // a bus for reading the page table from host memory.
    //

    // Add a register stage to walk requests for travel across the FPGA
    logic pt_walk_req_en;
    t_tlb_4kb_va_page_idx pt_walk_req_va;

    always_ff @(posedge clk)
    begin
        //
        // In addition to the page walker being ready we also require
        // that the TLB be ready to fill. This is done solely to handle
        // a corner case in which the TLB is processing a fill to the
        // same address that is signalling a miss. Processing the fill
        // would be technically correct but wasteful, since the
        // translation will be added to the TLB within a few cycles.
        //
        // If the page table walker or TLB isn't ready the lookup request
        // is simply dropped.  It will be reissued by the translation
        // pipeline.
        //
        pt_walk_req_en <= tlb_if.lookupMiss && pt_walk_client.reqRdy &&
                          tlb_if.fillRdy;

        pt_walk_req_va <= tlb_if.lookupMissVA;

        pt_walk_client.reqEn <= pt_walk_req_en;
        pt_walk_client.reqVA <= pt_walk_req_va;

        if (reset)
        begin
            pt_walk_client.reqEn <= 1'b0;
            pt_walk_req_en <= 1'b0;
        end
    end

    always_ff @(posedge clk)
    begin
        if (! reset && pt_walk_client.reqEn && DEBUG_MESSAGES)
        begin
            $display("VTP: Request page walk VA 0x%x",
                     {pt_walk_client.reqVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0)});
        end
    end

    always_ff @(posedge clk)
    begin
        events.vtp_out_event_failed_translation <= pt_walk_client.notPresent;

        if (! reset)
        begin
            assert (! pt_walk_client.notPresent) else
                $fatal("cci_mpf_svc_vtp: VA not present in page table");
        end
    end


    cci_mpf_svc_vtp_pt_walk
      #(
        .DEBUG_MESSAGES(DEBUG_MESSAGES)
        )
      walker
       (
        .clk,
        .reset,

        .pt_walk(pt_walk_walker),
        .csrs,
        .tlb_fill_if(tlb_if),

        .statBusy(events.vtp_out_event_pt_walk_busy),
        .statLastTranslateVA(events.vtp_out_pt_walk_last_vaddr)
        );

endmodule // cci_mpf_svc_vtp
