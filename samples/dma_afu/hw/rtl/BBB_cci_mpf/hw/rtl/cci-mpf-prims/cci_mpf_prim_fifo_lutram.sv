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
// FIFO --
//   A FIFO with N_ENTRIES storage elements and signaling almostFull when
//   THRESHOLD or fewer slots are free.
//

module cci_mpf_prim_fifo_lutram
  #(
    parameter N_DATA_BITS = 32,
    parameter N_ENTRIES = 2,
    parameter THRESHOLD = 1,

    // Register output if non-zero
    parameter REGISTER_OUTPUT = 0,
    // Bypass to register when FIFO and register are empty?  This parameter
    // has meaning only when REGISTER_OUTPUT is set.
    parameter BYPASS_TO_REGISTER = 0
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [N_DATA_BITS-1 : 0] enq_data,
    input  logic enq_en,
    output logic notFull,
    output logic almostFull,

    output logic [N_DATA_BITS-1 : 0] first,
    input  logic deq_en,
    output logic notEmpty
    );

    logic fifo_deq;
    logic fifo_notEmpty;
    logic [N_DATA_BITS-1 : 0] fifo_first;

    logic enq_bypass_en;

    cci_mpf_prim_fifo_lutram_base
      #(
        .N_DATA_BITS(N_DATA_BITS),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD)
        )
      fifo
       (
        .clk,
        .reset,
        .enq_data,
        .enq_en(enq_en && ! enq_bypass_en),
        .notFull,
        .almostFull,
        .first(fifo_first),
        .deq_en(fifo_deq),
        .notEmpty(fifo_notEmpty)
        );

    generate
        if (REGISTER_OUTPUT == 0)
        begin : nr
            assign first = fifo_first;
            assign notEmpty = fifo_notEmpty;
            assign fifo_deq = deq_en;
            assign enq_bypass_en = 1'b0;
        end
        else
        begin : r
            //
            // Add an output register stage.
            //
            logic first_reg_valid;
            logic [N_DATA_BITS-1 : 0] first_reg;

            assign first = first_reg;
            assign notEmpty = first_reg_valid;
            assign fifo_deq = fifo_notEmpty && (deq_en || ! first_reg_valid);

            // Bypass to outbound register it will be empty and the FIFO
            // is empty.
            assign enq_bypass_en = (BYPASS_TO_REGISTER != 0) &&
                                   (deq_en || ! first_reg_valid) &&
                                   ! fifo_notEmpty;

            always_ff @(posedge clk)
            begin
                if (enq_bypass_en)
                begin
                    // Bypass around FIFO when the FIFO is empty and
                    // BYPASS_TO_REGISTER is enabled.
                    first_reg_valid <= enq_en;
                    first_reg <= enq_data;
                end
                else if (deq_en || ! first_reg_valid)
                begin
                    first_reg_valid <= fifo_notEmpty;
                    first_reg <= fifo_first;
                end

                if (reset)
                begin
                    first_reg_valid <= 1'b0;
                end
            end
        end
    endgenerate

endmodule // cci_mpf_prim_fifo_lutram


//
// Base implementation of the LUTRAM FIFO, used by the main wrapper above.
//
module cci_mpf_prim_fifo_lutram_base
  #(
    parameter N_DATA_BITS = 32,
    parameter N_ENTRIES = 2,
    parameter THRESHOLD = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [N_DATA_BITS-1 : 0] enq_data,
    input  logic enq_en,
    output logic notFull,
    output logic almostFull,

    output logic [N_DATA_BITS-1 : 0] first,
    input  logic deq_en,
    output logic notEmpty
    );
     
    // Pointer to head/tail in storage
    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;

    t_idx enq_idx;
    t_idx first_idx;

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS)
        )
      data
       (
        .clk,
        .reset,

        .raddr(first_idx),
        .rdata(first),

        // Write the data as long as the FIFO isn't full.  This leaves the
        // data path independent of control.  notFull/notEmpty will track
        // the control messages.
        .waddr(enq_idx),
        .wen(notFull),
        .wdata(enq_data)
        );

    cci_mpf_prim_fifo_lutram_ctrl
      #(
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD)
        )
      ctrl
       (
        .clk,
        .reset,
        .enq_idx,
        .enq_en,
        .notFull,
        .almostFull,
        .first_idx,
        .deq_en,
        .notEmpty
        );

endmodule // cci_mpf_prim_fifo_lutram_base


//
// Control logic for FIFOs
//
module cci_mpf_prim_fifo_lutram_ctrl
  #(
    parameter N_ENTRIES = 2,
    parameter THRESHOLD = 1
    )
   (
    input  logic clk,
    input  logic reset,

    output logic [$clog2(N_ENTRIES)-1 : 0] enq_idx,
    input  logic enq_en,
    output logic notFull,
    output logic almostFull,

    output logic [$clog2(N_ENTRIES)-1 : 0] first_idx,
    input  logic deq_en,
    output logic notEmpty
    );

    // Pointer to head/tail in storage
    typedef logic [$clog2(N_ENTRIES)-1 : 0] t_idx;
    // Counter of active entries, leaving room to represent both 0 and N_ENTRIES.
    typedef logic [$clog2(N_ENTRIES+1)-1 : 0] t_counter;

    t_counter valid_cnt;
    t_counter valid_cnt_next;

    // Write pointer advances on ENQ
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            enq_idx <= 1'b0;
        end
        else if (enq_en)
        begin
            enq_idx <= (enq_idx == t_idx'(N_ENTRIES-1)) ? 0 : enq_idx + 1;

            assert (notFull) else
                $fatal("cci_mpf_prim_fifo_lutram: ENQ to full FIFO!");
        end
    end

    // Read pointer advances on DEQ
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            first_idx <= 1'b0;
        end
        else if (deq_en)
        begin
            first_idx <= (first_idx == t_idx'(N_ENTRIES-1)) ? 0 : first_idx + 1;

            assert (notEmpty) else
                $fatal("cci_mpf_prim_fifo_lutram: DEQ from empty FIFO!");
        end
    end

    // Update count of live values
    always_ff @(posedge clk)
    begin
        valid_cnt <= valid_cnt_next;
        notFull <= (valid_cnt_next != t_counter'(N_ENTRIES));
        almostFull <= (valid_cnt_next >= t_counter'(N_ENTRIES - THRESHOLD));
        notEmpty <= (valid_cnt_next != t_counter'(0));

        if (reset)
        begin
            valid_cnt <= t_counter'(0);
            notEmpty <= 1'b0;
        end
    end

    always_comb
    begin
        valid_cnt_next = valid_cnt;

        if (deq_en && ! enq_en)
        begin
            valid_cnt_next = valid_cnt_next - t_counter'(1);
        end
        else if (enq_en && ! deq_en)
        begin
            valid_cnt_next = valid_cnt_next + t_counter'(1);
        end
    end

endmodule // cci_mpf_prim_fifo_lutram_ctrl
