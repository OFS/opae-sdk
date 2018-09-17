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
// Register-based counting filter.  Use this only for small filters!
// For larger filters, see the block-RAM-based banked counting filter
// below.
//
module cci_mpf_prim_filter_counting
  #(
    // Number of individual buckets in the filter
    parameter N_BUCKETS = 16,
    // Size of each bucket in the counting filter
    parameter BITS_PER_BUCKET = 4,
    // Number of clients attempting to test an entry
    parameter N_TEST_CLIENTS = 1,
    // Number of clients attempting to insert an entry
    parameter N_INSERT_CLIENTS = 1,
    // Number of clients attempting to remove an entry
    parameter N_REMOVE_CLIENTS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    // Test a bucket.  The test logic is combinational.  Outputs are available
    // in the same cycle as the request.
    input  logic [0 : N_TEST_CLIENTS-1][$clog2(N_BUCKETS)-1 : 0]   test_req,
    // Will counter overflow if incremented?
    output logic [0 : N_TEST_CLIENTS-1]                            test_notFull,
    output logic [0 : N_TEST_CLIENTS-1]                            test_isZero,

    input  logic [0 : N_INSERT_CLIENTS-1][$clog2(N_BUCKETS)-1 : 0] insert,
    input  logic [0 : N_INSERT_CLIENTS-1]                          insert_en,

    input  logic [0 : N_REMOVE_CLIENTS-1][$clog2(N_BUCKETS)-1 : 0] remove,
    input  logic [0 : N_REMOVE_CLIENTS-1]                          remove_en
    );
     
    // Storage for the counters
    logic [0 : N_BUCKETS-1][BITS_PER_BUCKET-1 : 0] counters;


    // ====================================================================
    //
    // Test logic
    //
    // ====================================================================

    genvar p;
    generate
        for (p = 0; p < N_TEST_CLIENTS; p = p + 1)
        begin : test
            // Technically, notFull should be 1 if there are at least
            // N_INSERT_CLIENTS entries remaining in the bucket. To avoid
            // an add we instead simply require that the high bit be 0,
            // trading storage for time.
            assign test_notFull[p] = (! counters[test_req[p]][BITS_PER_BUCKET-1]);

            assign test_isZero[p] = (counters[test_req[p]] == BITS_PER_BUCKET'(0));
        end
    endgenerate


    // ====================================================================
    //
    // Update logic
    //
    // ====================================================================

    //
    // Each bucket builds its own CAM against request ports
    //
    logic [0 : N_BUCKETS-1][$clog2(N_INSERT_CLIENTS+1)-1 : 0] delta_up;
    logic [0 : N_BUCKETS-1][$clog2(N_REMOVE_CLIENTS+1)-1 : 0] delta_down;

    always_comb
    begin
        for (int c = 0; c < N_BUCKETS; c = c + 1)
        begin
            delta_up[c] = 0;
            delta_down[c] = 0;

            // For each insert port
            for (int i = 0; i < N_INSERT_CLIENTS; i = i + 1)
            begin
                // Increment if insert port index matches and is enabled
                delta_up[c] =
                     delta_up[c] +
                     ($bits(delta_up[c]))'((insert[i] == c) && insert_en[i]);
            end

            // For each insert port
            for (int i = 0; i < N_REMOVE_CLIENTS; i = i + 1)
            begin
                // Decrement for remove ports
                delta_down[c] =
                     delta_down[c] +
                     ($bits(delta_up[c]))'((remove[i] == c) && remove_en[i]);
            end
        end
    end


    //
    // Update counters
    //
    genvar b;
    generate
        for (b = 0; b < N_BUCKETS; b = b + 1)
        begin : update
            always_ff @(posedge clk)
            begin
                if (reset)
                begin
                    counters[b] <= BITS_PER_BUCKET'(0);
                end
                else
                begin
                    counters[b] <= counters[b] +
                                   BITS_PER_BUCKET'(delta_up[b]) -
                                   BITS_PER_BUCKET'(delta_down[b]);
               end
            end
        end
    endgenerate
endmodule // cci_mpf_prim_filter_counting



// ========================================================================
//
// Banked counting filter.  The filter is fully bypassed so that values
// inserted are recorded the cycle after insertion.
//
// The filters are banked because the memory's read port for updates
// is oversubscribed by the required read-modify-write for both insertion
// and removal.  Insertion is always given priority.  While a single
// bank may work in many cases, removal requests may back up and cause
// false filter conflicts.
//
// It is the client's responsibility either to honor remove_notFull or
// to set N_REMOVE_FIFO_ENTRIES large enough that the FIFO can never fill.
//
// ========================================================================

module cci_mpf_prim_filter_banked_counting
  #(
    // Log2 of the number of parallel banks, specified this way because the
    // number of banks must be a power of 2.
    parameter N_BANKS_RADIX = 1,

    // Number of individual buckets in the filter
    parameter N_BUCKETS = 16,

    // Size of each bucket in the counting filter
    parameter BITS_PER_BUCKET = 4,

    // There are two classes of clients:  those requesting a bucket's
    // value and those testing that the bucket is zero.  Each client gets
    // its own block RAM and testing for zero requires only one bit.
    parameter N_TEST_VALUE_CLIENTS = 1,
    parameter N_TEST_IS_ZERO_CLIENTS = 1,

    // Number of entries in the remove port's FIFO.  The remove port
    // shares a block RAM port with insertion.  Remove requests are
    // buffered until they can be processed.
    parameter N_REMOVE_FIFO_ENTRIES = 128,
    parameter N_REMOVE_FIFO_THRESHOLD = 1
    )
   (
    input  logic clk,
    input  logic reset,
    output logic rdy,

    input  logic [N_TEST_VALUE_CLIENTS-1 : 0][$clog2(N_BUCKETS)-1 : 0] test_value_req,
    input  logic [N_TEST_VALUE_CLIENTS-1 : 0] test_value_en,
    output logic [N_TEST_VALUE_CLIENTS-1 : 0][BITS_PER_BUCKET-1 : 0] T2_test_value,

    input  logic [N_TEST_IS_ZERO_CLIENTS-1 : 0][$clog2(N_BUCKETS)-1 : 0] test_isZero_req,
    input  logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] test_isZero_en,
    output logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] T2_test_isZero,

    input  logic insert_en,
    input  logic [$clog2(N_BUCKETS)-1 : 0] insert,

    input  logic remove_en,
    input  logic [$clog2(N_BUCKETS)-1 : 0] remove,
    // Remove shares a port with insert.  Insertion has priority, so
    // the remove port may become full.
    output logic remove_notFull,
    output logic remove_almostFull
    );

    typedef logic [$clog2(N_BUCKETS)-1 : 0] t_bucket_idx;
    typedef logic [BITS_PER_BUCKET-1 : 0] t_bucket_value;

    localparam N_BANKS = 1 << N_BANKS_RADIX;
    typedef logic [N_BANKS_RADIX-1 : 0] t_bank_idx;
    // Bucket idx within a bank
    typedef logic [$clog2(N_BUCKETS)-N_BANKS_RADIX-1 : 0] t_bank_bucket_idx;

    // Pick a bank given a bucket
    function automatic t_bank_idx bankFromBucket(t_bucket_idx bucket_idx);
        if (N_BANKS_RADIX == 0)
        begin
            // Special case for 1 bank
            return t_bank_idx'(0);
        end
        else
        begin
            t_bank_idx bank_idx;
            t_bank_bucket_idx bank_bucket_idx;
            {bank_bucket_idx, bank_idx} = bucket_idx;
            return bank_idx;
        end
    endfunction

    // Pick a bucket within a bank
    function automatic t_bank_bucket_idx bankBucket(t_bucket_idx bucket_idx);
        if (N_BANKS_RADIX == 0)
        begin
            // Special case for 1 bank
            return t_bank_bucket_idx'(bucket_idx);
        end
        else
        begin
            t_bank_idx bank_idx;
            t_bank_bucket_idx bank_bucket_idx;
            {bank_bucket_idx, bank_idx} = bucket_idx;
            return bank_bucket_idx;
        end
    endfunction

    //
    // Insertion has priority.  Removal requests flow through a FIFO and are
    // processed when possible.
    //
    t_bucket_idx remove_idx;
    logic process_remove;
    logic remove_notEmpty;

    generate
        //
        // Pick a FIFO implementation based on required size.
        //
        if (N_REMOVE_FIFO_ENTRIES >= 64)
        begin : brf
            cci_mpf_prim_fifo_bram
              #(
                .N_DATA_BITS($bits(t_bucket_idx)),
                .N_ENTRIES(N_REMOVE_FIFO_ENTRIES),
                .THRESHOLD(N_REMOVE_FIFO_THRESHOLD)
                )
              remove_fifo_br
               (
                .clk,
                .reset,
                .enq_data(remove),
                .enq_en(remove_en),
                .notFull(remove_notFull),
                .almostFull(remove_almostFull),
                .first(remove_idx),
                .deq_en(process_remove),
                .notEmpty(remove_notEmpty)
                );
        end
        else
        begin : lrf
            cci_mpf_prim_fifo_lutram
              #(
                .N_DATA_BITS($bits(t_bucket_idx)),
                .N_ENTRIES(N_REMOVE_FIFO_ENTRIES),
                .THRESHOLD(N_REMOVE_FIFO_THRESHOLD),
                .REGISTER_OUTPUT(1)
                )
              remove_fifo_lr
               (
                .clk,
                .reset,
                .enq_data(remove),
                .enq_en(remove_en),
                .notFull(remove_notFull),
                .almostFull(remove_almostFull),
                .first(remove_idx),
                .deq_en(process_remove),
                .notEmpty(remove_notEmpty)
                );
        end
    endgenerate


    //
    // Banks of filters
    //
    t_bank_bucket_idx [N_TEST_VALUE_CLIENTS-1 : 0] bank_test_value_req[0 : N_BANKS-1];
    t_bank_bucket_idx [N_TEST_IS_ZERO_CLIENTS-1 : 0] bank_test_isZero_req[0 : N_BANKS-1];
    t_bucket_value [N_TEST_VALUE_CLIENTS-1 : 0] bank_test_value[0 : N_BANKS-1];
    logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] bank_test_isZero[0 : N_BANKS-1];
    logic bank_remove_notFull[0 : N_BANKS-1];
    logic bank_rdy[0 : N_BANKS-1];

    genvar b;
    genvar c;
    generate
        for (b = 0; b < N_BANKS; b = b + 1)
        begin : fb
            for (c = 0; c < N_TEST_VALUE_CLIENTS; c = c + 1)
            begin : tv
                assign bank_test_value_req[b][c] = bankBucket(test_value_req[c]);
            end

            for (c = 0; c < N_TEST_IS_ZERO_CLIENTS; c = c + 1)
            begin : tz
                assign bank_test_isZero_req[b][c] = bankBucket(test_isZero_req[c]);
            end

            cci_mpf_prim_filter_counting_bank
              #(
                // Buckets are spread evenly across banks
                .N_BUCKETS(N_BUCKETS >> N_BANKS_RADIX),
                .BITS_PER_BUCKET(BITS_PER_BUCKET),
                .N_TEST_VALUE_CLIENTS(N_TEST_VALUE_CLIENTS),
                .N_TEST_IS_ZERO_CLIENTS(N_TEST_IS_ZERO_CLIENTS)
                )
              filter_bank
               (
                .clk,
                .reset,
                .rdy(bank_rdy[b]),

                .test_value_req(bank_test_value_req[b]),
                .T2_test_value(bank_test_value[b]),

                .test_isZero_req(bank_test_isZero_req[b]),
                .T2_test_isZero(bank_test_isZero[b]),

                .insert_en(insert_en &&
                           (bankFromBucket(insert) == t_bank_idx'(b))),
                .insert(bankBucket(insert)),

                .remove_en(process_remove &&
                           (bankFromBucket(remove_idx) == t_bank_idx'(b))),
                .remove(bankBucket(remove_idx)),
                .remove_notFull(bank_remove_notFull[b])
                );
        end
    endgenerate

    assign rdy = bank_rdy[0];

    // Handle a removal request if the target bank is available
    assign process_remove = bank_remove_notFull[bankFromBucket(remove_idx)] &&
                            remove_notEmpty;

    // Record the bank used for tests and cascade down the pipeline
    t_bank_idx [N_TEST_VALUE_CLIENTS-1 : 0] test_value_which_bank[0 : 2];
    t_bank_idx [N_TEST_IS_ZERO_CLIENTS-1 : 0] test_isZero_which_bank[0 : 2];

    generate
        for (c = 0; c < N_TEST_VALUE_CLIENTS; c = c + 1)
        begin : btv
            assign test_value_which_bank[0][c] = bankFromBucket(test_value_req[c]);

            // Return result from the bank handling the request
            assign T2_test_value[c] = bank_test_value[test_value_which_bank[2][c]][c];
        end

        for (c = 0; c < N_TEST_IS_ZERO_CLIENTS; c = c + 1)
        begin : btz
            assign test_isZero_which_bank[0][c] = bankFromBucket(test_isZero_req[c]);

            // Return result from the bank handling the request
            assign T2_test_isZero[c] = bank_test_isZero[test_isZero_which_bank[2][c]][c];
        end
    endgenerate

    always_ff @(posedge clk)
    begin
        test_value_which_bank[1:2] <= test_value_which_bank[0:1];
        test_isZero_which_bank[1:2] <= test_isZero_which_bank[0:1];
    end

endmodule // cci_mpf_prim_filter_banked_counting


//
// One bank of a banked counting filter.
//
// This should be treated as a module internal to the counting filter.
// To allocate a single bank, clients should use
// cci_mpf_prim_filter_banked_counting and specify N_BANKS_RADIX 0.
//
module cci_mpf_prim_filter_counting_bank
  #(
    // Number of individual buckets in the filter
    parameter N_BUCKETS = 16,

    // Size of each bucket in the counting filter
    parameter BITS_PER_BUCKET = 4,

    // There are two classes of clients:  those request a bucket's value
    // and those testing that the bucket is zero.  Each client gets
    // its own block RAM and testing for zero requires only one bit.
    parameter N_TEST_VALUE_CLIENTS = 1,
    parameter N_TEST_IS_ZERO_CLIENTS = 1,

    // Number of entries in the remove port's FIFO.  The remove port
    // shares a block RAM port with insertion.  Remove requests are
    // buffered until they can be processed.
    parameter N_REMOVE_FIFO_ENTRIES = 4
    )
   (
    input  logic clk,
    input  logic reset,
    output logic rdy,

    input  logic [N_TEST_VALUE_CLIENTS-1 : 0][$clog2(N_BUCKETS)-1 : 0] test_value_req,
    output logic [N_TEST_VALUE_CLIENTS-1 : 0][BITS_PER_BUCKET-1 : 0] T2_test_value,

    input  logic [N_TEST_IS_ZERO_CLIENTS-1 : 0][$clog2(N_BUCKETS)-1 : 0] test_isZero_req,
    output logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] T2_test_isZero,

    input  logic insert_en,
    input  logic [$clog2(N_BUCKETS)-1 : 0] insert,

    input  logic remove_en,
    input  logic [$clog2(N_BUCKETS)-1 : 0] remove,
    // Remove shares a port with insert.  Insertion has priority, so
    // the remove port may become full.
    output logic remove_notFull
    );

    typedef logic [$clog2(N_BUCKETS)-1 : 0] t_bucket_idx;
    typedef logic [BITS_PER_BUCKET-1 : 0] t_bucket_value;


    //
    // Global memory update lines consumed by all categories of
    // memory here.
    //
    logic mem_upd_wr_en;
    t_bucket_idx mem_upd_wr_idx;
    t_bucket_value mem_upd_wr_val;
    logic mem_upd_wr_notZero;


    // ====================================================================
    //
    //   "Not full" memory client read port.
    //
    // ====================================================================

    // Bypass tracking state.  The 0:2 index on the right is the stage
    // in the read pipeline.
    t_bucket_idx [N_TEST_VALUE_CLIENTS-1 : 0] mem_port_val_rd_idx[0:2];
    logic [N_TEST_VALUE_CLIENTS-1 : 0] mem_port_val_rd_bypass_en[0:2];
    t_bucket_value [N_TEST_VALUE_CLIENTS-1 : 0] mem_port_val_rd_bypass_val[0:2];

    t_bucket_value [N_TEST_VALUE_CLIENTS-1 : 0] mem_port_val_rd_val;

    // Client port index
    genvar c;
    // Read pipeline stage index
    genvar p;

    generate
        for (c = 0; c < N_TEST_VALUE_CLIENTS; c = c + 1)
        begin : vm
            cci_mpf_prim_ram_simple_init
              #(
                .N_ENTRIES(N_BUCKETS),
                .N_DATA_BITS(BITS_PER_BUCKET),
                .N_OUTPUT_REG_STAGES(1),
                .REGISTER_WRITES(1)
                )
              mem_port_value
               (
                .clk,
                .reset,
                // rdy from mem_upd goes high at the same time
                .rdy(),

                .raddr(mem_port_val_rd_idx[0][c]),
                .rdata(mem_port_val_rd_val[c]),

                .waddr(mem_upd_wr_idx),
                .wen(mem_upd_wr_en),
                .wdata(mem_upd_wr_val)
                );

            assign mem_port_val_rd_idx[0][c] = test_value_req[c];

            // Detect the need for bypass.
            assign mem_port_val_rd_bypass_en[0][c] =
                     mem_upd_wr_en && (mem_port_val_rd_idx[0][c] == mem_upd_wr_idx);
            assign mem_port_val_rd_bypass_val[0][c] = mem_upd_wr_val;

            // Continue checking for the need to bypass.  The lookup pipeline
            // here is the same length as the update pipeline generating the
            // writes.  By checking in every stage of lookup we eventually
            // see all updates requested before the cycle the lookup began.
            for (p = 1; p <= 2; p = p + 1)
            begin : vm_p
                always_ff @(posedge clk)
                begin
                    mem_port_val_rd_bypass_en[p][c] <= mem_port_val_rd_bypass_en[p-1][c];
                    mem_port_val_rd_bypass_val[p][c] <= mem_port_val_rd_bypass_val[p-1][c];
                    mem_port_val_rd_idx[p][c] <= mem_port_val_rd_idx[p-1][c];

                    if (mem_upd_wr_en &&
                        (mem_port_val_rd_idx[p-1][c] == mem_upd_wr_idx))
                    begin
                        // Same bucket was updated this cycle!
                        mem_port_val_rd_bypass_en[p][c] <= 1'b1;
                        mem_port_val_rd_bypass_val[p][c] <= mem_upd_wr_val;
                    end
                end
            end

            // Return either the bypassed value or the value from memory
            assign T2_test_value[c] =
                (mem_port_val_rd_bypass_en[2][c] ?
                    mem_port_val_rd_bypass_val[2][c] :
                    mem_port_val_rd_val[c]);
        end
    endgenerate


    // ====================================================================
    //
    //   "Is zero" ports are for clients that simply need to know whether
    //   or not a bucket is empty.  E.g. stores seeking to avoid conflicts
    //   with any read.
    //
    // ====================================================================

    // Bypass tracking state.  The 0:2 index on the right is the stage
    // in the read pipeline.
    t_bucket_idx [N_TEST_IS_ZERO_CLIENTS-1 : 0] mem_port_iz_rd_idx[0:2];
    logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] mem_port_iz_rd_bypass_en[0:2];
    logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] mem_port_iz_rd_bypass_val[0:2];

    logic [N_TEST_IS_ZERO_CLIENTS-1 : 0] mem_port_iz_rd_val;

    generate
        // For each client
        for (c = 0; c < N_TEST_IS_ZERO_CLIENTS; c = c + 1)
        begin : izm
            if (N_BUCKETS > 1024)
            begin : br
                cci_mpf_prim_ram_simple_init
                  #(
                    .N_ENTRIES(N_BUCKETS),
                    .N_DATA_BITS(1),
                    .N_OUTPUT_REG_STAGES(1),
                    .REGISTER_WRITES(1)
                    )
                  mem_port_is_zero
                   (
                    .clk,
                    .reset,
                    // rdy from mem_upd goes high at the same time
                    .rdy(),

                    .raddr(mem_port_iz_rd_idx[0][c]),
                    .rdata(mem_port_iz_rd_val[c]),

                    .waddr(mem_upd_wr_idx),
                    .wen(mem_upd_wr_en),
                    .wdata(mem_upd_wr_notZero)
                    );
            end
            else
            begin : lr
                // Read result pipeline register to make timing of LUTRAM the
                // same as BRAM.
                logic is_zero_rd_val[0:1];

                cci_mpf_prim_lutram_init
                  #(
                    .N_ENTRIES(N_BUCKETS),
                    .N_DATA_BITS(1)
                    )
                  mem_port_is_zero
                   (
                    .clk,
                    .reset,
                    // rdy from mem_upd goes high at the same time
                    .rdy(),

                    .raddr(mem_port_iz_rd_idx[0][c]),
                    .rdata(is_zero_rd_val[0]),

                    .waddr(mem_upd_wr_idx),
                    .wen(mem_upd_wr_en),
                    .wdata(mem_upd_wr_notZero)
                    );

                always_ff @(posedge clk)
                begin
                    is_zero_rd_val[1] <= is_zero_rd_val[0];
                    mem_port_iz_rd_val[c] <= is_zero_rd_val[1];
                end
            end

            assign mem_port_iz_rd_idx[0][c] = test_isZero_req[c];

            // Detect the need for bypass.
            assign mem_port_iz_rd_bypass_en[0][c] =
                     mem_upd_wr_en && (mem_port_iz_rd_idx[0][c] == mem_upd_wr_idx);
            assign mem_port_iz_rd_bypass_val[0][c] = mem_upd_wr_notZero;

            // Continue checking for the need to bypass.  The lookup pipeline
            // here is the same length as the update pipeline generating the
            // writes.  By checking in every stage of lookup we eventually
            // see all updates requested before the cycle the lookup began.
            for (p = 1; p <= 2; p = p + 1)
            begin : izm_p
                always_ff @(posedge clk)
                begin
                    mem_port_iz_rd_bypass_en[p][c] <= mem_port_iz_rd_bypass_en[p-1][c];
                    mem_port_iz_rd_bypass_val[p][c] <= mem_port_iz_rd_bypass_val[p-1][c];
                    mem_port_iz_rd_idx[p][c] <= mem_port_iz_rd_idx[p-1][c];

                    if (mem_upd_wr_en &&
                        (mem_port_iz_rd_idx[p-1][c] == mem_upd_wr_idx))
                    begin
                        // Same bucket was updated this cycle!
                        mem_port_iz_rd_bypass_en[p][c] <= 1'b1;
                        mem_port_iz_rd_bypass_val[p][c] <= mem_upd_wr_notZero;
                    end
                end
            end

            // Return either the bypassed value or the value from memory
            assign T2_test_isZero[c] =
                (mem_port_iz_rd_bypass_en[2][c] ?
                    ~mem_port_iz_rd_bypass_val[2][c] :
                    ~mem_port_iz_rd_val[c]);
        end
    endgenerate


    // ====================================================================
    //
    //   FIFO buffer of incoming remove requests.  These will be processed
    //   whenever the update port isn't already used by insertion.
    //
    // ====================================================================

    t_bucket_idx remove_idx;
    logic process_remove;
    logic remove_notEmpty;

    cci_mpf_prim_fifo_lutram
      #(
        .N_ENTRIES(8),
        .N_DATA_BITS($bits(t_bucket_idx))
        )
      remove_fifo
       (
        .clk,
        .reset,
        .enq_data(remove),
        .enq_en(remove_en),
        .notFull(remove_notFull),
        .first(remove_idx),
        .deq_en(process_remove),
        .notEmpty(remove_notEmpty),
        .almostFull()
        );


    // ====================================================================
    //
    //   Update management.
    //
    //   Updates require a read-modify-write of a bucket.  The pipeline
    //   includes a bypass in order to support multiple updates in flight.
    //
    // ====================================================================

    logic mem_upd_rdy;

    t_bucket_idx mem_upd_rd_idx;
    t_bucket_value mem_upd_rd_val;

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(N_BUCKETS),
        .N_DATA_BITS(BITS_PER_BUCKET),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1)
        )
      mem_upd
       (
        .clk,
        .reset,
        // Have this memory represent all memories.  They are the same size so
        // initialize at the same rate.
        .rdy,

        .raddr(mem_upd_rd_idx),
        .rdata(mem_upd_rd_val),

        .waddr(mem_upd_wr_idx),
        .wen(mem_upd_wr_en),
        .wdata(mem_upd_wr_val)
        );


    // The pipeline is 3 cycles starting with read request and ending with
    // the write of mem_upd.  Entry 0 is used during the read request and
    // is set combinationally.  The others are registered downstream copies.
    // An extra pipeline stage is recorded because the upd_delta computation
    // is performed in stage 1 for timing when it naturally should happen in
    // stage 0.  By the time upd_delta is computed the required state has
    // reached the extra stage 3.
    logic upd_en[0:3];
    t_bucket_idx upd_idx[0:3];
    logic upd_is_insert[0:3];

    always_comb
    begin
        // There is only one update port and insert has priority over remove.
        process_remove = ! insert_en && remove_notEmpty;

        // Read from where?
        mem_upd_rd_idx = (insert_en ? insert : remove_idx);

        upd_en[0] = insert_en || remove_notEmpty;
        upd_idx[0] = mem_upd_rd_idx;
        upd_is_insert[0] = insert_en;
    end

    //
    // Data flowing through the update pipeline
    //
    generate
        for (p = 1; p <= 3; p = p + 1)
        begin : upipe
            always_ff @(posedge clk)
            begin
                upd_en[p] <= upd_en[p-1];
                upd_idx[p] <= upd_idx[p-1];
                upd_is_insert[p] <= upd_is_insert[p-1];

                if (reset)
                begin
                    upd_en[p] <= 1'b0;
                end
            end
        end
    endgenerate


    // Bypassed update delta
    int delta;
    t_bucket_value upd_delta[1:2];

    always_comb
    begin
        //
        // Bypass from existing requests that won't be reflected in the
        // read for update started this cycle.  This LUT computes the
        // total delta for all in-flight pipelined updates relative to
        // the memory read result.
        //
        // The delta computation runs the cycle after the read is requested.
        //
        casex ({ upd_en[1], upd_is_insert[1],
                 upd_en[2] && (upd_idx[1] == upd_idx[2]), upd_is_insert[2],
                 upd_en[3] && (upd_idx[1] == upd_idx[3]), upd_is_insert[3] })

            6'b0x0x0x: delta = 0;
            6'b0x0x10: delta = -1;
            6'b0x0x11: delta = 1;

            6'b0x100x: delta = -1;
            6'b0x1010: delta = -2;
            6'b0x1011: delta = 0;
            6'b0x110x: delta = 1;
            6'b0x1110: delta = 0;
            6'b0x1111: delta = 2;

            6'b100x0x: delta = -1;
            6'b100x10: delta = -2;
            6'b100x11: delta = 0;

            6'b10100x: delta = -2;
            6'b101010: delta = -3;
            6'b101011: delta = -1;
            6'b10110x: delta = 0;
            6'b101110: delta = -1;
            6'b101111: delta = 1;

            6'b110x0x: delta = 1;
            6'b110x10: delta = 0;
            6'b110x11: delta = 2;

            6'b11100x: delta = 0;
            6'b111010: delta = -1;
            6'b111011: delta = 1;
            6'b11110x: delta = 2;
            6'b111110: delta = 1;
            6'b111111: delta = 3;

            default:   delta = 0;
        endcase

        upd_delta[1] = t_bucket_value'(delta);
    end

    always_ff @(posedge clk)
    begin
        upd_delta[2] <= upd_delta[1];
    end


    //
    // Update memory
    //
    assign mem_upd_wr_idx = upd_idx[2];
    assign mem_upd_wr_en = upd_en[2];
    assign mem_upd_wr_val = mem_upd_rd_val + upd_delta[2];
    assign mem_upd_wr_notZero = (|(mem_upd_wr_val));


    // synthesis translate_off

    //
    // Check updates to the counters without the need for bypasses, etc.
    //
    t_bucket_value bucket_checker[0 : N_BUCKETS];

    t_bucket_value bucket_check_upd;
    assign bucket_check_upd = bucket_checker[mem_upd_wr_idx] +
                              (upd_is_insert[2] ? t_bucket_value'(1) :
                                                  t_bucket_value'(-1));

    always_ff @(posedge clk)
    begin
        if (mem_upd_wr_en)
        begin
            bucket_checker[mem_upd_wr_idx] <= bucket_check_upd;
        end

        if (reset)
        begin
            for (int b = 0; b < N_BUCKETS; b = b + 1)
            begin
                bucket_checker[b] = t_bucket_value'(0);
            end
        end
    end

    always_ff @(posedge clk)
    begin
        if (! reset && mem_upd_wr_en)
        begin
            assert ((&(bucket_checker[mem_upd_wr_idx]) == 1'b0) || ! upd_is_insert[2]) else
                $fatal("cci_mpf_prim_filter_counting.sv: Bucket 0x%0x overflow", mem_upd_wr_idx);

            assert ((|(bucket_checker[mem_upd_wr_idx]) != 1'b0) || upd_is_insert[2]) else
                $fatal("cci_mpf_prim_filter_counting.sv: Bucket 0x%0x underflow", mem_upd_wr_idx);

            assert (mem_upd_wr_val == bucket_check_upd) else
                $fatal("cci_mpf_prim_filter_counting.sv: Bucket 0x%0x wrote %0d expected %0d",
                       mem_upd_wr_idx, mem_upd_wr_val, bucket_check_upd);
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_prim_filter_counting_bank
