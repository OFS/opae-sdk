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


//
// Primary virtual to physical address translation pipeline.  The TLB
// is provided as an interface.  The TLB may respond that a translation
// is invalid while simultaneously sending a message to the page table
// to look up the translation.  The pipeline here continues to request
// translation, even for requests that fail, until the TLB is capable
// of completing the request.
//

module cci_mpf_svc_vtp_pipe
  #(
    parameter N_VTP_PORTS = 0,
    parameter DEBUG_MESSAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // Requests and responses
    cci_mpf_shim_vtp_svc_if.server vtp_svc,
    input  logic [$clog2(N_VTP_PORTS)-1 : 0] reqPortIdx,
    output logic [$clog2(N_VTP_PORTS)-1 : 0] rspPortIdx,

    // TLB lookup
    cci_mpf_shim_vtp_tlb_if.client tlb_if
    );

    typedef logic [$clog2(N_VTP_PORTS)-1 : 0] t_cci_mpf_shim_vtp_port_idx;


    // ====================================================================
    //
    //  Send new requests through a FIFO for timing and flow control.
    //
    // ====================================================================

    t_cci_mpf_shim_vtp_lookup_req new_req;
    t_cci_mpf_shim_vtp_port_idx new_req_port;
    logic new_req_deq;
    logic new_req_rdy;

    cci_mpf_prim_fifo2
      #(
        .N_DATA_BITS($bits(t_cci_mpf_shim_vtp_lookup_req) +
                     $bits(t_cci_mpf_shim_vtp_port_idx))
        )
      in_fifo
       (
        .clk,
        .reset,

        .enq_data({ vtp_svc.lookupReq, reqPortIdx }),
        .enq_en(vtp_svc.lookupEn),
        .notFull(vtp_svc.lookupRdy),

        .first({ new_req, new_req_port }),
        .deq_en(new_req_deq),
        .notEmpty(new_req_rdy)
        );


    // ====================================================================
    //
    //  Manage active TLB requests.  TLB responses are returned in order
    //  but may indicate the translation failed.  The code here allows
    //  multiple requests to flow through the TLB so that hits can be
    //  returned while the TLB fills from the page table in response
    //  to misses.  Retried misses will eventually succeed.
    //
    // ====================================================================

    t_cci_mpf_shim_vtp_lookup_req lookup_req;
    t_cci_mpf_shim_vtp_port_idx lookup_req_port;
    logic lookup_req_en;
    logic lookup_req_almostFull, lookup_req_notFull;
    assign lookup_req_notFull = ~lookup_req_almostFull;

    t_cci_mpf_shim_vtp_lookup_req lookup_rsp;
    t_cci_mpf_shim_vtp_port_idx lookup_rsp_port;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_cci_mpf_shim_vtp_lookup_req) +
                     $bits(t_cci_mpf_shim_vtp_port_idx)),
        .N_ENTRIES(16),
        // Ensure there is always room to loop the oldest entry back to the
        // FIFO on TLB miss and for the delay in writing registered data.
        .THRESHOLD(3)
        )
      active_reqs
       (
        .clk,
        .reset,

        .enq_data({ lookup_req, lookup_req_port }),
        .enq_en(lookup_req_en),
        .almostFull(lookup_req_almostFull),

        .first({ lookup_rsp, lookup_rsp_port }),
        .deq_en(tlb_if.lookupRspValid || tlb_if.lookupMiss),
        // Ignored since TLB will respond only when there is an entry here
        .notEmpty(),
        .notFull()
        );


    //
    // Route requests to the TLB and to the active_reqs FIFO.
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            lookup_req_en <= 1'b0;
        end
        else
        begin
            // Either retry for TLB miss or new request.
            lookup_req_en <= tlb_if.lookupMiss || new_req_deq;
        end

        // Next lookup is either a retry or a new request
        lookup_req <= tlb_if.lookupMiss ? lookup_rsp : new_req;
        lookup_req_port <= tlb_if.lookupMiss ? lookup_rsp_port : new_req_port;
    end

    always_comb
    begin
        new_req_deq = new_req_rdy && lookup_req_notFull && tlb_if.lookupRdy &&
                      ! tlb_if.lookupMiss;

        // Generate TLB lookup
        tlb_if.lookupEn = lookup_req_en;
        tlb_if.lookupPageVA = lookup_req.pageVA;
    end

    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES)
        begin
            if (new_req_deq)
            begin
                $display("VTP PIPE: Lookup new port %0d, tag %0d, VA 0x%x",
                         new_req_port,
                         new_req.tag,
                         {new_req.pageVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0});
            end

            if (tlb_if.lookupMiss)
            begin
                $display("VTP PIPE: Lookup retry port %0d, tag %0d, VA 0x%x",
                         lookup_rsp_port,
                         lookup_rsp.tag,
                         {lookup_rsp.pageVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0});
            end
        end
    end


    // ====================================================================
    //
    //  Successful response.
    //
    // ====================================================================

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            vtp_svc.lookupRspValid <= 1'b0;
        end
        else
        begin
            vtp_svc.lookupRspValid <= tlb_if.lookupRspValid;
        end

        vtp_svc.lookupRsp.pagePA <= tlb_if.lookupRspPagePA;
        vtp_svc.lookupRsp.tag <= lookup_rsp.tag;
        vtp_svc.lookupRsp.isBigPage <= tlb_if.lookupRspIsBigPage;

        rspPortIdx <= lookup_rsp_port;
    end

    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES)
        begin
            if (vtp_svc.lookupRspValid)
            begin
                $display("VTP PIPE: Lookup response port %0d, tag %0d, PA 0x%x (%s)",
                         rspPortIdx,
                         vtp_svc.lookupRsp.tag,
                         {vtp_svc.lookupRsp.pagePA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0},
                         (vtp_svc.lookupRsp.isBigPage ? "2MB" : "4KB"));
            end
        end
    end

endmodule // cci_mpf_svc_vtp_pipe
