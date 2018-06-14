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
`include "cci_mpf_csrs.vh"

//
// Implements a history buffer shim in an MPF pipeline.  This module is not
// designed to be part of a normal pipeline.  It can be inserted for
// debugging, similar to SignalTap.  Values are added to the history
// buffer through the module interface and read back from the host using
// MMIO reads.
//

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  This module assumes there is at most one MMIO read to the history
//  buffer active at a time.  Software that pipelines the history
//  buffer reads may receive incorrect responses.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

module cci_mpf_shim_dbg_history
  #(
    // Base address of the history buffer in MMIO space.  This is the
    // unshifted address, including the low zero word index bits.
    parameter MMIO_BASE_ADDR = 'h2000,

    // 32 or 64 bit registers are supported
    parameter N_MMIO_REG_BITS = 32,

    // Number of entries in the ring buffer
    parameter N_ENTRIES = 1024,

    // Data size must be <= MMIO_REG_BITS
    parameter N_DATA_BITS = 32,

    // Indexed writes or ring buffer?  When RING_MODE is enabled history
    // buffer entries are written round-robin.  When RING_MODE is 0
    // new entries are written to history entry wr_idx.
    parameter RING_MODE = 1,

    // In ring mode write the ring once to collect the first N_ENTRIES.
    parameter WRITE_ONCE = 0,

    // Lock out writes after first read?  This keeps the ring from advancing
    // when readout starts.
    parameter LOCK_WRITES_AFTER_READ = 1
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    // Write a new entry
    input  logic wr_en,
    input  logic [N_DATA_BITS-1 : 0] wr_data,
    input  logic [$clog2(N_ENTRIES)-1 : 0] wr_idx
    );

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    logic mem_rdy;

    //
    // Channels 0 and 1 pass through.
    //
    assign fiu.c0Tx = afu.c0Tx;
    assign fiu.c1Tx = afu.c1Tx;
    assign afu.c0Rx = fiu.c0Rx;
    assign afu.c1Rx = fiu.c1Rx;

    assign afu.c0TxAlmFull = fiu.c0TxAlmFull || ! mem_rdy;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull || ! mem_rdy;


    // Forward responses to host, either generated locally (c2_rsp) or from
    // the AFU.
    t_if_ccip_c2_Tx c2_rsp;
    logic c2_rsp_en;

    always_ff @(posedge clk)
    begin
        fiu.c2Tx <= (c2_rsp_en ? c2_rsp : afu.c2Tx);
    end


    //
    // Write address control
    //
    logic wen;
    logic [$clog2(N_ENTRIES)-1 : 0] waddr;
    logic [$clog2(N_ENTRIES)-1 : 0] waddr_ring;
    logic wr_wrapped;
    logic wr_locked;

    // Write when new data arrives unless in ring mode and all entries
    // were already written.
    assign wen = wr_en && ! wr_locked && (! wr_wrapped ||
                                          (WRITE_ONCE == 0) ||
                                          (RING_MODE == 0));
    // Address is either next index in ring mode or wr_idx if user specified.
    assign waddr = ((RING_MODE == 0) ? wr_idx : waddr_ring);

    always_ff @(posedge clk)
    begin
        if (wen)
        begin
            wr_wrapped <= wr_wrapped || (&(waddr_ring) == 1'b1);
            waddr_ring <= waddr_ring + 1;
        end

        if (reset)
        begin
            wr_wrapped <= 1'b0;
            waddr_ring <= 0;
        end
    end


    //
    // Buffer
    //
    logic [$clog2(N_ENTRIES)-1 : 0] raddr;
    logic [N_DATA_BITS-1 : 0] rdata;

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .N_OUTPUT_REG_STAGES(1),
        .REGISTER_WRITES(1),
        .BYPASS_REGISTERED_WRITES(0),
        .INIT_VALUE(N_DATA_BITS'(64'haaaaaaaaaaaaaaaa))
        )
      mem
       (
        .clk,
        .reset,
        .rdy(mem_rdy),

        .wen,
        .waddr,
        .wdata(wr_data),

        .raddr,
        .rdata
        );


    // ====================================================================
    //
    // Detect and respond to MMIO reads.
    //
    // ====================================================================

    t_if_cci_c0_Rx c0Rx;

    logic is_mmio_rd;
    logic is_mmio_rd_q;
    logic is_mmio_rd_qq;
    t_cci_mmioAddr mmio_rd_addr;
    t_cci_tid mmio_rd_tid;

    always_ff @(posedge clk)
    begin
        c0Rx <= fiu.c0Rx;
    end

    localparam MMIO_ADDR_START = (MMIO_BASE_ADDR >> 2);
    localparam MMIO_ADDR_END   = (MMIO_BASE_ADDR +
                                  N_ENTRIES * (N_MMIO_REG_BITS / 8)) >> 2;

    always_ff @(posedge clk)
    begin
        // A new read?  The code assumes at most one read to the buffer is
        // active, so doesn't check that a read is active.
        if (cci_csr_isRead(c0Rx) &&
            (cci_csr_getAddress(c0Rx) >= MMIO_ADDR_START) &&
            (cci_csr_getAddress(c0Rx) < MMIO_ADDR_END))
        begin
            is_mmio_rd <= 1'b1;
            mmio_rd_addr <= cci_csr_getAddress(c0Rx) - t_cci_mmioAddr'(MMIO_ADDR_START);
            mmio_rd_tid <= cci_csr_getTid(c0Rx);

            // Lock writes after first read?
            wr_locked <= (LOCK_WRITES_AFTER_READ ? 1'b1 : 1'b0);
        end

        // Read data available after 2 cycles.  The read will remain active
        // until the response to the host is generated.
        is_mmio_rd_q <= is_mmio_rd;
        is_mmio_rd_qq <= is_mmio_rd_q;

        if (reset || c2_rsp_en)
        begin
            is_mmio_rd <= 1'b0;
            is_mmio_rd_q <= 1'b0;
            is_mmio_rd_qq <= 1'b0;
        end

        if (reset)
        begin
            wr_locked <= 1'b0;
        end
    end

    // Read history buffer
    always_comb
    begin
        raddr = mmio_rd_addr[(N_MMIO_REG_BITS / 32)-1 +: $bits(raddr)];

        if (RING_MODE != 0)
        begin
            // Read address is relative to the oldest entry in ring mode
            raddr = waddr_ring - 1 - raddr;
        end
    end

    // Respond when ready and no afu response is active
    assign c2_rsp_en = is_mmio_rd_qq && ! afu.c2Tx.mmioRdValid;
    always_comb
    begin
        c2_rsp.mmioRdValid = 1'b1;
        c2_rsp.hdr.tid = mmio_rd_tid;
        c2_rsp.data = t_ccip_mmioData'(rdata);
    end

endmodule // cci_mpf_shim_dbg_history
