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
`include "cci_mpf_csrs.vh"

`include "cci_mpf_shim_vtp.vh"
`include "cci_mpf_prim_hash.vh"


//
// Page table walker for handling virtual to physical TLB misses.
//
// The walker receives requests from the TLB when a translation is not present
// in the TLB.
//
// The table being walked is constructed by software.  The format is
// is described in SW/src/cci_mpf_shim_vtp_pt.cpp.
//


// Hierarchical page table is composed of 4KB pages, each with 512
// 64 bit pointers either to the translated PA or to the next page
// in the page table.  Each index is thus 9 bits.
localparam CCI_MPF_PT_PAGE_IDX_WIDTH = 9;
typedef logic [CCI_MPF_PT_PAGE_IDX_WIDTH-1 : 0] t_cci_mpf_pt_page_idx;

// Maximum depth (levels of indirection) in the page table.
localparam CCI_MPF_PT_MAX_DEPTH = 4;
typedef logic [$clog2(CCI_MPF_PT_MAX_DEPTH)-1 : 0] t_cci_mpf_pt_walk_depth;

// Vector of page indices, representing the set of indices used in a
// hierarchical page table walk.
typedef t_cci_mpf_pt_page_idx [CCI_MPF_PT_MAX_DEPTH-1 : 0] t_cci_mpf_pt_page_idx_vec;


//
// Status bits in the low bits of a page table entry.
//
typedef struct packed
{
    // Translation error (no translation found)
    logic error;

    // Terminal entry found (the translation)
    logic terminal;
}
t_cci_mpf_pt_walk_status;

function automatic t_cci_mpf_pt_walk_status cci_mpf_ptWalkWordToStatus(logic [63:0] w);
    t_cci_mpf_pt_walk_status s;

    // The SW initializes entries to ~0.  Check bit 3 as a proxy for
    // the entire entry being invalid.  Bits 0-2 are used as flags.
    s.error = w[3];

    // Bit 0 in the response word indicates a successful translation.
    s.terminal = w[0];

    return s;
endfunction


module cci_mpf_svc_vtp_pt_walk
  #(
    parameter DEBUG_MESSAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // Primary interface
    cci_mpf_shim_vtp_pt_walk_if.pt_walk pt_walk,

    // CSRs
    cci_mpf_csrs.vtp csrs,

    // Completed a page walk.  Tell the TLB about a new translation
    cci_mpf_shim_vtp_tlb_if.fill tlb_fill_if,

    // Statistics
    output logic statBusy,
    output t_cci_clAddr statLastTranslateVA
    );

    initial begin
        // Confirm that the VA size specified in VTP matches CCI.  The CCI
        // version is line addresses, so the units must be converted.
        assert (CCI_MPF_CLADDR_WIDTH + $clog2(CCI_CLDATA_WIDTH >> 3) ==
                48) else
            $fatal("cci_mpf_svc_vtp_pt_walk.sv: VA address size mismatch!");
    end

    // Root address of the page table
    t_tlb_4kb_pa_page_idx page_table_root;
    assign page_table_root = vtp4kbPageIdxFromPA(csrs.vtp_in_page_table_base);

    logic initialized;
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            initialized <= 1'b0;
        end
        else
        begin
            initialized <= csrs.vtp_in_page_table_base_valid &&
                           csrs.vtp_in_mode.enabled;
        end
    end


    // ====================================================================
    //
    //   Page table properties.
    //
    // ====================================================================

    // Page index components: line address and word within line
    localparam PT_WORDS_PER_LINE = CCI_CLDATA_WIDTH / 64;
    localparam PT_LINE_WORD_IDX_WIDTH = $clog2(PT_WORDS_PER_LINE);
    typedef logic [PT_LINE_WORD_IDX_WIDTH-1 : 0] t_pt_line_word_idx;

    localparam PT_PAGE_LINE_IDX_WIDTH = CCI_MPF_PT_PAGE_IDX_WIDTH - PT_LINE_WORD_IDX_WIDTH;
    typedef logic [PT_PAGE_LINE_IDX_WIDTH-1 : 0] t_pt_page_line_idx;

    typedef struct packed
    {
        // Index of a line within a 4KB page table
        t_pt_page_line_idx line_idx;
        // Index of a word within the line
        t_pt_line_word_idx word_idx;
    }
    t_pt_page_idx;

    function automatic t_pt_page_line_idx ptPageLineIdx(
        t_cci_mpf_pt_page_idx_vec pidx_vec
        );

        t_pt_page_idx pidx = pidx_vec[0];
        return pidx.line_idx;
    endfunction

    function automatic t_pt_line_word_idx ptLineWordIdx(
        t_cci_mpf_pt_page_idx_vec pidx_vec
        );

        t_pt_page_idx pidx = pidx_vec[0];
        return pidx.word_idx;
    endfunction


    // ====================================================================
    //
    //   Page walker state machine.
    //
    // ====================================================================

    typedef enum logic [3:0]
    {
        STATE_PT_WALK_IDLE,
        STATE_PT_WALK_READ_CACHE_REQ,
        STATE_PT_WALK_READ_CACHE_RSP,
        STATE_PT_WALK_READ_CACHE_RETRY,
        STATE_PT_WALK_READ_REQ,
        STATE_PT_WALK_READ_WAIT_RSP,
        STATE_PT_WALK_READ_RSP,
        STATE_PT_WALK_DONE,
        STATE_PT_WALK_ERROR,
        STATE_PT_WALK_HALT
    }
    t_state_pt_walk;

    t_state_pt_walk state;

    // Single-bit registers corresponding to states. Using these helps
    // some critical timing paths.
    logic state_is_walk_idle;
    logic state_is_walk_done;

    //
    // The miss handler supports processing only one request at a time.
    //
    assign pt_walk.reqRdy = initialized && state_is_walk_idle;

    assign statBusy = ! state_is_walk_idle;


    // Base address of current page being accessed.  During a walk pt_cur_page
    // points to pages in the page table.  When translation is complete it
    // points to the translated physical page.
    t_tlb_4kb_pa_page_idx pt_walk_cur_page;
    t_tlb_4kb_pa_page_idx pt_walk_next_page;

    t_cci_mpf_pt_walk_status pt_walk_cur_status;

    // Selected word within the response line
    logic [63 : 0] pt_read_rsp_word;

    // VA being translated
    t_tlb_4kb_va_page_idx translate_va;
    assign statLastTranslateVA = { translate_va, CCI_PT_4KB_PAGE_OFFSET_BITS'(0) };

    // During translation the VA is broken down into 9 bit indices during
    // the tree-based page walk.  This register is shifted as each level
    // is traversed, leaving the next index in the high bits.
    t_cci_mpf_pt_page_idx_vec translate_va_idx_vec;
    t_cci_mpf_pt_page_idx_vec translate_va_lower_idx_vec;
    
    // High bits of the requested VA are the page table indices
    t_cci_mpf_pt_page_idx_vec req_va_as_idx_vec;
    assign req_va_as_idx_vec = pt_walk.reqVA[($bits(pt_walk.reqVA)-1) -:
                                             $bits(t_cci_mpf_pt_page_idx_vec)];


    // Track the depth while walking the table.  This is one way of detecting
    // a malformed table or missing entry.
    t_cci_mpf_pt_walk_depth translate_depth;

    //
    // Add a register stage to incoming read responses to relax timing.
    //
    t_cci_clData ptReadData_q;
    logic [63 : 0] ptReadData_qq;    // Line reduced to a word
    logic ptReadDataEn_q;
    logic ptReadDataEn_qq;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            ptReadDataEn_q <= 1'b0;
            ptReadDataEn_qq <= 1'b0;
        end
        else
        begin
            ptReadDataEn_q <= pt_walk.readDataEn;
            ptReadDataEn_qq <= ptReadDataEn_q;
        end

        ptReadData_q <= pt_walk.readData;

        // pt_read_rsp_word is the word needed from ptReadData_q
        ptReadData_qq <= pt_read_rsp_word;
    end

    //
    // Cache of previous page table reads.  We don't rely on the small
    // QPI cache.  Instead, a small cache of recent page table lines is
    // maintained.
    //
    logic ptReadCacheRdy;
    logic ptReadCacheMissRsp;
    logic ptReadCacheHitRsp;
    t_tlb_4kb_pa_page_idx ptReadCachePage;
    t_cci_mpf_pt_walk_status ptReadCacheStatus;

    t_tlb_4kb_pa_page_idx pt_walk_cache_page;
    logic pt_walk_page_from_cache;

    cci_mpf_svc_vtp_pt_walk_cache
      #(
        .DEBUG_MESSAGES(DEBUG_MESSAGES)
        )
      cache
       (
        .clk,
        .reset,
        .csrs,

        .rdy(ptReadCacheRdy),

        // Request lookup
        .reqEn(ptReadCacheRdy && (state == STATE_PT_WALK_READ_CACHE_REQ)),
        .reqPageIdxVec(translate_va_idx_vec),
        .reqWalkDepth(translate_depth),

        // Lookup response
        .rspMiss(ptReadCacheMissRsp),
        .rspHit(ptReadCacheHitRsp),
        .rspPageAddr(ptReadCachePage),
        .rspStatus(ptReadCacheStatus),

        // Insert a new line in the cache
        .insertEn(ptReadDataEn_q),
        .insertData(ptReadData_q),
        .insertPageIdxVec(translate_va_idx_vec),
        .insertWalkDepth(translate_depth)
        );


    //
    // State transition.  One request is processed at a time.
    //
    always_ff @(posedge clk)
    begin
        case (state)
          STATE_PT_WALK_IDLE:
            begin
                // New request arrived and not already doing a walk
                if (pt_walk.reqEn)
                begin
                    state <= STATE_PT_WALK_READ_CACHE_REQ;
                    state_is_walk_idle <= 1'b0;
                end

                // New request: start by searching the local page table
                // cache (depth first).
                translate_va <= pt_walk.reqVA;
                translate_va_idx_vec <= req_va_as_idx_vec;
                translate_depth <=
                    t_cci_mpf_pt_walk_depth'(CCI_MPF_PT_MAX_DEPTH - 1);

                pt_walk_cur_page <= page_table_root;
            end

          STATE_PT_WALK_READ_CACHE_REQ:
            begin
                // Wait until a PT cache read request can fire
                if (ptReadCacheRdy)
                begin
                    state <= STATE_PT_WALK_READ_CACHE_RSP;
                end
            end

          STATE_PT_WALK_READ_CACHE_RSP:
            begin
                if (ptReadCacheMissRsp)
                begin
                    // Not in cache.
                    if (translate_depth != t_cci_mpf_pt_walk_depth'(0))
                    begin
                        // Try higher up in the page table hierarchy.
                        state <= STATE_PT_WALK_READ_CACHE_RETRY;
                    end
                    else
                    begin
                        // Reached the root of the table without finding
                        // the entry.  Read the from page table instead.
                        state <= STATE_PT_WALK_READ_REQ;
                    end
                end
                else if (ptReadCacheHitRsp)
                begin
                    // Hit!  No need to read from host memory.
                    state <= STATE_PT_WALK_READ_RSP;

                    pt_walk_cur_status <= ptReadCacheStatus;

                    pt_walk_page_from_cache <= 1'b1;
                    pt_walk_cache_page <= ptReadCachePage;
                end
            end

          STATE_PT_WALK_READ_CACHE_RETRY:
            begin
                // Shift to look higher up in the page table hierarchy.
                state <= STATE_PT_WALK_READ_CACHE_REQ;
                translate_depth <= translate_depth -
                                   t_cci_mpf_pt_walk_depth'(1);

                // Shift the page table index vector to represent
                // a higher level.
                for (int i = 0; i < CCI_MPF_PT_MAX_DEPTH-1; i = i + 1)
                begin
                    translate_va_idx_vec[i] <=
                        translate_va_idx_vec[i + 1];
                    translate_va_lower_idx_vec[i] <=
                        translate_va_lower_idx_vec[i + 1];
                end

                translate_va_idx_vec[CCI_MPF_PT_MAX_DEPTH-1] <=
                    t_cci_mpf_pt_page_idx'(0);

                // "Lower" vector holds indices only relevant to
                // the hierarchy below the current search depth.
                translate_va_lower_idx_vec[CCI_MPF_PT_MAX_DEPTH-1] <=
                    translate_va_idx_vec[0];
            end

          STATE_PT_WALK_READ_REQ:
            begin
                // Wait until a PT read request can fire
                if (pt_walk.readEn)
                begin
                    state <= STATE_PT_WALK_READ_WAIT_RSP;
                end
            end

          STATE_PT_WALK_READ_WAIT_RSP:
            begin
                // Wait for PT read response
                if (ptReadDataEn_qq)
                begin
                    state <= STATE_PT_WALK_READ_RSP;

                    pt_walk_cur_status <= cci_mpf_ptWalkWordToStatus(ptReadData_qq);

                    // Extract the address of a line from the entry.
                    pt_walk_page_from_cache <= 1'b0;
                    pt_walk_next_page <=
                        vtp4kbPageIdxFromPA(ptReadData_qq[$clog2(CCI_CLDATA_WIDTH / 8) +:
                                                          CCI_CLADDR_WIDTH]);
                end
            end

          STATE_PT_WALK_READ_RSP:
            begin
                // The update of pt_walk_cur_page could logically have been
                // in earlier states.  Putting the MUX here is better
                // for timing.
                pt_walk_cur_page <= pt_walk_page_from_cache ?
                                    pt_walk_cache_page : pt_walk_next_page;

                if (pt_walk_cur_status.terminal)
                begin
                    // Found the translation
                    state <= STATE_PT_WALK_DONE;
                    state_is_walk_done <= 1'b1;
                end
                else
                begin
                    // Continue the walk
                    state <= STATE_PT_WALK_READ_REQ;
                    translate_depth <= translate_depth + t_cci_mpf_pt_walk_depth'(1);
                end

                // Raise an error if the maximum walk depth is reached without
                // finding the entry.
                if (pt_walk_cur_status.error || 
                    ! pt_walk_cur_status.terminal && (&(translate_depth) == 1'b1))
                begin
                    state <= STATE_PT_WALK_ERROR;
                    state_is_walk_done <= 1'b0;
                end

                // Shift to move to the index of the next level.
                for (int i = 0; i < CCI_MPF_PT_MAX_DEPTH-1; i = i + 1)
                begin
                    translate_va_idx_vec[i + 1] <= translate_va_idx_vec[i];
                    translate_va_lower_idx_vec[i + 1] <= translate_va_lower_idx_vec[i];
                end
                translate_va_idx_vec[0] <= translate_va_lower_idx_vec[CCI_MPF_PT_MAX_DEPTH-1];
            end

          STATE_PT_WALK_DONE:
            begin
                // Current request is complete
                if (tlb_fill_if.fillEn)
                begin
                    state <= STATE_PT_WALK_IDLE;
                    state_is_walk_idle <= 1'b1;
                    state_is_walk_done <= 1'b0;
                end
            end

          STATE_PT_WALK_ERROR:
            begin
                // Terminal state
                pt_walk.notPresent <= 1'b1;
                state <= STATE_PT_WALK_HALT;

                if (! reset)
                begin
                    $fatal("VTP PT WALK: No translation found for VA 0x%x",
                           { translate_va, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0 });
                end
            end

          STATE_PT_WALK_HALT:
            begin
                pt_walk.notPresent <= 1'b0;
            end
        endcase

        if (reset)
        begin
            state <= STATE_PT_WALK_IDLE;
            pt_walk.notPresent <= 1'b0;
            state_is_walk_idle <= 1'b1;
            state_is_walk_done <= 1'b0;
        end
    end


    // ====================================================================
    //
    //   Generate page table read requests.
    //
    // ====================================================================

    // Enable a read request?
    assign pt_walk.readEn = (state == STATE_PT_WALK_READ_REQ) && pt_walk.readRdy;

    // Address of read request
    always_comb
    begin
        pt_walk.readAddr = t_cci_clAddr'(0);

        // Current page in table
        pt_walk.readAddr[CCI_PT_4KB_PAGE_OFFSET_BITS +: CCI_PT_4KB_PA_PAGE_INDEX_BITS] =
            pt_walk_cur_page;

        // Select the proper line in this level of the table, based on the
        // portion of the VA corresponding to the level.
        pt_walk.readAddr[PT_PAGE_LINE_IDX_WIDTH-1 : 0] = ptPageLineIdx(translate_va_idx_vec);
    end


    // ====================================================================
    //
    //   Consume page table read responses.
    //
    // ====================================================================

    // Break a read response line into 64 bit words
    logic [(CCI_CLDATA_WIDTH / 64)-1 : 0][63 : 0] pt_read_rsp_word_vec;

    always_comb
    begin
        pt_read_rsp_word_vec = ptReadData_q;
        pt_read_rsp_word = pt_read_rsp_word_vec[ptLineWordIdx(translate_va_idx_vec)];
    end


    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES)
        begin
            if (pt_walk.reqEn && (state == STATE_PT_WALK_IDLE))
            begin
                $display("VTP PT WALK: New req translate line 0x%x (VA 0x%x)",
                         { pt_walk.reqVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0) },
                         { pt_walk.reqVA, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0 });
            end

            if ((state == STATE_PT_WALK_READ_CACHE_REQ) && ptReadCacheRdy)
            begin
                $display("VTP PT WALK: Cache read [0x%x 0x%x 0x%x 0x%x] depth 0x%x",
                         translate_va_idx_vec[3],
                         translate_va_idx_vec[2],
                         translate_va_idx_vec[1],
                         translate_va_idx_vec[0],
                         translate_depth);
            end

            if ((state == STATE_PT_WALK_READ_CACHE_RSP) && ptReadCacheMissRsp)
            begin
                $display("VTP PT WALK: Cache miss");
            end

            if ((state == STATE_PT_WALK_READ_CACHE_RSP) && ptReadCacheHitRsp)
            begin
                $display("VTP PT WALK: Cache hit PA 0x%x (terminal %0d, error %0d)",
                         {ptReadCachePage, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0},
                         ptReadCacheStatus.terminal,
                         ptReadCacheStatus.error);
            end

            if (pt_walk.readEn)
            begin
                $display("VTP PT WALK: PTE read addr 0x%x (PA 0x%x) (line 0x%x, word 0x%x)",
                         pt_walk.readAddr, {pt_walk.readAddr, 6'b0},
                         ptPageLineIdx(translate_va_idx_vec),
                         ptLineWordIdx(translate_va_idx_vec));
            end

            if (ptReadDataEn_q)
            begin
                $display("VTP PT WALK: Line arrived 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                         pt_read_rsp_word_vec[7],
                         pt_read_rsp_word_vec[6],
                         pt_read_rsp_word_vec[5],
                         pt_read_rsp_word_vec[4],
                         pt_read_rsp_word_vec[3],
                         pt_read_rsp_word_vec[2],
                         pt_read_rsp_word_vec[1],
                         pt_read_rsp_word_vec[0]);

                $display("VTP PT WALK: Cache insert [0x%x 0x%x 0x%x 0x%x] depth 0x%x",
                         translate_va_idx_vec[3],
                         translate_va_idx_vec[2],
                         translate_va_idx_vec[1],
                         translate_va_idx_vec[0],
                         translate_depth);
            end

            if (tlb_fill_if.fillEn)
            begin
                $display("VTP PT WALK: Response PA 0x%x, size %s",
                         {pt_walk_cur_page, CCI_PT_4KB_PAGE_OFFSET_BITS'(0), 6'b0},
                         (tlb_fill_if.fillBigPage ? "2MB" : "4KB"));
            end
        end
    end


    // ====================================================================
    //
    //   Return page walk result.
    //
    // ====================================================================

    //
    // TLB insertion (in STATE_PT_WALK_INSERT)
    //
    assign tlb_fill_if.fillEn = state_is_walk_done && tlb_fill_if.fillRdy;
    assign tlb_fill_if.fillVA = translate_va;
    assign tlb_fill_if.fillPA = pt_walk_cur_page;

    // Use just bit 0 of translate_depth, which is either 2 for a 2MB page
    // or 3 for a 4KB page.
    assign tlb_fill_if.fillBigPage = ! (translate_depth[0]);

endmodule // cci_mpf_svc_vtp_pt_walk


//
// Small cache of previously read lines in the page table.
//
module cci_mpf_svc_vtp_pt_walk_cache
  #(
    parameter DEBUG_MESSAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // CSRs
    cci_mpf_csrs.vtp csrs,

    // Ready for new request?
    output logic rdy,

    // Look up line/word
    input  logic reqEn,
    input  t_cci_mpf_pt_page_idx_vec reqPageIdxVec,
    input  t_cci_mpf_pt_walk_depth reqWalkDepth,

    // Response
    output logic rspMiss,
    output logic rspHit,
    output t_tlb_4kb_pa_page_idx rspPageAddr,
    output t_cci_mpf_pt_walk_status rspStatus,

    // Insert data in the cache
    input  logic insertEn,
    input  t_cci_clData insertData,
    input  t_cci_mpf_pt_page_idx_vec insertPageIdxVec,
    input  t_cci_mpf_pt_walk_depth insertWalkDepth
    );

    // Number of cache entries.  Each entry has one tag and
    // PT_WORDS_PER_LINE words.
    localparam PT_CACHE_ENTRIES = 128;
    typedef logic [$clog2(PT_CACHE_ENTRIES)-1 : 0] t_pt_cache_idx;

    //
    // Break the page index vector into two components:
    //   - The majority is a tag.
    //   - The low few bits are the index of a word within a CCI line.
    //     This is what makes the cache useful, since multiple
    //     page table entries are fetched with each line.
    //
    localparam PT_WORDS_PER_LINE = CCI_CLDATA_WIDTH / 64;
    localparam PT_LINE_WORD_IDX_WIDTH = $clog2(PT_WORDS_PER_LINE);
    typedef logic [PT_LINE_WORD_IDX_WIDTH-1 : 0] t_pt_line_word_idx;

    // t_cci_mpf_pt_page_idx_vec without the low t_pt_line_word_idx bits
    typedef logic [$bits(t_cci_mpf_pt_page_idx_vec) - PT_LINE_WORD_IDX_WIDTH - 1 : 0]
        t_pt_entry_tag;

    //
    // Cache tag from index vector.  The vector represents the offsets within
    // each page table, so the set of indices uniquely identifies a page
    // table entry.  The tag represents a full line so it ignores the low
    // word index bits.
    //
    function automatic t_pt_entry_tag cacheTag(t_cci_mpf_pt_page_idx_vec idxVec);
        // Ignore the low (word index) bits of the page index vector
        t_pt_entry_tag tag;
        t_pt_line_word_idx w_idx;
        {tag, w_idx} = idxVec;

        return tag;
    endfunction

    //
    // Compute the cache index given a page index vector and the depth
    // in the page table walk.  Each depth gets its own region in the
    // address space.
    //
    function automatic t_pt_cache_idx cacheIdx(t_cci_mpf_pt_page_idx_vec idxVec,
                                               t_cci_mpf_pt_walk_depth depth);
        // Hash the tag and include the depth
        return t_pt_cache_idx'({ hash32(32'(cacheTag(idxVec))), depth });
    endfunction


    logic tag_rdy;
    logic insert_busy;
    assign rdy = tag_rdy && ! insert_busy;


    // ====================================================================
    //
    //  Storage
    //
    // ====================================================================

    //
    // Tag memory
    //
    logic ins_tag_en;
    t_pt_cache_idx ins_idx;
    t_pt_entry_tag ins_tag;

    t_pt_cache_idx lookup_idx;
    assign lookup_idx = cacheIdx(reqPageIdxVec, reqWalkDepth);
    t_pt_entry_tag lookup_tag;
    logic lookup_tag_valid;

    logic n_reset_tlb[0:1];
    always @(posedge clk)
    begin
        n_reset_tlb[1] <= ~csrs.vtp_in_mode.inval_translation_cache &&
                          // Reset the page table cache when any page is
                          // invalidated.  The cost isn't that high and the
                          // protocol for invalidating a single entry is
                          // complicated.
                          ~csrs.vtp_in_inval_page_valid;

        n_reset_tlb[0] <= n_reset_tlb[1];

        if (reset)
        begin
            n_reset_tlb[1] <= 1'b0;
            n_reset_tlb[0] <= 1'b0;
        end
    end

    always_ff @(posedge clk)
    begin
        if (! reset && DEBUG_MESSAGES)
        begin
            if (~n_reset_tlb[0])
            begin
                $display("VTP PT WALK: Invalidate PT cache");
            end
        end
    end

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(PT_CACHE_ENTRIES),
        .N_DATA_BITS(1 + $bits(t_pt_entry_tag)),
        .INIT_VALUE({ 1'b0, t_pt_entry_tag'('x) }),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0),
        .N_OUTPUT_REG_STAGES(1)
        )
      tag
       (
        .clk,
        .reset(~n_reset_tlb[0]),
        .rdy(tag_rdy),

        .waddr(ins_idx),
        .wen(ins_tag_en),
        .wdata({ 1'b1, ins_tag }),

        .raddr(lookup_idx),
        .rdata({ lookup_tag_valid, lookup_tag })
        );

    //
    // Data memory
    //
    logic ins_data_en;
    t_pt_line_word_idx ins_word_idx;
    t_tlb_4kb_pa_page_idx ins_data_word;
    t_cci_mpf_pt_walk_status ins_data_status;

    t_tlb_4kb_pa_page_idx rsp_page_addr;
    t_cci_mpf_pt_walk_status rsp_status;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(PT_CACHE_ENTRIES * PT_WORDS_PER_LINE),
        .N_DATA_BITS(CCI_PT_4KB_PA_PAGE_INDEX_BITS +
                     $bits(t_cci_mpf_pt_walk_status)),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0),
        .N_OUTPUT_REG_STAGES(1)
        )
      data
       (
        .clk,

        .waddr({ ins_idx, ins_word_idx }),
        .wen(ins_data_en),
        .wdata({ ins_data_word, ins_data_status }),

        .raddr({ lookup_idx, t_pt_line_word_idx'(reqPageIdxVec) }),
        .rdata({ rsp_page_addr, rsp_status })
        );


    // ====================================================================
    //
    //  Lookup pipeline
    //
    // ====================================================================

    logic lookup_q;
    logic lookup_qq;
    t_pt_entry_tag lookup_tgt_tag_q;
    t_pt_entry_tag lookup_tgt_tag_qq;

    logic lookup_hit;
    assign lookup_hit = lookup_tag_valid &&
                        (lookup_tgt_tag_qq == lookup_tag) &&
                        ! rsp_status.error;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            lookup_q <= 1'b0;
            lookup_qq <= 1'b0;

            rspMiss <= 1'b0;
            rspHit <= 1'b0;
        end
        else
        begin
            lookup_q <= reqEn;
            lookup_qq <= lookup_q;

            rspMiss <= lookup_qq && ! lookup_hit;
            rspHit <= lookup_qq && lookup_hit;
        end

        lookup_tgt_tag_q <= cacheTag(reqPageIdxVec);
        lookup_tgt_tag_qq <= lookup_tgt_tag_q;

        rspPageAddr <= rsp_page_addr;
        rspStatus <= rsp_status;
    end


    // ====================================================================
    //
    //  Insertion state machine
    //
    // ====================================================================

    //
    // Inserting takes multiple cycles since each word in a line is stored
    // in a separate index.
    //

    // Break a line into 64 bit words
    logic [PT_WORDS_PER_LINE-1 : 0][63 : 0] ins_line_words;

    always_ff @(posedge clk)
    begin
        if (insertEn)
        begin
            // New line to insert.  The rdy bit guarantees no insert is
            // happening when insertEn is triggered.
            insert_busy <= 1'b1;

            ins_idx <= cacheIdx(insertPageIdxVec, insertWalkDepth);
            ins_tag <= cacheTag(insertPageIdxVec);
            ins_line_words <= insertData;
        end
        else if (insert_busy)
        begin
            // Last word?
            if (ins_tag_en)
            begin
                insert_busy <= 1'b0;
            end

            ins_word_idx <= ins_word_idx + t_pt_line_word_idx'(1);

            // Shift line as words are written to the memory
            for (int w = 0; w < PT_WORDS_PER_LINE-1; w = w + 1)
            begin
                ins_line_words[w] <= ins_line_words[w + 1];
            end
        end

        if (reset)
        begin
            insert_busy <= 1'b0;
            ins_word_idx <= t_pt_line_word_idx'(0);
        end
    end

    // Write the tag when the last word in the line is saved to the cache
    assign ins_tag_en = (&(ins_word_idx) == 1'b1);
    assign ins_data_en = insert_busy;

    assign ins_data_word =
        vtp4kbPageIdxFromPA(ins_line_words[0][$clog2(CCI_CLDATA_WIDTH / 8) +:
                                              CCI_CLADDR_WIDTH]);
    assign ins_data_status = cci_mpf_ptWalkWordToStatus(ins_line_words[0]);

endmodule // cci_mpf_svc_vtp_pt_walk_cache
