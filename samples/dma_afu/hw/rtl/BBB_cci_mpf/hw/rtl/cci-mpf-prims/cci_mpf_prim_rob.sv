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
// ROB that returns data FIFO by sorting out of order arrival of
// the payload.  The ROB combines two pieces of data with each entry:
// meta-data that is supplied at the time an index is allocated and the
// late-arriving data.  Both are returned together through first and first_meta.
// Within the driver this is typically used to combine a parent's Mdata
// field for the response header in combination with read data.
//

module cci_mpf_prim_rob
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    parameter N_META_BITS = 1,
    // Threshold below which heap asserts "full"
    parameter MIN_FREE_SLOTS = 1,
    // Maximum number of entries that can be allocated in a single cycle.
    // This is used for multi-line requests.
    parameter MAX_ALLOC_PER_CYCLE = 1,

    // Register output if non-zero
    parameter REGISTER_OUTPUT = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // Add one or more new entries in the ROB.  No payload, just control.
    // The ROB returns a handle -- the index where the payload should
    // be written.  When allocating multiple entries the indices are
    // sequential.
    input  logic [$clog2(MAX_ALLOC_PER_CYCLE) : 0] alloc,
    input  logic [N_META_BITS-1 : 0] allocMeta,      // Save meta-data for new entry
    output logic notFull,                            // Is ROB full?
    output logic [$clog2(N_ENTRIES)-1 : 0] allocIdx, // Index of new entry

    // Payload write.  No ready signal.  The ROB must always be ready
    // to receive data.
    input  logic enqData_en,                        // Store data for existing entry
    input  logic [$clog2(N_ENTRIES)-1 : 0] enqDataIdx,
    input  logic [N_DATA_BITS-1 : 0] enqData,

    // Ordered output
    input  logic deq_en,                            // Deq oldest entry
    output logic notEmpty,                          // Is oldest entry ready?
    // Data arrives TWO CYCLES AFTER notEmpty and deq_en are asserted
    output logic [N_DATA_BITS-1 : 0] T2_first,      // Data for oldest entry
    output logic [N_META_BITS-1 : 0] T2_firstMeta   // Meta-data for oldest entry
    );

    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;

    // Count that can include both 0 and N_ENTRIES.
    typedef logic [$clog2(N_ENTRIES) : 0] t_idx_nowrap;

    t_idx newest;
    t_idx oldest, oldest_q;
    logic validBits_rdy[0:1];

    // notFull is true as long as there are at least MIN_FREE_SLOTS available
    // at the end of the ring buffer. The computation is complicated by the
    // wrapping pointer.
    logic newest_ge_oldest;
    assign newest_ge_oldest = (newest >= oldest);
    assign notFull =
        validBits_rdy[0] &&
        (({1'b0, newest} + t_idx_nowrap'(MIN_FREE_SLOTS)) < {newest_ge_oldest, oldest});

    // enq allocates a slot and returns the index of the slot.
    assign allocIdx = newest;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            newest <= 0;
        end
        else
        begin
            newest <= newest + t_idx'(alloc);

            assert ((alloc == 0) || ((newest + t_idx'(alloc)) != oldest)) else
                $fatal("cci_mpf_prim_rob: Can't ENQ when FULL!");
            assert ((N_ENTRIES & (N_ENTRIES - 1)) == 0) else
                $fatal("cci_mpf_prim_rob: N_ENTRIES must be a power of 2!");
        end
    end

    // Bump the oldest pointer on deq
    always_ff @(posedge clk)
    begin
        oldest <= oldest + deq_en;
        oldest_q <= oldest;

        if (reset)
        begin
            oldest <= 0;
        end
    end

    always_ff @(negedge clk)
    begin
        if (! reset)
        begin
            assert(! deq_en || notEmpty) else
              $fatal("cci_mpf_prim_rob: Can't DEQ when EMPTY!");
        end
    end


    // ====================================================================
    //
    //  Track data arrival
    //
    // ====================================================================

    //
    // Valid bits are stored in RAM.  To avoid the problem of needing
    // two write ports to the memory we toggle the meaning of the valid
    // bit on every trip around the ring buffer.  On the first trip around
    // the valid tag is 1 since the memory is initialized to 0.  On the
    // second trip around the valid bits start 1, having been set on the
    // previous loop.  The target is thus changed to 0.
    //
    logic valid_tag, valid_tag_q;
    t_idx enqDataIdx_q;

    always_ff @(posedge clk)
    begin
        enqDataIdx_q <= enqDataIdx;
        valid_tag_q <= valid_tag;

        // Toggle the valid_tag every trip around the ring buffer.
        if (deq_en && (&(oldest) == 1'b1))
        begin
            valid_tag <= ~valid_tag;
        end

        if (reset)
        begin
            valid_tag <= 1'b1;
        end
    end


    //
    // Track the number of valid entries ready to go.
    //
    // A small counter works fine.  The count just has to stay ahead of
    // the ROB's output.
    //
    typedef logic [2:0] t_valid_cnt;
    t_valid_cnt num_valid;

    // Valid bits array.  Memory reads are multi-cycle.  Valid bits
    // are stored in two banks and accessed in alternate cycles to hide
    // the latency.
    logic [$clog2(N_ENTRIES)-2 : 0] test_valid_idx[0:1], test_valid_idx_q[0:1];
    logic test_valid_value_q[0:1];
    logic test_valid_tgt;
    logic test_valid_bypass_q[0:1];

    // Which test_valid bank to read?
    logic test_valid_bank;

    // The next entry is ready when its valid tag matches
    // the target for the current trip around the ring buffer.
    logic test_valid_is_set;
    assign test_valid_is_set =
        (test_valid_tgt == test_valid_value_q[test_valid_bank]) ||
        test_valid_bypass_q[test_valid_bank];

    // Don't exceed num_valid's bounds
    logic test_is_valid;
    assign test_is_valid = test_valid_is_set && (&(num_valid) != 1'b1);

    assign notEmpty = (num_valid != t_valid_cnt'(0));

    //
    // Update the pointer to the oldest valid entry.
    //
    always_ff @(posedge clk)
    begin
        test_valid_idx_q <= test_valid_idx;

        if (test_is_valid)
        begin
            test_valid_idx[test_valid_bank] <= test_valid_idx[test_valid_bank] + 1;

            // Invert the comparison tag when wrapping, just like valid_tag.
            if (test_valid_bank && (&(test_valid_idx[1]) == 1'b1))
            begin
                test_valid_tgt <= ~test_valid_tgt;
            end

            test_valid_bank <= ~test_valid_bank;
        end

        if (reset || ! validBits_rdy[0])
        begin
            test_valid_idx[0] <= 0;
            test_valid_idx[1] <= 0;

            test_valid_bank <= 1'b0;
            test_valid_tgt <= 1'b1;
        end
    end

    //
    // Count the number of oldest data-ready entries in the ROB.
    //
    always_ff @(posedge clk)
    begin
        num_valid <= num_valid - t_valid_cnt'(deq_en) +
                                 t_valid_cnt'(test_is_valid);

        if (reset)
        begin
            num_valid <= t_valid_cnt'(0);
        end
    end

    genvar p;
    generate
        for (p = 0; p <= 1; p = p + 1)
        begin : r
            logic p_wen, p_wen_q;
            logic [$clog2(N_ENTRIES)-2 : 0] p_waddr_q;

            //
            // Don't confuse the two banks of valid bits in the ROB with this
            // banked implementation of a LUTRAM.  The banked LUTRAM exists
            // for timing, breaking the deep MUX required for a large array
            // into multiple cycles.  The ROB valid bits are in two banks,
            // which are implemented as multi-bank LUTRAMs.
            //
            cci_mpf_prim_lutram_init_banked
              #(
                // Two ROB valid bit banks, each with half the entries
                .N_ENTRIES(N_ENTRIES >> 1),
                .N_DATA_BITS(1),
                .INIT_VALUE(1'b0),
                // Writes are delayed a cycle for timing so bypass new writes
                // to reads in the same cycle.
                .READ_DURING_WRITE("NEW_DATA"),
                // LUTRAM banks -- could be any number.  This is not ROB
                // valid bit banks.
                .N_BANKS(4)
                )
              validBits
               (
                .clk,
                .reset,
                .rdy(validBits_rdy[p]),

                .raddr(test_valid_idx[p]),
                .T1_rdata(test_valid_value_q[p]),

                .wen(p_wen_q),
                .waddr(p_waddr_q),
                // Indicate the entry is valid using the appropriate tag to
                // mark validity.  Indices less than oldest are very young
                // and have the tag for the next ring buffer loop.  Indicies
                // greater than or equal to oldest use the tag for the current
                // trip.
                .wdata((enqDataIdx_q >= oldest_q) ? valid_tag_q : ~valid_tag_q)
                );

            // Use the low bit of the data index as a bank select bit
            assign p_wen = enqData_en && (enqDataIdx[0] == p[0]);
            assign p_waddr_q = enqDataIdx_q[1 +: $bits(enqDataIdx)-1];

            // Bypass -- note writes to the bank being read
            assign test_valid_bypass_q[p] = (p_wen_q &&
                                             (p_waddr_q == test_valid_idx_q[p]));

            always_ff @(posedge clk)
            begin
                p_wen_q <= p_wen;
            end
        end
    endgenerate


    // ====================================================================
    //
    //  Storage.
    //
    // ====================================================================

    //
    // Data
    //
    cci_mpf_prim_ram_simple
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0)
        )
      memData
       (
        .clk,

        .waddr(enqDataIdx),
        .wen(enqData_en),
        .wdata(enqData),

        .raddr(oldest),
        .rdata(T2_first)
        );

    //
    // Meta-data memory.
    //
    generate
        if (N_META_BITS != 0)
        begin : genMeta
            cci_mpf_prim_ram_simple
              #(
                .N_ENTRIES(N_ENTRIES),
                .N_DATA_BITS(N_META_BITS),
                .N_OUTPUT_REG_STAGES(1)
                )
              memMeta
               (
                .clk(clk),

                .waddr(newest),
                .wen(alloc != 0),
                .wdata(allocMeta),

                .raddr(oldest),
                .rdata(T2_firstMeta)
                );
        end
        else
        begin : noMeta
            assign T2_firstMeta = 'x;
        end
    endgenerate

endmodule // cci_mpf_prim_rob
