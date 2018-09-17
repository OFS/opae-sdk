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


//
// Pseudo LRU replacement implementation.  Pseudo LRU sets a bit vector to 1
// when an entry is referenced.  When all bits in a vector become 1 all bits
// are reset to 0.
//
// This implementation has more query and update ports than there are
// read and write ports in the internal memory.  Instead of blocking,
// the code makes a best effort to update the vector when requested.
// Lookup requests are always processed correctly.  The assumption is
// there are few lookups compared to updates and that sampling the
// updates instead of recording each one is sufficient.  A fully accurate
// implementation would require one block RAM for each way.  This
// version requires only one block RAM.
//

module cci_mpf_prim_repl_lru_pseudo
  #(
    parameter N_WAYS = 4,
    parameter N_ENTRIES = 1024
    )
   (
    input  logic clk,
    input  logic reset,

    // The entire module is ready after initialization.  Once ready, the
    // lookup function is always available.  The reference functions appear
    // always ready but make no guarantees that all references will be
    // recorded.  Contention on internal memory may require that some
    // references be ignored.
    output logic rdy,

    // Look up LRU for index
    input  logic [$clog2(N_ENTRIES)-1 : 0] lookupIdx,
    input  logic lookupEn,

    // Returns LRU one cycle after lookupEn.  The response is returned
    // in two forms with the same answer.  One is a one-hot vector.  The
    // other is an index.
    output logic [N_WAYS-1 : 0] lookupVecRsp,
    output logic [$clog2(N_WAYS)-1 : 0] lookupRsp,
    output logic lookupRspRdy,

    // Port 0 update.
    input  logic [$clog2(N_ENTRIES)-1 : 0] refIdx0,
    // Update refWayVec will be ORed into the current state
    input  logic [N_WAYS-1 : 0] refWayVec0,
    input  logic refEn0,

    // Port 1 update
    input  logic [$clog2(N_ENTRIES)-1 : 0] refIdx1,
    input  logic [N_WAYS-1 : 0] refWayVec1,
    input  logic refEn1
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_entry_idx;
    typedef logic [N_WAYS-1 : 0] t_way_vec;

    //
    // Pseudo-LRU update function.
    //
    function automatic t_way_vec updatePseudoLRU;
        input t_way_vec cur;
        input t_way_vec newLRU;

        // Or new LRU position into the current state
        t_way_vec upd = cur | newLRU;

        // If all bits set turn everything off, otherwise return the updated
        // set.
        return (&(upd) == 1) ? t_way_vec'(0) : upd;
    endfunction


    //
    // Register inputs for timing.  Delaying LRU update a cycle doesn't
    // really matter.
    //
    t_entry_idx refIdx0_q;
    t_way_vec refWayVec0_q;
    logic refEn0_q;

    t_entry_idx refIdx1_q;
    t_way_vec refWayVec1_q;
    logic refEn1_q;

    always_ff @(posedge clk)
    begin
        refIdx0_q <= refIdx0;
        refWayVec0_q <= refWayVec0;
        refEn0_q <= refEn0;

        refIdx1_q <= refIdx1;
        refWayVec1_q <= refWayVec1;
        refEn1_q <= refEn1;
    end


    // ====================================================================
    //
    //   Storage
    //
    // ====================================================================

    t_entry_idx addr0;
    t_way_vec wdata0;
    logic wen0;
    t_way_vec rdata0;

    t_entry_idx addr1;
    t_way_vec wdata1;
    logic wen1;
    t_way_vec rdata1;

    cci_mpf_prim_ram_dualport_init
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS($bits(t_way_vec))
        )
      lru_data
        (
         .reset,
         .rdy,

         .clk0(clk),
         .addr0,
         .wen0,
         .wdata0,
         .rdata0,
         .clk1(clk),
         .addr1,
         .wen1,
         .wdata1,
         .rdata1
         );


    // ====================================================================
    //
    //   LRU storage port 0 handles this module's port 0 updates and
    //   initialization.
    //
    // ====================================================================

    //
    // Update is a multi-cycle read/modify/write operation. An extra cycle
    // in the middle breaks the block RAM read response -> block RAM write
    // into separate cycles for timing.
    //
    logic update0_en;
    logic update0_en_q;
    logic update0_en_qq;

    t_entry_idx update0_idx;
    t_entry_idx update0_idx_q;
    t_entry_idx update0_idx_qq;

    // New reference
    t_way_vec update0_ref;
    t_way_vec update0_ref_q;
    t_way_vec update0_ref_qq;

    // Current state into which new reference is added
    t_way_vec update0_cur_lru_qq;

    //
    // Address and write data assignment.
    //
    always_comb
    begin
        // Default -- read for update if requested
        addr0 = refIdx0_q;
        wdata0 = 'x;
        wen0 = 1'b0;

        update0_en  = refEn0_q && rdy;
        update0_idx = refIdx0_q;
        update0_ref = refWayVec0_q;

        if (update0_en_qq)
        begin
            // Updating
            addr0 = update0_idx_qq;
            wdata0 = updatePseudoLRU(update0_cur_lru_qq, update0_ref_qq);
            wen0 = 1'b1;

            // No new update can start since can't read for update
            update0_en = 1'b0;
        end
    end

    // Read/modify/write update pipeline
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            update0_en_q <= 0;
            update0_en_qq <= 0;
        end
        else
        begin
            update0_en_q <= update0_en;
            update0_idx_q <= update0_idx;
            update0_ref_q <= update0_ref;

            // Register read response to avoid block RAM read response ->
            // block RAM write in a single cycle.
            update0_en_qq <= update0_en_q;
            update0_idx_qq <= update0_idx_q;
            update0_ref_qq <= update0_ref_q;
            update0_cur_lru_qq <= rdata0;
        end 
    end


    // ====================================================================
    //
    //   LRU storage port 1 handles this module's port 1 updates and
    //   LRU lookup requests.
    //
    // ====================================================================

    logic update1_en;
    logic update1_en_q;
    logic update1_en_qq;

    t_entry_idx update1_idx;
    t_entry_idx update1_idx_q;
    t_entry_idx update1_idx_qq;

    // New reference
    t_way_vec update1_ref;
    t_way_vec update1_ref_q;
    t_way_vec update1_ref_qq;

    // Current state into which new reference is added.  Also holds the
    // buffered state of table reads for LRU lookup.
    t_way_vec update1_cur_lru_qq;

    // Compute the lookup response
    t_way_vec lookup_vec_rsp;
    t_entry_idx lookup_rsp;

    always_comb
    begin
        lookup_vec_rsp = t_way_vec'(0);
        lookup_rsp = 'x;

        for (int w = 0; w < N_WAYS; w = w + 1)
        begin
            if (update1_cur_lru_qq[w] == 0)
            begin
                // One hot vector response
                lookup_vec_rsp[w] = 1;
                // Index response
                lookup_rsp = w;
                break;
            end
        end
    end

    //
    // Delay from lookup request to response is 3 cycles.  The first reads
    // the table, the second registers the table read response, the third
    // registers the LRU computation.
    //
    logic lookup_en_q;
    logic lookup_en_qq;
    always_ff @(posedge clk)
    begin
        lookup_en_q <= lookupEn;
        lookup_en_qq <= lookup_en_q;

        lookupRspRdy <= lookup_en_qq;
        lookupRsp <= lookup_rsp;
        lookupVecRsp <= lookup_vec_rsp;
    end


    // Lookup request has priority of writes
    assign wen1 = update1_en_qq && ! lookupEn &&
                  // Don't request writes to the same line in both ports
                  (! wen0 || (addr0 != addr1));

    //
    // Address and write data assignment.
    //
    always_comb
    begin
        // Default -- read for update if requested
        addr1 = refIdx1_q;
        wdata1 = 'x;

        update1_en  = refEn1_q && rdy;
        update1_idx = refIdx1_q;
        update1_ref = refWayVec1_q;

        if (lookupEn)
        begin
            // Lookup
            addr1 = lookupIdx;
            wdata1 = 'x;

            // No new update can start since can't read for update
            update1_en = 1'b0;
        end
        else if (update1_en_qq)
        begin
            // Ready to write for update
            addr1 = update1_idx_qq;
            wdata1 = updatePseudoLRU(update1_cur_lru_qq, update1_ref_qq);

            // No new update can start since can't read for update
            update1_en = 1'b0;
        end
    end

    //
    // Shift update state as updates progress (read, modify, write)
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            update1_en_q <= 0;
            update1_en_qq <= 0;
        end
        else
        begin
            // Register read for update requests made this cycle
            update1_en_q <= update1_en;
            update1_idx_q <= update1_idx;
            update1_ref_q <= update1_ref;

            // Register read response to avoid block RAM read response ->
            // block RAM write in a single cycle.
            update1_en_qq <= update1_en_q;
            update1_idx_qq <= update1_idx_q;
            update1_ref_qq <= update1_ref_q;
            update1_cur_lru_qq <= rdata1;
        end 
    end

endmodule // cci_mpf_prim_repl_lru_pseudo

