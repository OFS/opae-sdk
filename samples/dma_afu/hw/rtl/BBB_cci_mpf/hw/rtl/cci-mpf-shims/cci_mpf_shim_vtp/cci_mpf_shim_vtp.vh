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

`ifndef __CCI_MPF_SHIM_VTP_VH__
`define __CCI_MPF_SHIM_VTP_VH__


// ************************************************************************
// **                                                                    **
// **    All bit widths are for LINE addresses, not byte addresses!      **
// **                                                                    **
// ************************************************************************

// Width of a virtual address (line addresses).  This value must match
// the width of the CCI request header defined in the base CCI structures
// and the page table data structure.
localparam CCI_PT_VA_BITS = CCI_MPF_CLADDR_WIDTH;

// Width of a physical address (line addresses).  Old machines supported
// only 32 bits.  Recent machines support larger spaces.
localparam CCI_PT_PA_BITS =
    ((MPF_PLATFORM == "OME" || MPF_PLATFORM == "BDX") ?
        32 /* Old, small memory machine */ :
        CCI_MPF_CLADDR_WIDTH);


//
// Components of an address:
//

// Low order (page offset) bits in both VA and PA of a page
localparam CCI_PT_2MB_PAGE_OFFSET_BITS = 15;   // Line sized data (21-6)
localparam CCI_PT_4KB_PAGE_OFFSET_BITS = 6;    //                 (12-6)

// High order (page index) bits in 2MB and 4KB spaces
localparam CCI_PT_2MB_VA_PAGE_INDEX_BITS = CCI_PT_VA_BITS - CCI_PT_2MB_PAGE_OFFSET_BITS;
localparam CCI_PT_4KB_VA_PAGE_INDEX_BITS = CCI_PT_VA_BITS - CCI_PT_4KB_PAGE_OFFSET_BITS;
localparam CCI_PT_2MB_PA_PAGE_INDEX_BITS = CCI_PT_PA_BITS - CCI_PT_2MB_PAGE_OFFSET_BITS;
localparam CCI_PT_4KB_PA_PAGE_INDEX_BITS = CCI_PT_PA_BITS - CCI_PT_4KB_PAGE_OFFSET_BITS;


// Address of a virtual page without the page offset bits
typedef logic [CCI_PT_2MB_VA_PAGE_INDEX_BITS-1 : 0] t_tlb_2mb_va_page_idx;
typedef logic [CCI_PT_4KB_VA_PAGE_INDEX_BITS-1 : 0] t_tlb_4kb_va_page_idx;

// Address of a physical page without the page offset bits
typedef logic [CCI_PT_2MB_PA_PAGE_INDEX_BITS-1 : 0] t_tlb_2mb_pa_page_idx;
typedef logic [CCI_PT_4KB_PA_PAGE_INDEX_BITS-1 : 0] t_tlb_4kb_pa_page_idx;

// Offset within a page
typedef logic [CCI_PT_2MB_PAGE_OFFSET_BITS-1 : 0] t_tlb_2mb_page_offset;
typedef logic [CCI_PT_4KB_PAGE_OFFSET_BITS-1 : 0] t_tlb_4kb_page_offset;


//
// Convert a full address to page index and offset.
//
function automatic t_tlb_2mb_va_page_idx vtp2mbPageIdxFromVA(t_cci_clAddr va);
    return va[CCI_PT_2MB_PAGE_OFFSET_BITS +: CCI_PT_2MB_VA_PAGE_INDEX_BITS];
endfunction

function automatic t_tlb_4kb_va_page_idx vtp4kbPageIdxFromVA(t_cci_clAddr va);
    return va[CCI_PT_4KB_PAGE_OFFSET_BITS +: CCI_PT_4KB_VA_PAGE_INDEX_BITS];
endfunction

function automatic t_tlb_2mb_page_offset vtp2mbPageOffsetFromVA(t_cci_clAddr va);
    return va[CCI_PT_2MB_PAGE_OFFSET_BITS-1 : 0];
endfunction

function automatic t_tlb_4kb_page_offset vtp4kbPageOffsetFromVA(t_cci_clAddr va);
    return va[CCI_PT_4KB_PAGE_OFFSET_BITS-1 : 0];
endfunction


function automatic t_tlb_4kb_pa_page_idx vtp4kbPageIdxFromPA(t_cci_clAddr pa);
    return pa[CCI_PT_4KB_PAGE_OFFSET_BITS +: CCI_PT_4KB_PA_PAGE_INDEX_BITS];
endfunction

function automatic t_tlb_4kb_page_offset vtp4kbPageOffsetFromPA(t_cci_clAddr pa);
    return pa[CCI_PT_4KB_PAGE_OFFSET_BITS-1 : 0];
endfunction


//
// Conversion functions between 4KB and 2MB page indices
//
function automatic t_tlb_2mb_va_page_idx vtp4kbTo2mbVA(t_tlb_4kb_va_page_idx p);
    return p[9 +: CCI_PT_2MB_VA_PAGE_INDEX_BITS];
endfunction

function automatic t_tlb_4kb_va_page_idx vtp2mbTo4kbVA(t_tlb_2mb_va_page_idx p);
    return {p, 9'b0};
endfunction

function automatic t_tlb_4kb_va_page_idx vtp2mbTo4kbVAx(t_tlb_2mb_va_page_idx p);
    return {p, 9'bx};
endfunction

function automatic t_tlb_2mb_pa_page_idx vtp4kbTo2mbPA(t_tlb_4kb_pa_page_idx p);
    return p[9 +: CCI_PT_2MB_PA_PAGE_INDEX_BITS];
endfunction

function automatic t_tlb_4kb_pa_page_idx vtp2mbTo4kbPA(t_tlb_2mb_pa_page_idx p);
    return {p, 9'b0};
endfunction

function automatic t_tlb_4kb_pa_page_idx vtp2mbTo4kbPAx(t_tlb_2mb_pa_page_idx p);
    return {p, 9'bx};
endfunction


// ========================================================================
//
// Interface from a VTP shim to the VTP translation service.  A single
// service instance is shared by all VTP shims, even when multiple VTP
// pipeline shims are allocated.
//
// ========================================================================

// Multiple translation requests may be outstanding and they may be
// returned out of order.  A tag matches responses to requests.
localparam CCI_MPF_SHIM_VTP_MAX_SVC_REQS = 16;
typedef logic [$clog2(CCI_MPF_SHIM_VTP_MAX_SVC_REQS)-1 : 0]
    t_cci_mpf_shim_vtp_req_tag;

typedef struct packed
{
    // Virtual page to translate
    t_tlb_4kb_va_page_idx pageVA;

    // Dynamically unique tag for lookup request for associating out of order
    // responses with requests.
    t_cci_mpf_shim_vtp_req_tag tag;
}
t_cci_mpf_shim_vtp_lookup_req;

typedef struct packed
{
    // Translated physical address
    t_tlb_4kb_pa_page_idx pagePA;

    // Tag from lookup request
    t_cci_mpf_shim_vtp_req_tag tag;

    // Is translation a big page or just a 4KB page?
    logic isBigPage;
}
t_cci_mpf_shim_vtp_lookup_rsp;


interface cci_mpf_shim_vtp_svc_if;
    // Request translation of 4KB aligned address
    logic lookupEn;         // Enable the request
    t_cci_mpf_shim_vtp_lookup_req lookupReq;
    logic lookupRdy;        // Ready to accept a request?

    // Response with translated address.  The response is latency-
    // insensitive, signaled with lookupRspValid.
    logic lookupRspValid;
    t_cci_mpf_shim_vtp_lookup_rsp lookupRsp;

    modport server
       (
        input  lookupEn,
        input  lookupReq,
        output lookupRdy,

        output lookupRspValid,
        output lookupRsp
        );

    modport client
       (
        output lookupEn,
        output lookupReq,
        input  lookupRdy,

        input  lookupRspValid,
        input  lookupRsp
        );

endinterface // cci_mpf_shim_vtp_svc_if


// ========================================================================
//
// Interface for TLB lookup.
//
//   The interface is always described in terms of 4KB pages, independent
//   of whether the TLB is managing 4KB or 2MB pages.  This normalizes the
//   message passing among the VTP pipeline, a TLB and the page table
//   walker.
//
// ========================================================================

interface cci_mpf_shim_vtp_tlb_if;
    // Look up VA in the table and return the PA or signal a miss two cycles
    // later.  The TLB code operates on aligned pages.  It is up to the caller
    // to compute offsets into pages.
    logic lookupEn;         // Enable the request
    t_tlb_4kb_va_page_idx lookupPageVA;
    logic lookupRdy;        // Ready to accept a request?

    // Signal lookupValid two cycles after lookupEn if the page
    // isn't currently in the FPGA-side translation table.
    logic lookupRspValid;
    // Respond with page's physical address two cycles after lookupEn
    t_tlb_4kb_pa_page_idx lookupRspPagePA;
    logic lookupRspIsBigPage;


    //
    // Signals for handling misses and fills.
    //
    logic lookupMiss;
    t_tlb_4kb_va_page_idx lookupMissVA;

    logic fillEn;
    t_tlb_4kb_va_page_idx fillVA;
    t_tlb_4kb_pa_page_idx fillPA;
    // 2MB page? If 0 then it is a 4KB page.
    logic fillBigPage;
    logic fillRdy;

    modport server
       (
        input  lookupEn,
        input  lookupPageVA,
        output lookupRdy,

        output lookupRspValid,
        output lookupRspPagePA,

        output lookupMiss,
        output lookupMissVA,
        input  fillEn,
        input  fillVA,
        input  fillPA,
        output fillRdy
        );

    modport client
       (
        output lookupEn,
        output lookupPageVA,
        input  lookupRdy,

        // Responses are returned from the TLB in the order translations were
        // requested.  At the end of a lookup exactly one of lookupRspValid
        // and lookupMiss will be set for one cycle.  When looukpRspValid is
        // set the remaining lookupRsp fields also have meaning.
        input  lookupMiss,
        input  lookupRspValid,
        input  lookupRspPagePA,
        input  lookupRspIsBigPage
        );

    // Fill -- add a new translation
    modport fill
       (
        output fillEn,
        output fillVA,
        output fillPA,
        output fillBigPage,
        input  fillRdy
        );

endinterface


// ========================================================================
//
// Page table walker interface.
//
// ========================================================================

interface cci_mpf_shim_vtp_pt_walk_if;
    // Enable PT walk request
    logic reqEn;
    // VA to translate
    t_tlb_4kb_va_page_idx reqVA;
    // Ready to accept a request?
    logic reqRdy;

    // Requested VA is not in the page table.  This is an error!
    logic notPresent;

    //
    // Read the page table from host memory
    //
    logic readEn;
    t_cci_clAddr readAddr;
    logic readRdy;

    logic readDataEn;
    t_cci_clData readData;


    // Page table walker (server) ports
    modport pt_walk
       (
        input  reqEn,
        input  reqVA,
        output reqRdy,

        output notPresent,

        output readEn,
        output readAddr,
        input  readRdy,

        input  readDataEn,
        input  readData
        );

    // Client (TLB) interface
    modport client
       (
        output reqEn,
        output reqVA,
        input  reqRdy,

        input  notPresent
        );

    // Memory read port, used by the walker to read PT entries in host memory
    modport mem_read
       (
        input  readEn,
        input  readAddr,
        output readRdy,

        output readDataEn,
        output readData
        );

endinterface

`endif // __CCI_MPF_SHIM_VTP_VH__
