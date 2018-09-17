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
`include "cci_test_csrs.vh"


module test_afu
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // CSR connections
    test_csrs.test csrs,

    input  logic c0NotEmpty,
    input  logic c1NotEmpty
    );

    logic reset;
    assign reset = fiu.reset;

    //
    // Simple error injector shim module, used for testing the error detecting
    // logic.  Normally it just passes messages through.  Writing to test
    // CSR 7 sets the error injection rate.
    //

    typedef logic [39 : 0] t_counter;

    logic wr_error_enable;
    logic rd_error_enable;
    logic error_en;
    t_counter error_interval;
    t_counter error_trigger_cnt;

    always_ff @(posedge clk)
    begin
        // Trigger an error when the trigger counter reaches 0
        error_en <= (error_trigger_cnt == t_counter'(1));

        // Decrement the trigger counter
        error_trigger_cnt <= (error_en ? error_interval :
                                         error_trigger_cnt - t_counter'(1));

        // CPU requests errors?
        if (csrs.cpu_wr_csrs[7].en)
        begin
            // Trigger either read or write errors -- not both
            wr_error_enable <= csrs.cpu_wr_csrs[7].data[0];
            rd_error_enable <= ~ csrs.cpu_wr_csrs[7].data[0];

            // Frequency
            error_interval <= csrs.cpu_wr_csrs[7].data[1 +: $bits(t_counter)];
            error_trigger_cnt <= csrs.cpu_wr_csrs[7].data[1 +: $bits(t_counter)];
        end

        if (reset)
        begin
            wr_error_enable <= 1'b0;
            rd_error_enable <= 1'b0;
            error_en <= 1'b0;
            error_interval <= t_counter'(0);
            error_trigger_cnt <= t_counter'(0);
        end
    end

    cci_mpf_if afu(.clk(clk));

    always_comb
    begin
        afu.reset = fiu.reset;

        fiu.c0Tx = afu.c0Tx;
        fiu.c1Tx = afu.c1Tx;
        fiu.c2Tx = afu.c2Tx;

        afu.c0Rx = fiu.c0Rx;
        afu.c1Rx = fiu.c1Rx;
        afu.c0TxAlmFull = fiu.c0TxAlmFull;
        afu.c1TxAlmFull = fiu.c1TxAlmFull;

        //
        // Inject errors into locations expected to be 0.
        //

        if (error_en && wr_error_enable)
        begin
            fiu.c1Tx.data[140] = 1'b1;
        end

        if (error_en && rd_error_enable)
        begin
            afu.c0Rx.data[140] = 1'b1;
        end
    end

    main_afu main_afu
       (
        .clk,
        .fiu(afu),
        .csrs,
        .c0NotEmpty,
        .c1NotEmpty
        );

endmodule // test_afu


module main_afu
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // CSR connections
    test_csrs.test csrs,

    input  logic c0NotEmpty,
    input  logic c1NotEmpty
    );

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end


    // ====================================================================
    //
    //  Global types and signals
    //
    // ====================================================================

    //
    // State machine
    //
    typedef enum logic [1:0]
    {
        STATE_IDLE,
        STATE_RUN,
        STATE_TERMINATE
    }
    t_state;

    t_state state;

    //
    // Array of 2MB buffers
    //
    localparam N_TEST_BUFFERS = 8;
    typedef logic [$clog2(N_TEST_BUFFERS)-1 : 0] t_buffer_idx;

    t_cci_clAddr mem[0 : N_TEST_BUFFERS-1];


    // Offset into a 2MB buffer
    typedef logic [14:0] t_buffer_offset;

    // Offset into the 4KB DSM buffer
    typedef logic [5:0] t_dsm_offset;

    // Base of a 2MB buffer, excluding the offset.  Buffers must be aligned
    // to 2MB boundaries.
    typedef logic [$bits(t_cci_clAddr)-$bits(t_buffer_offset)-1 : 0] t_buffer_base;

    // Extract the 2MB buffer base from an address.  Buffers are aligned
    // to 2MB.
    function automatic t_buffer_base bufBase(t_cci_clAddr addr);
        t_buffer_base base;
        t_buffer_offset offset;

        {base, offset} = addr;
        return base;
    endfunction


    // ====================================================================
    //
    //   Almost full tracker
    //
    // ====================================================================

    // An AFU may continue to send up to CCI_TX_ALMOST_FULL_THRESHOLD
    // requests after the almost full signal is raised.  Allow starting
    // new writes for up to half the maximum.
    localparam C1TX_ALMOST_FULL_THRESHOLD = (CCI_TX_ALMOST_FULL_THRESHOLD >> 1);

    logic c0TxAlmFull;
    assign c0TxAlmFull = fiu.c0TxAlmFull;

    logic n_c1TxAlmFull_vec[1 : C1TX_ALMOST_FULL_THRESHOLD-1];
    logic c1TxAlmFull;

    assign c1TxAlmFull = (~n_c1TxAlmFull_vec[1] && fiu.c1TxAlmFull) || reset;

    always_ff @(posedge clk)
    begin
        n_c1TxAlmFull_vec[C1TX_ALMOST_FULL_THRESHOLD-1] <= ~fiu.c1TxAlmFull;
        n_c1TxAlmFull_vec[1 : C1TX_ALMOST_FULL_THRESHOLD-2] <=
            n_c1TxAlmFull_vec[2 : C1TX_ALMOST_FULL_THRESHOLD-1];

        if (reset)
        begin
            for (int i = 1; i < C1TX_ALMOST_FULL_THRESHOLD; i = i + 1)
            begin
                n_c1TxAlmFull_vec[i] <= 1'b0;
            end
        end
    end


    // ====================================================================
    //
    //  CSRs (simple connections to the external CSR management engine)
    //
    // ====================================================================

    typedef logic [39 : 0] t_counter;

    t_cci_clAddr dsm;
    t_cci_vc dsm_vc;

    //
    // Read CSR from host
    //
    t_counter cnt_errors;
    t_counter cnt_rd_rsp;
    t_counter cnt_wr_rsp;

    // Return these through a CSR in order to preserve the entire response,
    // making the dependence on CCI more realistic.
    logic c0Rx_xor;
    logic c1Rx_xor;

    logic [63:0] csr_state;
    always_ff @(posedge clk)
    begin
        csr_state <= { 48'(0),
                       8'(state),
                       4'(0),
                       c1NotEmpty,
                       c0NotEmpty,
                       fiu.c1TxAlmFull,
                       fiu.c0TxAlmFull };
    end

    always_comb
    begin
        csrs.afu_id = 128'hbfd75b03_9608_4e82_ae22_f61a62b8f992;

        // Default
        for (int i = 0; i < NUM_TEST_CSRS; i = i + 1)
        begin
            csrs.cpu_rd_csrs[i].data = 64'(0);
        end

        // CSR 0 returns the number of buffers to use for the test.
        csrs.cpu_rd_csrs[0].data = 64'(N_TEST_BUFFERS);

        csrs.cpu_rd_csrs[1].data = 64'(dsm);

        // Total errors
        csrs.cpu_rd_csrs[3].data = 64'(cnt_errors);

        // Number of read responses
        csrs.cpu_rd_csrs[4].data = 64'(cnt_rd_rsp);

        // Number of completed writes
        csrs.cpu_rd_csrs[5].data = 64'(cnt_wr_rsp);

        // Various state
        csrs.cpu_rd_csrs[7].data = csr_state;
    end

    //
    // Incoming configuration
    //
    t_counter cycles_rem;
    t_counter cycles_actual;

    logic cl_beats_variable;
    t_cci_clNum cl_beats;

    logic rdline_mode_s;
    logic [1:0] wrline_req_type;

    // Encoding of VC mapping requests in 3 bits.  When the high bit is set
    // the mapping mode is round-robin among either PCIe (bit 0 == 0) or
    // among all channels (bit 0 == 1).
    logic [2:0] wr_vc_map;
    logic [2:0] rd_vc_map;


    //
    // Track buffer pointer initialization index, which is updated every
    // time a new write is observed to CSR 2.
    //
    t_buffer_idx buf_init_idx;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            buf_init_idx <= t_buffer_idx'(0);
        end
        else
        begin
            buf_init_idx <= buf_init_idx + t_buffer_idx'(csrs.cpu_wr_csrs[2].en);
        end
    end


    //
    // Consume configuration CSR writes
    //
    always_ff @(posedge clk)
    begin
        if (csrs.cpu_wr_csrs[1].en)
        begin
            dsm <= csrs.cpu_wr_csrs[1].data;
            if (! reset) $display("DSM: 0x%x", csrs.cpu_wr_csrs[1].data);
        end

        if (csrs.cpu_wr_csrs[2].en)
        begin
            mem[buf_init_idx] <= csrs.cpu_wr_csrs[2].data;
            if (! reset) $display("MEM[%0d]: 0x%x", buf_init_idx, csrs.cpu_wr_csrs[2].data);
        end
    end


    //
    // Configure the simulation
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            cycles_rem <= t_counter'(0);
            dsm_vc <= eVC_VH0;
            cl_beats_variable <= 1'b0;
            cl_beats <= t_cci_clLen'(0);
            wrline_req_type <= 2'b0;
            rdline_mode_s <= 1'b0;
            wr_vc_map <= 1'b0;
            rd_vc_map <= 1'b0;
        end
        else
        begin
            // Normal case: decrement cycle counter
            if (cycles_rem != t_counter'(0))
            begin
                cycles_rem <= cycles_rem - t_counter'(1);
            end

            // Execution cycle count update from the host?
            if (csrs.cpu_wr_csrs[0].en)
            begin
                { cycles_rem,           // t_counter 40 (14)
                  dsm_vc,               // 2 (12)
                  cl_beats_variable,    // 1 (11)
                  cl_beats,             // 2 (9)
                  wrline_req_type,      // 2 (7)
                  rdline_mode_s,        // 1 (6)
                  wr_vc_map,            // 3 (3)
                  rd_vc_map             // 3
                  } <= csrs.cpu_wr_csrs[0].data;
            end
        end
    end


    logic start_new_run;

    // Signals from the write engine's decisions
    logic wr_ctrl_term_en;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            start_new_run <= 1'b0;
            state <= STATE_IDLE;
        end
        else
        begin
            start_new_run <= 1'b0;

            case (state)
              STATE_IDLE:
                begin
                    // New run requested
                    if (csrs.cpu_wr_csrs[0].en)
                    begin
                        state <= STATE_RUN;
                        start_new_run <= 1'b1;
                        $display("Starting test...");
                    end
                end

              STATE_RUN:
                begin
                    // Finished ?
                    if (cycles_rem == t_counter'(0))
                    begin
                        state <= STATE_TERMINATE;
                        $display("Ending test...");
                    end
                end

              default:
                begin
                    // Various signalling states terminate when a write is allowed
                    if (wr_ctrl_term_en)
                    begin
                        state <= STATE_IDLE;
                        $display("Test done.");
                    end
                end
            endcase
        end
    end


    //
    // Count actual cycles executed.  This will be at least the number of
    // cycles requested plus whatever time is needed to finish writing to
    // whatever 2MB buffer is active when time runs out.
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            cycles_actual <= t_counter'(0);
        end
        else if (start_new_run)
        begin
            cycles_actual <= t_counter'(0);
        end
        else if (state != STATE_IDLE)
        begin
            cycles_actual <= cycles_actual + t_counter'(1);
        end
    end


    //
    // Pick a read/write group length given an address and the requested
    // configuration.
    //
    function automatic t_cci_clLen reqLenFromAddr(t_cci_clAddr addr);
        t_cci_clLen len;

        if (! cl_beats_variable)
        begin
            // Not variable length.  Return the requested constant length.
            len = t_cci_clLen'(cl_beats);
        end
        else
        begin
            // Emit requests in a pattern: 1, 1, 2, 4.  The pattern tiles
            // to the required alignments.
            casez (addr[2:0])
                3'b00?:  len = eCL_LEN_1;
                3'b01?:  len = eCL_LEN_2;
                default: len = eCL_LEN_4;
            endcase
        end

        return len;
    endfunction


    // ====================================================================
    //
    //   Write engine
    //
    //     The write engine goes through all active buffers round robin,
    //     writing to each location exactly once on each trip.  A random
    //     number is chosen for a given pass through a buffer so that
    //     each write has a unique tag, making it possible to detect
    //     dropped writes.
    //
    // ====================================================================

    t_buffer_base wr_buf_base;
    t_buffer_idx wr_buf_next_idx;
    t_buffer_offset wr_buf_offset;
    // Finished writing all lines in a buffer
    logic wr_buf_done;

    // Next line to write in DSM.  DSM writes sweep through the buffer in
    // order to send error reports.
    t_dsm_offset wr_dsm_offset;

    // Normal write address is the combination of the buffer base and offset.
    t_cci_clAddr wr_addr;
    assign wr_addr = { wr_buf_base, wr_buf_offset };

    t_cci_clLen wr_len, wr_len_next;
    assign wr_len_next = reqLenFromAddr(wr_addr);

    // Record a random tag that is written to each line in a buffer
    typedef logic [11:0] t_random_tag;
    t_random_tag mem_tag_lfsr;
    t_random_tag mem_tag_random[0 : N_TEST_BUFFERS-1];
    t_random_tag wr_tag_random;

    logic wr_ena;
    assign wr_ena = ! c1TxAlmFull && (state == STATE_RUN);

    //
    // These error signals are set by the read logic when an incorrect
    // response is received.  It will trigger a write to DSM.
    //
    // The protocol here may fail to report all errors when a bunch
    // happen together.  We assume this will be rare and that a more
    // complicated protocol isn't required.  The error_count maintained
    // above will always be correct, so we can detect reporting failures.
    //
    logic error_request;
    t_buffer_idx error_buf_idx;
    t_buffer_offset error_buf_offset;
    t_random_tag error_tag;
    // Set in write arbitration when an error request is processed
    logic wr_dsm_error;

    //
    // Write request header.
    //
    t_cci_mpf_c1_ReqMemHdr wr_hdr;
    t_cci_vc wr_vc_prev;

    always_comb
    begin
        wr_hdr = cci_mpf_c1_genReqHdr(t_cci_c1_req'(wrline_req_type),
                                      wr_addr,
                                      t_cci_mdata'(0),
                                      cci_mpf_defaultReqHdrParams());

        //
        // Pick a VC for the write
        //
        wr_hdr.base.vc_sel = t_cci_vc'(wr_vc_map[1:0]);

        // Round robin mapping requested?
        if (wr_vc_map[2])
        begin
            if (! wr_vc_map[0])
            begin
                // Just PCIe mapping.  Use an address based mapping to
                // avoid harmonics.
                wr_hdr.base.vc_sel = t_cci_vc'({1'b1, wr_addr[3]});
            end
            else
            begin
                // All channels, round robin
                case (wr_vc_prev)
                    eVC_VL0: wr_hdr.base.vc_sel = eVC_VH0;
                    eVC_VH0: wr_hdr.base.vc_sel = eVC_VH1;
                    eVC_VH1: wr_hdr.base.vc_sel = eVC_VL0;
                    default: wr_hdr.base.vc_sel = eVC_VL0;
                endcase
            end
        end


        // SOP if this is the first beat in a write group
        wr_hdr.base.sop = ~(|(wr_len & wr_addr[1:0]));
        wr_hdr.base.cl_len = wr_len_next;
    end

    //
    // Write data.
    //
    t_cci_clData wr_data;
    always_comb
    begin
        // Mostly write 0
        wr_data = t_cci_clData'(0);

        // Put the buffer offset in the high bits
        wr_data[$left(wr_data)-1 -: $bits(wr_buf_offset)] = wr_buf_offset;

        // Put the random tag for this pass through the buffer in the low
        // bits.  This will prove that the write was actually completed.
        wr_data[0 +: $bits(t_random_tag)] = wr_tag_random;
    end


    //
    // Write control.
    //
    typedef enum logic [1:0]
    {
        WR_STATE_IDLE,
        WR_STATE_NORMAL,
        WR_STATE_NEXT_BUF,
        WR_STATE_DSM_EN
    }
    t_wr_state;

    t_wr_state wr_state;

    //
    // Termination logic.  First a full buffer must be completed.  Terminate
    // on the next cycle.
    //
    logic prepare_to_term;
    assign prepare_to_term = (wr_state == WR_STATE_NEXT_BUF) &&
                             (state == STATE_TERMINATE);

    always_comb
    begin
        wr_dsm_error = 1'b0;

        if (start_new_run || (state == STATE_IDLE))
        begin
            wr_state = WR_STATE_IDLE;
        end
        else if (wr_buf_done)
        begin
            // Just finished a buffer.  Flush it and prepare for another.
            wr_state = WR_STATE_NEXT_BUF;
        end
        else if (wr_ctrl_term_en)
        begin
            // Termination requested and a buffer write was just completed.
            wr_state = WR_STATE_DSM_EN;
        end
        else if (c1TxAlmFull)
        begin
            wr_state = WR_STATE_IDLE;
        end
        else if ((state == STATE_RUN) && error_request && wr_hdr.base.sop)
        begin
            // Error!  Notify the CPU.
            wr_state = WR_STATE_DSM_EN;
            wr_dsm_error = 1'b1;
        end
        else
        begin
            // Normal writes
            wr_state = WR_STATE_NORMAL;
        end
    end

    always_ff @(posedge clk)
    begin
        wr_ctrl_term_en <= prepare_to_term;

        // Set up a typical write.
        fiu.c1Tx <= cci_mpf_genC1TxWriteReq(wr_hdr, wr_data,
                                            (wr_state != WR_STATE_IDLE));

        if (start_new_run)
        begin
            // Starting a new run.  Writes begin at in the middle buffer.
            // Reads will start in the first buffer.  Reads and writes are
            // never active in the same buffer to avoid requiring flow control.
            wr_buf_base <= bufBase(mem[N_TEST_BUFFERS >> 1]);
            wr_buf_next_idx <= t_buffer_idx'(1 + (N_TEST_BUFFERS >> 1));
            wr_buf_offset <= t_buffer_offset'(0);
            wr_buf_done <= 1'b0;
            wr_dsm_offset <= t_dsm_offset'(0);

            wr_len <= t_cci_clLen'(cl_beats);

            // Set the random tag for this pass through the buffer
            mem_tag_random[N_TEST_BUFFERS >> 1] <= mem_tag_lfsr;
            wr_tag_random <= mem_tag_lfsr;
        end
        else if (wr_state == WR_STATE_NORMAL)
        begin
            // Normal write to current buffer.
            wr_buf_offset <= wr_buf_offset + t_buffer_offset'(1);
            wr_buf_done <= &(wr_buf_offset);

            // Pick a write length.  This is either a fixed length for all writes
            // or a variety of lengths in a pattern that is a function of the
            // address.
            wr_len <= wr_len_next;
        end
        else if (wr_state == WR_STATE_NEXT_BUF)
        begin
            // Wrote entire buffer.  Time to switch to the next buffer.
            wr_buf_base <= bufBase(mem[wr_buf_next_idx]);
            wr_buf_next_idx <= wr_buf_next_idx + t_buffer_idx'(1);
            wr_buf_offset <= 0;
            wr_buf_done <= 1'b0;
            wr_len <= t_cci_clLen'(cl_beats);

            // Pick a new random tag for the next buffer.
            wr_tag_random <= mem_tag_lfsr;
            if (! prepare_to_term)
            begin
                mem_tag_random[wr_buf_next_idx] <= mem_tag_lfsr;
            end

            // Emit a fence
            fiu.c1Tx.hdr.base.req_type <= eREQ_WRFENCE;
            fiu.c1Tx.hdr.base.vc_sel <= eVC_VA;
            fiu.c1Tx.hdr.base.cl_len <= eCL_LEN_1;
            fiu.c1Tx.hdr.base.sop <= 1'b0;
        end
        else if (wr_state == WR_STATE_DSM_EN)
        begin
            // Either report an error or note run completion.  Both are done
            // by walking round-robin through the DSM buffer.
            fiu.c1Tx.valid <= 1'b1;
            fiu.c1Tx.hdr.base.req_type <= eREQ_WRLINE_I;
            fiu.c1Tx.hdr.base.address <= { dsm[$left(dsm) : $bits(t_dsm_offset)],
                                           wr_dsm_offset };
            fiu.c1Tx.hdr.base.sop <= 1'b1;
            fiu.c1Tx.hdr.base.cl_len <= eCL_LEN_1;
            fiu.c1Tx.hdr.base.vc_sel <= dsm_vc;

            // Use separate fields for error and run completion to avoid
            // a MUX.  Bit 1 signals the message type.
            fiu.c1Tx.data[127:64] <= 64'(cycles_actual);
            fiu.c1Tx.data[47:32] <= 16'(error_tag);
            fiu.c1Tx.data[31:16] <= 16'(error_buf_offset);
            fiu.c1Tx.data[15:8] <= 8'(error_buf_idx);
            fiu.c1Tx.data[7:0] <= { 6'(state), wr_dsm_error, 1'b1 };

            wr_dsm_offset <= wr_dsm_offset + t_dsm_offset'(1);
        end

        if (reset)
        begin
            wr_len <= eCL_LEN_1;
            wr_buf_offset <= t_buffer_offset'(0);
            wr_dsm_offset <= t_dsm_offset'(0);

            for (int i = 0; i < N_TEST_BUFFERS; i = i + 1)
            begin
                mem_tag_random[i] <= t_random_tag'(0);
            end
        end
    end

    // Record the VC used this cycle for writes
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            wr_vc_prev <= eVC_VA;
        end
        else if ((wr_state == WR_STATE_NORMAL) &&
                 // Last beat of multi-line write?
                 (&(~wr_hdr.base.cl_len | wr_addr[1:0])))
        begin
            wr_vc_prev <= wr_hdr.base.vc_sel;
        end
    end


    logic c1Rx_is_write_rsp;
    t_cci_clNum c1Rx_cl_num;

    always_ff @(posedge clk)
    begin
        c1Rx_is_write_rsp <= cci_c1Rx_isWriteRsp(fiu.c1Rx);
        c1Rx_cl_num <= (fiu.c1Rx.hdr.format ? fiu.c1Rx.hdr.cl_num : t_cci_clNum'(0));

        if (c1Rx_is_write_rsp)
        begin
            // Count beats so multi-line writes get credit for all data
            cnt_wr_rsp <= cnt_wr_rsp + t_counter'(1) + t_counter'(c1Rx_cl_num);
        end

        if (reset || start_new_run)
        begin
            cnt_wr_rsp <= t_counter'(0);
            c1Rx_is_write_rsp <= 1'b0;
        end
    end


    //
    // LFSR generates random data for writes
    //
    cci_mpf_prim_lfsr12
      #(
        .INITIAL_VALUE(12'(15))
        )
      lfsr
       (
        .clk,
        .reset,
        // Update when the writer moves to a new 2MB buffer
        .en(start_new_run || wr_buf_done),
        .value(mem_tag_lfsr)
        );


    // ====================================================================
    //
    //   Read engine
    //
    //     The read engine trails the write engine above, looping
    //     repeatedly through the buffer half way around the collection
    //     of buffers relative to the write engine.  The read engine
    //     switches to a new buffer whenever the write engine switches.
    //     No attempt is made to guarantee that all writes have been
    //     committed to system memory (by tracking write ACKs).  We
    //     assume that writes commit before reads since multiple MBs
    //     of writes complete between a write and a read of the same
    //     line.  A write fence is emitted at the end of every buffer,
    //     forcing some write ordering.
    //
    // ====================================================================

    t_buffer_base rd_buf_base;
    t_buffer_idx rd_buf_next_idx;
    t_buffer_offset rd_buf_offset;

    // Normal read address is the combination of the buffer base and offset.
    t_cci_clAddr rd_addr;
    assign rd_addr = { rd_buf_base, rd_buf_offset };

    t_cci_clLen rd_len;
    assign rd_len = reqLenFromAddr(rd_addr);

    // The expected read tag for the current buffer and the previous one.
    // There is no ROB to sort responses, but we assume that with 2MB buffers
    // that all reads retire by the time a third buffer is reached.
    t_random_tag rd_tag_expected[0:1];
    t_buffer_idx rd_buf_active_idx[0:1];

    // Index of rd_tag_expected for current buffer
    logic rd_tag_idx;
    assign rd_tag_idx = ~rd_buf_next_idx[0];

    //
    // Read request header.
    //
    t_cci_mpf_c0_ReqMemHdr rd_hdr;
    t_cci_vc rd_vc_prev;

    always_comb
    begin
        rd_hdr = cci_mpf_c0_genReqHdr(rdline_mode_s ? eREQ_RDLINE_S : eREQ_RDLINE_I,
                                      rd_addr,
                                      // mdata holds the index of rd_tag_expected and
                                      // the buffer offset.  This is enough to compute
                                      // the expected read value.
                                      t_cci_mdata'({rd_tag_idx, rd_buf_offset}),
                                      cci_mpf_defaultReqHdrParams());

        //
        // Pick a VC for the read
        //
        rd_hdr.base.vc_sel = t_cci_vc'(rd_vc_map[1:0]);

        // Round robin mapping requested?
        if (rd_vc_map[2])
        begin
            if (! rd_vc_map[0])
            begin
                // Just PCIe mapping.  Use an address based mapping to
                // avoid harmonics.
                rd_hdr.base.vc_sel = t_cci_vc'({1'b1, rd_addr[3]});
            end
            else
            begin
                // All channels
                case (rd_vc_prev)
                    eVC_VL0: rd_hdr.base.vc_sel = eVC_VH0;
                    eVC_VH0: rd_hdr.base.vc_sel = eVC_VH1;
                    eVC_VH1: rd_hdr.base.vc_sel = eVC_VL0;
                    default: rd_hdr.base.vc_sel = eVC_VL0;
                endcase
            end
        end

        rd_hdr.base.cl_len = rd_len;
    end

    logic rd_en;
    assign rd_en = ! c0TxAlmFull && ! start_new_run && (state == STATE_RUN);

    always_ff @(posedge clk)
    begin
        // Set up a typical read
        fiu.c0Tx <= cci_mpf_genC0TxReadReq(rd_hdr, rd_en);

        if (start_new_run)
        begin
            // Starting a new run.  Reads from the first buffer.  Even for
            // the first pass, the CPU has initialized the memory.
            rd_buf_base <= bufBase(mem[0]);
            rd_buf_next_idx <= t_buffer_idx'(1);
            rd_buf_offset <= t_buffer_offset'(0);

            // Set the random tag for this pass through the buffer
            rd_tag_expected[0] <= mem_tag_random[0];
            rd_buf_active_idx[0] <= t_buffer_idx'(0);
        end
        else if (wr_state == WR_STATE_NEXT_BUF)
        begin
            // Write process moved to next buffer.  Move the read to its
            // next buffer, too.
            rd_buf_base <= bufBase(mem[rd_buf_next_idx]);
            rd_buf_next_idx <= rd_buf_next_idx + t_buffer_idx'(1);
            rd_buf_offset <= t_buffer_offset'(0);

            // Set the random tag for this pass through the buffer
            rd_tag_expected[rd_buf_next_idx[0]] <= mem_tag_random[rd_buf_next_idx];
            rd_buf_active_idx[rd_buf_next_idx[0]] <= rd_buf_next_idx;
        end
        else if (! c0TxAlmFull)
        begin
            // Normal case.  Move to next offset based on requested length.
            rd_buf_offset <= rd_buf_offset +
                             t_buffer_offset'(3'(1) + 3'(rd_len));
        end
    end

    // Record the VC used this cycle for reads
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            rd_vc_prev <= eVC_VA;
        end
        else if (rd_en)
        begin
            rd_vc_prev <= rd_hdr.base.vc_sel;
        end
    end

    logic c0Rx_is_read_rsp;

    always_ff @(posedge clk)
    begin
        c0Rx_is_read_rsp <= cci_c0Rx_isReadRsp(fiu.c0Rx);
        if (c0Rx_is_read_rsp)
        begin
            cnt_rd_rsp <= cnt_rd_rsp + t_counter'(1);
        end

        if (reset || start_new_run)
        begin
            cnt_rd_rsp <= t_counter'(0);
            c0Rx_is_read_rsp <= 1'b0;
        end
    end


    //
    // Read response, value check pipeline.
    //
    
    logic rd_rsp_valid_q;
    logic [15:0] rd_word_cmp_q;
    t_buffer_idx rd_rsp_buf_idx_q;
    t_random_tag rd_rsp_tag_q;

    t_buffer_offset rd_rsp_offset, rd_rsp_offset_q;
    always_comb
    begin
        rd_rsp_offset = t_buffer_offset'(fiu.c0Rx.hdr.mdata);
        rd_rsp_offset[1:0] = rd_rsp_offset[1:0] | fiu.c0Rx.hdr.cl_num;
    end

    // First stage: reduce to 16 bits
    always_ff @(posedge clk)
    begin
        rd_rsp_valid_q <= cci_c0Rx_isReadRsp(fiu.c0Rx);
        rd_rsp_offset_q <= rd_rsp_offset;

        // High word holds the offset of the line in the buffer.  The offset is
        // also stored in mdata[0:14].
        rd_word_cmp_q[15] <= (|({ fiu.c0Rx.data[511],
                                  (fiu.c0Rx.data[510:496] ^ rd_rsp_offset),
                                  fiu.c0Rx.data[495:480] }));

        // Low word holds a random number that is the same for every entry
        // in the buffer for a given write pass.  There are two buffer IDs
        // active at any given time.  mdata[15] indicates which ID to expected.
        // We assume that with 2MB per buffer it is impossible to have reads
        // in flight for 3 buffers simultaneously but have no mechanism to
        // enforce this.  It is unlikely any BBS would ever have buffers
        // so large to make this a problem.
        rd_word_cmp_q[0] <= (|({ fiu.c0Rx.data[31 : $bits(t_random_tag)],
                                 (fiu.c0Rx.data[$bits(t_random_tag)-1 : 0] ^
                                  rd_tag_expected[fiu.c0Rx.hdr.mdata[15]]) }));

        // Middle words are all expected to be 32'b0
        for (int i = 1; i <= 14; i = i + 1)
        begin
            rd_word_cmp_q[i] <= (|(fiu.c0Rx.data[i*32 +: 32]));
        end

        rd_rsp_buf_idx_q <= rd_buf_active_idx[fiu.c0Rx.hdr.mdata[15]];
        rd_rsp_tag_q <= rd_tag_expected[fiu.c0Rx.hdr.mdata[15]];

        if (reset)
        begin
            rd_rsp_valid_q <= 1'b0;
        end
    end

    // Second stage: Error if any bit of compared state is 1
    logic rd_rsp_error;
    assign rd_rsp_error = rd_rsp_valid_q && (|(rd_word_cmp_q));

    always_ff @(posedge clk)
    begin
        if (rd_rsp_error)
        begin
            $display("ERROR: buf idx %0d, offset 0x%x (tag 0x%x)", rd_rsp_buf_idx_q, rd_rsp_offset_q, rd_rsp_tag_q);

            error_request <= 1'b1;
            error_buf_idx <= rd_rsp_buf_idx_q;
            error_buf_offset <= rd_rsp_offset_q;
            error_tag <= rd_rsp_tag_q;

            cnt_errors <= cnt_errors + t_counter'(1);
        end
        else if (wr_dsm_error)
        begin
            // Previously requested error report has been written to memory
            error_request <= 1'b0;
        end

        if (reset || start_new_run)
        begin
            error_request <= 1'b0;
            error_buf_idx <= t_buffer_idx'(0);
            error_buf_offset <= t_buffer_offset'(0);
            error_tag <= t_random_tag'(0);
            cnt_errors <= t_counter'(0);
        end
    end

endmodule // main_afu
