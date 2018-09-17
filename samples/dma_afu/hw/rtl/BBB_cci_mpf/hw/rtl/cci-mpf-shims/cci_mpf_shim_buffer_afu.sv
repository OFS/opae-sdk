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


//
// This is more a primitive shim than a full fledged shim.  It takes an
// AFU-side raw connection (wires) and adds latency insensitive buffering
// on the read/write request wires.  Most shims will instantiate this
// shim on the AFU-side connections in order to eliminate loops in the
// computation of the AFU-side almost full signals.
//

module cci_mpf_shim_buffer_afu
  #(
    // Assert almost full when the number of free entries falls below threshold.
    parameter THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,

    // Total entries in the buffer.
    parameter N_ENTRIES = THRESHOLD + 4,

    // If nonzero, incoming requests from afu_raw on channel 0 may bypass
    // the FIFO and be received in the same cycle through afu_buf.  This
    // is offered only on channel 0.  Bypassing is not offered on channel
    // 1 both because stores are less latency sensitive and because the
    // cost of a bypass MUX on the line-sized store data is too high.
    parameter ENABLE_C0_BYPASS = 0,

    // Register output with skid buffers on the LUTRAM FIFOs if non-zero.
    parameter REGISTER_OUTPUT = 0
    )
   (
    input  logic clk,

    // Raw unbuffered connection.  This is the AFU-side connection of the
    // parent module.
    cci_mpf_if.to_afu afu_raw,

    // Generated buffered connection.  The confusing interface direction
    // arises because the shim is an interposer on the AFU side of a
    // standard shim.
    cci_mpf_if.to_fiu afu_buf,

    // Dequeue signals combined with the buffering make the buffered interface
    // latency insensitive.  Requests sit in the buffers unless explicitly
    // removed.
    input logic deqC0Tx,
    input logic deqC1Tx
    );

    assign afu_raw.reset = afu_buf.reset;

    //
    // Rx wires pass through toward the AFU.  They are latency sensitive
    // since the CCI provides no back pressure.
    //
    assign afu_raw.c0Rx = afu_buf.c0Rx;
    assign afu_raw.c1Rx = afu_buf.c1Rx;


    // ====================================================================
    //
    // Channel 0 Tx buffer.
    //
    //   The buffer triggers c0TxAlmFull when there are THRESHOLD or fewer
    //   slots available, as required by the CCI specification.  Unlike the
    //   usual CCI request interface, movement through the pipeline is
    //   explicit.  The code that instantiates this buffer must dequeue
    //   the head of the FIFO using deqC0Tx in order to consume a request.
    //
    // ====================================================================

    localparam C0TX_BITS = CCI_MPF_C0TX_MEMHDR_WIDTH;

    t_cci_mpf_c0_ReqMemHdr c0_fifo_first;
    logic c0_fifo_notEmpty;
    logic c0_fifo_enq;
    logic c0_fifo_deq;

    // If the bypass is enabled on channel 0 then route around the FIFO if
    // the FIFO is currently empty and the logic connected to afu_buf consumes
    // a new message from afu_raw immediately.
    generate
        if (ENABLE_C0_BYPASS == 0)
        begin : nb
            // No bypass.  All messages flow through the FIFO.
            assign afu_buf.c0Tx.hdr = c0_fifo_first;
            assign afu_buf.c0Tx.valid = c0_fifo_notEmpty;

            assign c0_fifo_enq = afu_raw.c0Tx.valid;
            assign c0_fifo_deq = deqC0Tx;
        end
        else
        begin : b
            // Bypass FIFO when possible.
            always_comb
            begin
                if (c0_fifo_notEmpty)
                begin
                    afu_buf.c0Tx.hdr = c0_fifo_first;
                    afu_buf.c0Tx.valid = 1'b1;
                end
                else
                begin
                    afu_buf.c0Tx = afu_raw.c0Tx;
                end
            end

            // Enq to the FIFO if a new request has arrived and it wasn't
            // consumed immediately through afu_buf.
            assign c0_fifo_enq =
                afu_raw.c0Tx.valid && (c0_fifo_notEmpty || ! deqC0Tx);

            assign c0_fifo_deq = deqC0Tx && c0_fifo_notEmpty;
        end
    endgenerate

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(C0TX_BITS),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT),
        .BYPASS_TO_REGISTER(ENABLE_C0_BYPASS)
        )
      c0_fifo
       (
        .clk,
        .reset(afu_buf.reset),

        .enq_data(afu_raw.c0Tx.hdr),
        // Map the valid bit through as enq here and notEmpty below.
        .enq_en(c0_fifo_enq),
        .notFull(),
        .almostFull(afu_raw.c0TxAlmFull),

        .first(c0_fifo_first),
        .deq_en(c0_fifo_deq),
        .notEmpty(c0_fifo_notEmpty)
        );


    // ====================================================================
    //
    // Channel 1 Tx buffer.
    //
    //   Same principle as channel 0 above.
    //
    // ====================================================================

    localparam C1TX_BITS = $bits(t_if_cci_mpf_c1_Tx);

    // Request payload exists when one of the valid bits is set.
    logic c1_enq_en;
    assign c1_enq_en = afu_raw.c1Tx.valid;

    logic c1_notEmpty;

    // Pull request details out of the head of the FIFO.
    t_if_cci_mpf_c1_Tx c1_first;

    // Forward the FIFO to the buffered output.  The valid bit is
    // only meaningful when the FIFO isn't empty.
    always_comb
    begin
        afu_buf.c1Tx = c1_first;
        afu_buf.c1Tx.valid = c1_notEmpty;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(C1TX_BITS),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT)
        )
      c1_fifo
       (
        .clk,
        .reset(afu_buf.reset),

        // The concatenated field order must match the use of c1_first above.
        .enq_data(afu_raw.c1Tx),
        .enq_en(c1_enq_en),
        .notFull(),
        .almostFull(afu_raw.c1TxAlmFull),

        .first(c1_first),
        .deq_en(deqC1Tx),
        .notEmpty(c1_notEmpty)
        );


    // ====================================================================
    //
    // Channel 2 Tx (MMIO read response) is unbuffered.
    //
    // ====================================================================

    assign afu_buf.c2Tx = afu_raw.c2Tx;

endmodule // cci_mpf_shim_buffer_afu


//
// Buffer only channel 1 (writes.  Channel 0 (reads) is straight through.
//
module cci_mpf_shim_buffer_afu_c1
  #(
    // Assert almost full when the number of free entries falls below threshold.
    parameter THRESHOLD = CCI_TX_ALMOST_FULL_THRESHOLD,

    // Total entries in the buffer.
    parameter N_ENTRIES = THRESHOLD + 4,

    // Register output with skid buffers on the LUTRAM FIFOs if non-zero.
    parameter REGISTER_OUTPUT = 0
    )
   (
    input  logic clk,

    // Raw unbuffered connection.  This is the AFU-side connection of the
    // parent module.
    cci_mpf_if.to_afu afu_raw,

    // Generated buffered connection.  The confusing interface direction
    // arises because the shim is an interposer on the AFU side of a
    // standard shim.
    cci_mpf_if.to_fiu afu_buf,

    // Dequeue signals combined with the buffering make the buffered interface
    // latency insensitive.  Requests sit in the buffers unless explicitly
    // removed.
    input logic deqC1Tx
    );

    assign afu_raw.reset = afu_buf.reset;

    //
    // Rx wires pass through toward the AFU.  They are latency sensitive
    // since the CCI provides no back pressure.
    //
    assign afu_raw.c0Rx = afu_buf.c0Rx;
    assign afu_raw.c1Rx = afu_buf.c1Rx;


    // ====================================================================
    //
    // Channel 0 Tx flows straight through.
    //
    // ====================================================================

    assign afu_buf.c0Tx = afu_raw.c0Tx;
    assign afu_raw.c0TxAlmFull = afu_buf.c0TxAlmFull;


    // ====================================================================
    //
    // Channel 1 Tx buffer.
    //
    // ====================================================================

    localparam C1TX_BITS = $bits(t_if_cci_mpf_c1_Tx);

    // Request payload exists when one of the valid bits is set.
    logic c1_enq_en;
    assign c1_enq_en = afu_raw.c1Tx.valid;

    logic c1_notEmpty;

    // Pull request details out of the head of the FIFO.
    t_if_cci_mpf_c1_Tx c1_first;

    // Forward the FIFO to the buffered output.  The valid bit is
    // only meaningful when the FIFO isn't empty.
    always_comb
    begin
        afu_buf.c1Tx = c1_first;
        afu_buf.c1Tx.valid = c1_notEmpty;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(C1TX_BITS),
        .N_ENTRIES(N_ENTRIES),
        .THRESHOLD(THRESHOLD),
        .REGISTER_OUTPUT(REGISTER_OUTPUT)
        )
      c1_fifo
       (
        .clk,
        .reset(afu_buf.reset),

        // The concatenated field order must match the use of c1_first above.
        .enq_data(afu_raw.c1Tx),
        .enq_en(c1_enq_en),
        .notFull(),
        .almostFull(afu_raw.c1TxAlmFull),

        .first(c1_first),
        .deq_en(deqC1Tx),
        .notEmpty(c1_notEmpty)
        );


    // ====================================================================
    //
    // Channel 2 Tx (MMIO read response) is unbuffered.
    //
    // ====================================================================

    assign afu_buf.c2Tx = afu_raw.c2Tx;

endmodule // cci_mpf_shim_buffer_afu_c1
