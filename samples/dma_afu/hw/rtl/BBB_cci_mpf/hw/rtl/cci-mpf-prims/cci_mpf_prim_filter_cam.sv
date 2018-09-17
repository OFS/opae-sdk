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


module cci_mpf_prim_filter_cam
  #(
    // Number of individual buckets in the filter
    parameter N_BUCKETS = 16,
     // Size of each bucket in the counting filter
    parameter BITS_PER_BUCKET = 4,
    // Number of clients attempting to test an entry
    parameter N_TEST_CLIENTS = 1,
    // Include values inserted in the current cycle in the test this cycle?
    // Be careful here, since enabling the bypass can cause a dependence
    // loop. The bypass is useful if there is a pipeline stage between
    // test and insertion.
    parameter BYPASS_INSERT_TO_TEST = 0
    )
   (
    input  logic clk,
    input  logic reset,

    // Test values against the set of values stored in the CAM.
    input  logic [0 : N_TEST_CLIENTS-1][BITS_PER_BUCKET-1 : 0] test_value,
    input  logic [0 : N_TEST_CLIENTS-1] test_en,
    output logic [0 : N_TEST_CLIENTS-1] T0_test_notPresent,

    // Mirrors test_notPresent but one or two cycles delayed. Internally
    // this gives the logic more time to implement the CAM and the
    // computation is split across the cycles.  The code instantiating the
    // CAM should pick a latency appropriate to the target frequency
    // and hardware.
    output logic [0 : N_TEST_CLIENTS-1] T1_test_notPresent,
    output logic [0 : N_TEST_CLIENTS-1] T2_test_notPresent,

    // Insert one value into the CAM in a specific slot. Slots are managed
    // outside this module.
    input  logic [$clog2(N_BUCKETS)-1 : 0] insert_idx,
    input  logic [BITS_PER_BUCKET-1 : 0] insert_value,
    input  logic insert_en,

    // Remove (invalidate) entries from the CAM.
    input  logic [$clog2(N_BUCKETS)-1 : 0] remove_idx,
    input  logic remove_en
    );
     
    // Storage for the values
    logic [0 : N_BUCKETS-1][BITS_PER_BUCKET-1 : 0] values;
    logic [0 : N_BUCKETS-1] valid;

    // Test insert and protect
    logic [0 : N_TEST_CLIENTS-1] insert_match;

    // Insertion pipeline registers
    logic [$clog2(N_BUCKETS)-1 : 0] insert_idx_q;
    logic [BITS_PER_BUCKET-1 : 0]   insert_value_q;
    logic                           insert_en_q;

    //
    // An array to hold in flight insert values and multi-cycle tests that
    // might be inserted but aren't yet in the CAM.  Values in this array
    // must also be checked.  The arrays are declared to be the maximum
    // possible size used.
    //
    logic [BITS_PER_BUCKET-1 : 0] inflight_values[0 : 1];
    logic inflight_valids[0 : 1];
    logic [0 : N_TEST_CLIENTS-1] inflight_test_match;

    // Number of inflight_values entries actually used 
    localparam NUM_INFLIGHT_VALUES = 1 + BYPASS_INSERT_TO_TEST;

    //
    // Compare test_value against various registers holding in flight state.
    //
    always_comb
    begin
        // Insertion requests are registered for timing.  Compare test_value
        // against an entry that will be active next cycle.
        inflight_valids[0] = insert_en_q;
        inflight_values[0] = insert_value_q;

        // Compare against values inserted this cycle?  This entry will be
        // considered only if BYPASS_INSERT_TO_TEST is set.
        inflight_valids[1] = insert_en;
        inflight_values[1] = insert_value;

        // Compare test_value to inflight_values
        for (int c = 0; c < N_TEST_CLIENTS; c = c + 1)
        begin
            inflight_test_match[c] = 1'b0;

            for (int v = 0; v < NUM_INFLIGHT_VALUES; v = v + 1)
            begin
                inflight_test_match[c] =
                    inflight_test_match[c] ||
                    (inflight_valids[v] && (inflight_values[v] == test_value[c]));
            end
        end
    end


    logic [0 : N_TEST_CLIENTS-1][0 : N_BUCKETS-1] test_match;

    always_comb
    begin
        //
        // Compare test input against all values stored in the filter.
        //
        for (int c = 0; c < N_TEST_CLIENTS; c = c + 1)
        begin
            // Construct a bit vector that is the result of comparing each
            // entry to the target.
            for (int b = 0; b < N_BUCKETS; b = b + 1)
            begin
                test_match[c][b] = (values[b] == test_value[c]);
            end

            // Is the value present in the CAM?
            T0_test_notPresent[c] = (! test_en[c] ||
                                     // Does any valid entry match?
                                     (! (|(valid & test_match[c])) &&
                                      // Does an in-flight entry match?
                                      ! (inflight_test_match[c])));
        end
    end


    // Registered equivalent of the combinational logic, producing the
    // T1_test_notPresent registered result.
    always_ff @(posedge clk)
    begin
        for (int c = 0; c < N_TEST_CLIENTS; c = c + 1)
        begin
            T1_test_notPresent[c] <= T0_test_notPresent[c];
        end
    end

      
    // Two cycle version of test_notPresent, with an intermediate comparison
    // stage.  This should produce the same state as T1_test_notPresent delayed
    // one cycle.
    logic [0 : N_TEST_CLIENTS-1] test_en_q;
    logic [0 : N_BUCKETS-1] valid_q;
    logic [0 : N_TEST_CLIENTS-1][0 : N_BUCKETS-1] test_match_q;
    logic [0 : N_TEST_CLIENTS-1] inflight_test_match_q;

    always_ff @(posedge clk)
    begin
        for (int c = 0; c < N_TEST_CLIENTS; c = c + 1)
        begin
            T2_test_notPresent[c] <= (! test_en_q[c] ||
                                      // Does any valid entry match?
                                      (! (|(valid_q & test_match_q[c])) &&
                                       // Does an in-flight entry match?
                                       ! (inflight_test_match_q[c])));
        end

        test_en_q <= test_en;
        valid_q <= valid;
        test_match_q <= test_match;
        inflight_test_match_q <= inflight_test_match;
    end


    //
    // Insert new entries
    //
    always_ff @(posedge clk)
    begin
        insert_idx_q <= insert_idx;
        insert_value_q <= insert_value;
        insert_en_q <= insert_en;
    end

    always_ff @(posedge clk)
    begin
        // Insert new entry
        if (insert_en_q)
        begin
            values[insert_idx_q] <= insert_value_q;
            valid[insert_idx_q] <= 1'b1;
        end

        // Remove old entries
        if (remove_en)
        begin
            valid[remove_idx] <= 1'b0;
        end

        if (reset)
        begin
            valid <= N_BUCKETS'(0);
        end
    end

endmodule // cci_mpf_prim_filter_cam
