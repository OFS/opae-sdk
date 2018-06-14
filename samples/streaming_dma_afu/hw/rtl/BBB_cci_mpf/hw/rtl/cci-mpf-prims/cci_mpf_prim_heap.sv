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
// cci_mpf_prim_heap_ctrl manage entry allocation and not data.
// cci_mpf_prim_heap is a convenience wrapper that manages both.
//

module cci_mpf_prim_heap
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1,
    // Register enq, delaying the write by a cycle?
    parameter REGISTER_INPUT = 0,
    // Number of additional register stages on readRsp
    parameter N_OUTPUT_REG_STAGES = 0
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic enq,                                // Allocate an entry
    input  logic [N_DATA_BITS-1 : 0] enqData,
    output logic notFull,                            // Is scoreboard full?
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    input  logic [$clog2(N_ENTRIES)-1 : 0] readReq,  // Read requested index
    output logic [N_DATA_BITS-1 : 0] readRsp,        // Read data (cycle after req)

    input  logic free,                               // enable free freeIdx
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    typedef logic [N_DATA_BITS-1 : 0] t_data;
    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;


    // ====================================================================
    //
    // Control logic.
    //
    // ====================================================================

    cci_mpf_prim_heap_ctrl
      #(
        .N_ENTRIES(N_ENTRIES),
        .MIN_FREE_SLOTS(MIN_FREE_SLOTS)
        )
      ctrl
       (
        .clk,
        .reset,
        .enq,
        .notFull,
        .allocIdx,
        .free,
        .freeIdx
        );


    // ====================================================================
    //
    // Heap writes either happen the cycle requested (default) or are
    // registered for timing and delayed a cycle. The allocation pointers
    // and counters are updated the cycle requested independent of
    // the data timing.
    //
    // ====================================================================

    logic heap_enq;
    t_data heap_enqData;
    t_idx heap_allocIdx;

    generate
        if (REGISTER_INPUT == 0)
        begin : nr
            // Unregistered
            assign heap_enq = enq;
            assign heap_enqData = enqData;
            assign heap_allocIdx = allocIdx;
        end
        else
        begin : r
            // Registered
            always_ff @(posedge clk)
            begin
                heap_enq <= enq;
                heap_enqData <= enqData;
                heap_allocIdx <= allocIdx;
            end
        end
    endgenerate


    //
    // Heap memory
    //
    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .N_OUTPUT_REG_STAGES(N_OUTPUT_REG_STAGES)
        )
      mem(
        .clk,

        .wen(heap_enq),
        .waddr(heap_allocIdx),
        .wdata(heap_enqData),

        .raddr(readReq),
        .rdata(readRsp)
        );

endmodule // cci_mpf_prim_heap


// ========================================================================
//
//   Multiple implementations of the heap control logic.  Each is
//   appropriate for a different heap size.
//
// ========================================================================


//
// Wrapper for heap control that picks an implementation appropriate
// for the size of the heap.
//
module cci_mpf_prim_heap_ctrl
  #(
    parameter N_ENTRIES = 32,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic enq,                                // Allocate an entry
    output logic notFull,                            // Is scoreboard full?
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    input  logic free,                               // enable free freeIdx
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    generate
        if ((N_ENTRIES * $clog2(N_ENTRIES)) <= 1024)
        begin : lr
            // Small heap -- use LUTRAM
            cci_mpf_prim_heap_ctrl_lutram
              #(
                .N_ENTRIES(N_ENTRIES),
                .MIN_FREE_SLOTS(MIN_FREE_SLOTS)
                )
              fl
               (
                .clk,
                .reset,
                .enq,
                .notFull,
                .allocIdx,
                .free,
                .freeIdx
                );
        end
        else
        begin : br
            // Large heap -- use BRAM
            cci_mpf_prim_heap_ctrl_bram
              #(
                .N_ENTRIES(N_ENTRIES),
                .MIN_FREE_SLOTS(MIN_FREE_SLOTS)
                )
              fl
               (
                .clk,
                .reset,
                .enq,
                .notFull,
                .allocIdx,
                .free,
                .freeIdx
                );
        end
    endgenerate


    //
    // Check heap integrity.  Don't allow double free!
    //

    // synthesis translate_off
    logic [N_ENTRIES-1 : 0] entry_busy;

    // Track free/busy entries
    always_ff @(posedge clk)
    begin
        if (enq)
        begin
            entry_busy[allocIdx] <= 1'b1;
        end

        if (free)
        begin
            entry_busy[freeIdx] <= 1'b0;
        end

        if (reset)
        begin
            entry_busy <= N_ENTRIES'(0);
        end
    end

    logic error;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            error <= 1'b0;
        end
        else
        begin
            error <= 1'b0;

            if (enq && free && (allocIdx == freeIdx))
            begin
                $error("cci_mpf_prim_heap: Alloc and free same cycle (idx 0x%0h)", allocIdx);
                error <= 1'b1;
            end

            if (enq && entry_busy[allocIdx])
            begin
                $error("cci_mpf_prim_heap: Alloc busy entry (idx 0x%0h)", allocIdx);
                error <= 1'b1;
            end

            if (free && ! entry_busy[freeIdx])
            begin
                $error("cci_mpf_prim_heap: Free unused entry (idx 0x%0h)", freeIdx);
                error <= 1'b1;
            end
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_prim_heap_ctrl


//
// Heap free list implemented in LUTRAM.
//
module cci_mpf_prim_heap_ctrl_lutram
  #(
    parameter N_ENTRIES = 32,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic enq,                                // Allocate an entry
    output logic notFull,                            // Is scoreboard full?
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    input  logic free,                               // enable free freeIdx
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;


    // ====================================================================
    //
    // Free list.
    //
    // ====================================================================

    //
    // Prefetch the next entry to be allocated.
    //
    logic pop_free;
    logic free_idx_avail;
    logic free_idx_above_min;

    t_idx free_head_idx;
    t_idx free_tail_idx;

    // Need a new entry in allocIdx?
    logic need_pop_free;
    // Is a free entry available?
    assign pop_free = need_pop_free && free_idx_avail;

    // Heap isn't full as long as an index is available and the number of free
    // slots is above the MIN_FREE_SLOTS threshold.
    logic fifo_notEmpty;
    assign notFull = fifo_notEmpty && free_idx_above_min;

    cci_mpf_prim_fifo2
      #(
        .N_DATA_BITS($clog2(N_ENTRIES))
        )
      alloc_fifo
       (
        .clk,
        .reset,
        .enq_data(free_head_idx),
        .enq_en(pop_free),
        .notFull(need_pop_free),
        .first(allocIdx),
        .deq_en(enq),
        .notEmpty(fifo_notEmpty)
        );


    logic initialized;
    t_idx init_idx;

    //
    // Free list memory
    //
    logic free_wen;
    t_idx free_widx_next;
    t_idx free_rnext;

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS($bits(t_idx))
        )
      freeList
       (
        .clk,
        .reset,

        .wen(free_wen),
        .waddr(free_tail_idx),
        .wdata(free_widx_next),

        .raddr(free_head_idx),
        .rdata(free_rnext)
        );


    // Pop from free list
    always_ff @(posedge clk)
    begin
        if (pop_free)
        begin
            free_head_idx <= free_rnext;
        end

        if (reset)
        begin
            free_head_idx <= t_idx'(0);
        end
    end


    // Push released entry on the tail of a list, delayed by a cycle for
    // timing.
    logic free_q;
    t_idx freeIdx_q;
    always_ff @(posedge clk)
    begin
        free_q <= free;
        freeIdx_q <= freeIdx;

        if (reset)
        begin
            free_q <= 1'b0;
        end
    end

    assign free_wen = ! initialized || free_q;
    assign free_widx_next = (! initialized ? init_idx : freeIdx_q);


    always_ff @(posedge clk)
    begin
        if (free_wen)
        begin
            // Move tail pointer to index just pushed
            free_tail_idx <= free_widx_next;
        end

        if (reset)
        begin
            free_tail_idx <= t_idx'(0);
        end
    end


    // Initialize the free list and track the number of free entries
    t_idx num_free;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            // Start pushing with entry 1 since entry 0 starts as the
            // free list head pointer above.
            init_idx <= t_idx'(1);
            initialized <= 1'b0;

            // Reserve an entry that must stay on the free list.
            // This guarantees that free_head_idx ever goes
            // NULL, which would require managing a special case.
            num_free <= t_idx'(N_ENTRIES - 1);
            free_idx_avail <= 1'b0;
            free_idx_above_min <= 1'b0;
        end
        else
        begin
            if (! initialized)
            begin
                init_idx <= init_idx + t_idx'(1);

                if (init_idx == t_idx'(N_ENTRIES-1))
                begin
                    // Initialization complete
                    initialized <= 1'b1;
                    free_idx_avail <= 1'b1;
                    free_idx_above_min <= 1'b1;

                    assert (N_ENTRIES > 1 + MIN_FREE_SLOTS) else
                       $fatal("cci_mpf_prim_heap: Heap too small");
                end
            end
            else if (free_q != pop_free)
            begin
                if (free_q)
                begin
                    num_free <= num_free + t_idx'(1);
                    free_idx_avail <= 1'b1;
                    free_idx_above_min <= (num_free >= t_idx'(MIN_FREE_SLOTS));

                    assert (num_free < N_ENTRIES - 1) else
                       $fatal("cci_mpf_prim_heap: Too many free items. Pushed one twice?");
                end
                else
                begin
                    num_free <= num_free - t_idx'(1);
                    free_idx_avail <= (num_free > t_idx'(0));
                    free_idx_above_min <= (num_free > t_idx'(MIN_FREE_SLOTS+1));

                    assert (num_free != 0) else
                       $fatal("cci_mpf_prim_heap: alloc from empty heap!");
                end
            end
        end
    end


    cci_mpf_prim_heap_ctrl_checker
      #(
        .N_ENTRIES(N_ENTRIES)
        )
      ctrl_checker
       (
        .clk,
        .reset,
        .enq,
        .allocIdx,
        .free,
        .freeIdx
        );

endmodule // cci_mpf_prim_heap_ctrl_lutram


//
// BRAM implementation for large heaps.  This version is more complicated
// than the LUTRAM control because of the multi-cycle latency of BRAM.
//
module cci_mpf_prim_heap_ctrl_bram
  #(
    parameter N_ENTRIES = 32,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic enq,                                // Allocate an entry
    output logic notFull,                            // Is scoreboard full?
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    input  logic free,                               // enable free freeIdx
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;


    // ====================================================================
    //
    // Free list.
    //
    // ====================================================================

    //
    // Prefetch the next entry to be allocated.
    //
    logic pop_free;
    logic free_idx_avail;
    logic free_idx_above_min;
    t_idx next_free_idx;

    // Need a new entry in allocIdx?
    logic need_pop_free;
    // Is a free entry available?
    assign pop_free = need_pop_free && free_idx_avail;

    // Heap isn't full as long as an index is available and the number of free
    // slots is above the MIN_FREE_SLOTS threshold.
    logic fifo_notEmpty;
    assign notFull = fifo_notEmpty && free_idx_above_min;

    cci_mpf_prim_fifo2
      #(
        .N_DATA_BITS($clog2(N_ENTRIES))
        )
      alloc_fifo
       (
        .clk,
        .reset,
        .enq_data(next_free_idx),
        .enq_en(pop_free),
        .notFull(need_pop_free),
        .first(allocIdx),
        .deq_en(enq),
        .notEmpty(fifo_notEmpty)
        );


    // There are two free list heads to deal with the 2 cycle latency of
    // BRAM reads.  The lists are balanced, since both push and pop of
    // free entries are processed round-robin.
    t_idx free_head_idx[0:1];
    t_idx free_head_idx_reg[0:1];
    t_idx free_tail_idx[0:1];
    logic head_rr_select;
    logic tail_rr_select;

    logic initialized;
    t_idx init_idx;

    assign next_free_idx = free_head_idx[head_rr_select];

    //
    // Free list memory
    //
    logic free_wen;
    t_idx free_widx_next;
    t_idx free_rnext;

    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS($bits(t_idx)),
        .N_OUTPUT_REG_STAGES(1)
        )
      freeList
       (
        .clk,

        .wen(free_wen),
        .waddr(free_tail_idx[tail_rr_select]),
        .wdata(free_widx_next),

        .raddr(free_head_idx[head_rr_select]),
        .rdata(free_rnext)
        );


    // Pop from free list
    logic pop_free_q;
    logic pop_free_qq;
    logic head_rr_select_q;
    logic head_rr_select_qq;

    always_comb
    begin
        for (int i = 0; i < 2; i = i + 1)
        begin
            if (pop_free_qq && (head_rr_select_qq == 1'(i)))
            begin
                // Did a pop from this free list two cycles.  Receive the
                // updated head pointer.
                free_head_idx[i] = free_rnext;
            end
            else
            begin
                // No pop -- no change
                free_head_idx[i] = free_head_idx_reg[i];
            end
        end
    end

    // Track pop until the free list read response is received
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            pop_free_q <= 1'b0;
            pop_free_qq <= 1'b0;
            head_rr_select <= 1'b0;

            // Entries 0 and 1 begin as the head pointers
            free_head_idx_reg[0] <= t_idx'(0);
            free_head_idx_reg[1] <= t_idx'(1);
        end
        else
        begin
            pop_free_q <= pop_free;
            pop_free_qq <= pop_free_q;

            // Register combinationally computed free_head_idx
            free_head_idx_reg[0] <= free_head_idx[0];
            free_head_idx_reg[1] <= free_head_idx[1];
        end

        head_rr_select_q <= head_rr_select;
        head_rr_select_qq <= head_rr_select_q;

        if (pop_free)
        begin
            head_rr_select <= ~head_rr_select;
        end
    end


    // Push released entry on the tail of a list, delayed by a cycle for
    // timing.
    logic free_q;
    t_idx freeIdx_q;
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            free_q <= 1'b0;
        end
        else
        begin
            free_q <= free;
        end

        freeIdx_q <= freeIdx;
    end
    
    assign free_wen = ! initialized || free_q;
    assign free_widx_next = (! initialized ? init_idx : freeIdx_q);


    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            free_tail_idx[0] <= t_idx'(0);
            free_tail_idx[1] <= t_idx'(1);
            tail_rr_select <= 1'b0;
        end
        else
        begin
            if (free_wen)
            begin
                // Move tail pointer to index just pushed
                free_tail_idx[tail_rr_select] <= free_widx_next;
                // Swap round-robin selector
                tail_rr_select <= ~tail_rr_select;
            end
        end
    end
    

    // Initialize the free list and track the number of free entries
    t_idx num_free;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            // Start pushing with entry 2 since entries 0 and 1 start as the
            // free list head pointers above.
            init_idx <= t_idx'(2);
            initialized <= 1'b0;

            // Reserve two entries that must stay on the free list.
            // This guarantees that neither free_head_idx ever goes
            // NULL, which would require managing a special case.
            num_free <= t_idx'(N_ENTRIES - 2);
            free_idx_avail <= 1'b0;
            free_idx_above_min <= 1'b0;
        end
        else
        begin
            if (! initialized)
            begin
                init_idx <= init_idx + t_idx'(1);

                if (init_idx == t_idx'(N_ENTRIES-1))
                begin
                    // Initialization complete
                    initialized <= 1'b1;
                    free_idx_avail <= 1'b1;
                    free_idx_above_min <= 1'b1;

                    assert (N_ENTRIES > 2 + MIN_FREE_SLOTS) else
                       $fatal("cci_mpf_prim_heap: Heap too small");
                end
            end
            else if (free_q != pop_free)
            begin
                if (free_q)
                begin
                    num_free <= num_free + t_idx'(1);
                    free_idx_avail <= 1'b1;
                    free_idx_above_min <= (num_free >= t_idx'(MIN_FREE_SLOTS));

                    assert (num_free < N_ENTRIES - 2) else
                       $fatal("cci_mpf_prim_heap: Too many free items. Pushed one twice?");
                end
                else
                begin
                    num_free <= num_free - t_idx'(1);
                    free_idx_avail <= (num_free > t_idx'(0));
                    free_idx_above_min <= (num_free > t_idx'(MIN_FREE_SLOTS+1));

                    assert (num_free != 0) else
                       $fatal("cci_mpf_prim_heap: alloc from empty heap!");
                end
            end
        end
    end


    cci_mpf_prim_heap_ctrl_checker
      #(
        .N_ENTRIES(N_ENTRIES)
        )
      ctrl_checker
       (
        .clk,
        .reset,
        .enq,
        .allocIdx,
        .free,
        .freeIdx
        );

endmodule // cci_mpf_prim_heap_ctrl_bram


//
// A simple control module for heaps using a bit vector to tag free indices.
//
module cci_mpf_prim_heap_ctrl_simple
  #(
    parameter N_ENTRIES = 32,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    // Allocate an entry
    input  logic enq,
    output logic notFull,
    // Index of new entry
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx,

    input  logic free,
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;

    // Head of the free list is prefetched to the allocIdx register.  The rest
    // of the list is flagged in a bit vector.
    logic [N_ENTRIES-1 : 0] free_vec;

    always_ff @(posedge clk)
    begin
        // Find the next free entry when an entry is allocated
        if (enq)
        begin
            // Find first free index
            for (int i = 0; i < N_ENTRIES; i = i + 1)
            begin
                if (free_vec[i])
                begin
                    allocIdx <= t_idx'(i);
                    free_vec[i] <= 1'b0;
                    break;
                end
            end
        end

        // Set the "free" bit for released entries
        if (free)
        begin
            free_vec[freeIdx] <= 1'b1;

            assert (free_vec[freeIdx] == 1'b0) else
                $fatal("cci_mpf_prim_heap.sv: Free unallocated entry!");
        end

        if (reset)
        begin
            allocIdx <= t_idx'(0);

            free_vec <= ~ (N_ENTRIES'(0));
            free_vec[0] <= 1'b0;
        end
    end


    // Track the number of free entries
    typedef logic [$clog2(N_ENTRIES) : 0] t_free_cnt;

    t_free_cnt num_free;
    assign notFull = (num_free >= t_free_cnt'(MIN_FREE_SLOTS));

    // Give credit for free one cycle late because it takes a cycle for a
    // released index to get to allocIdx when no other entries are free.
    logic free_q;

    always_ff @(posedge clk)
    begin
        free_q <= free;

        if (enq && ! free_q)
        begin
            num_free <= num_free - t_free_cnt'(1);

            assert (num_free != 0) else
                $fatal("cci_mpf_prim_heap.sv: Alloc from empty heap!");
        end

        if (! enq && free_q)
        begin
            num_free <= num_free + t_free_cnt'(1);
        end

        if (reset)
        begin
            num_free <= t_free_cnt'(N_ENTRIES);
            free_q <= 1'b0;
        end
    end

endmodule // cci_mpf_prim_heap_ctrl_simple



// ========================================================================
//
//   Internal error checker: detect duplicate allocation and deallocation.
//
// ========================================================================

module cci_mpf_prim_heap_ctrl_checker
  #(
    parameter N_ENTRIES = 32
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic enq,                                // Allocate an entry
    input  logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    input  logic free,                               // enable free freeIdx
    input  logic [$clog2(N_ENTRIES)-1 : 0] freeIdx
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;

    // synthesis translate_off

    logic chk_alloc;
    t_idx chk_alloc_idx;
    logic chk_free;
    t_idx chk_free_idx;

    always_ff @(posedge clk)
    begin
        chk_alloc <= enq;
        chk_alloc_idx <= allocIdx;

        chk_free <= free;
        chk_free_idx <= freeIdx;

        if (reset)
        begin
            chk_alloc <= 1'b0;
            chk_free <= 1'b0;
        end
    end


    logic [N_ENTRIES-1 : 0] chk_state;

    always_ff @(posedge clk)
    begin
        if (chk_alloc)
        begin
            assert ((chk_state[chk_alloc_idx] == 1'b0) || reset) else
                $fatal("cci_mpf_prim_heap.sv: HEAP double allocation!");

            chk_state[chk_alloc_idx] <= 1'b1;
        end

        if (chk_free)
        begin
            assert ((chk_state[chk_free_idx] == 1'b1) || reset) else
                $fatal("cci_mpf_prim_heap.sv: HEAP double free!");

            chk_state[chk_free_idx] <= 1'b0;
        end

        if (reset)
        begin
            chk_state <= N_ENTRIES'(0);
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_prim_heap_ctrl_checker
