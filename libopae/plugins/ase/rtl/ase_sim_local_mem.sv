//
// Copyright (c) 2019, Intel Corporation
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
// Instantiate local memory models.
//

`include "platform_if.vh"

module ase_sim_local_mem_avmm
 #(
   parameter NUM_BANKS = 2,
   parameter ADDR_WIDTH = 27,
   parameter DATA_WIDTH = 512,
   parameter BURST_CNT_WIDTH = 7
   )
  (
   // Local memory as Avalon master
`ifdef OFS_PLAT_PROVIDES_ASE_TOP
   ofs_plat_local_mem_avalon_if.to_afu local_mem[NUM_BANKS],
`else
   avalon_mem_if.to_afu local_mem[NUM_BANKS],
`endif

   // Memory clocks, one for each bank
   output logic clks[NUM_BANKS]
   );

   logic ddr_reset_n;
   logic [NUM_BANKS-1:0] ddr_pll_ref_clk;
   real delay = 1875; // 266.666 MHz

   initial begin
      #0     ddr_reset_n = 0;
             ddr_pll_ref_clk = {NUM_BANKS{1'b0}};
      #10000 ddr_reset_n = 1;
   end

   // Number of bytes in a data line
   localparam DATA_N_BYTES = (DATA_WIDTH + 7) / 8;

   logic emul_waitrequest[NUM_BANKS];
   logic [DATA_WIDTH-1:0] emul_readdata[NUM_BANKS];
   logic emul_readdatavalid[NUM_BANKS];

   logic [BURST_CNT_WIDTH-1:0] emul_burstcount[NUM_BANKS];
   logic [DATA_WIDTH-1:0] emul_writedata[NUM_BANKS];
   logic [ADDR_WIDTH-1:0] emul_address[NUM_BANKS];
   logic emul_write[NUM_BANKS];
   logic emul_read[NUM_BANKS];
   logic [DATA_N_BYTES-1:0] emul_byteenable[NUM_BANKS];

   // emif model
   genvar b;
   generate
      for (b = 0; b < NUM_BANKS; b = b + 2)
      begin : b_emul
         // Slightly different clock on each bank
         always #(delay+b) ddr_pll_ref_clk[b] = ~ddr_pll_ref_clk[b];

         emif_ddr4
          #(
            .DDR_ADDR_WIDTH(local_mem[b].ADDR_WIDTH),
            .DDR_DATA_WIDTH(local_mem[b].DATA_WIDTH)
            )
          emif_ddr4
          (
            .ddr4a_avmm_waitrequest                (emul_waitrequest[b]),
            .ddr4a_avmm_readdata                   (emul_readdata[b]),
            .ddr4a_avmm_readdatavalid              (emul_readdatavalid[b]),
            .ddr4a_avmm_burstcount                 (emul_burstcount[b]),
            .ddr4a_avmm_writedata                  (emul_writedata[b]),
            .ddr4a_avmm_address                    (emul_address[b]),
            .ddr4a_avmm_write                      (emul_write[b]),
            .ddr4a_avmm_read                       (emul_read[b]),
            .ddr4a_avmm_byteenable                 (emul_byteenable[b]),
            .ddr4a_avmm_clk_clk                    (clks[b]),

            .ddr4a_global_reset_reset_sink_reset_n (ddr_reset_n),
            .ddr4a_pll_ref_clk_clock_sink_clk      (ddr_pll_ref_clk[b]),

            .ddr4b_avmm_waitrequest                (emul_waitrequest[b+1]),
            .ddr4b_avmm_readdata                   (emul_readdata[b+1]),
            .ddr4b_avmm_readdatavalid              (emul_readdatavalid[b+1]),
            .ddr4b_avmm_burstcount                 (emul_burstcount[b+1]),
            .ddr4b_avmm_writedata                  (emul_writedata[b+1]),
            .ddr4b_avmm_address                    (emul_address[b+1]),
            .ddr4b_avmm_write                      (emul_write[b+1]),
            .ddr4b_avmm_read                       (emul_read[b+1]),
            .ddr4b_avmm_byteenable                 (emul_byteenable[b+1]),
            .ddr4b_avmm_clk_clk                    (clks[b+1])
         );
      end
   endgenerate

   //
   // Add a bridge between the emulator and the DUT in order to eliminate
   // timing glitches induces by the DDR model. The model's waitrequest isn't
   // quite aligned to the clock, leading to random failures in some RTL
   // emulators.
   //
   generate
      for (b = 0; b < NUM_BANKS; b = b + 1)
      begin : b_bridge
         ase_sim_local_mem_avmm_bridge
          #(
            .DATA_WIDTH(local_mem[b].DATA_WIDTH),
            .HDL_ADDR_WIDTH(local_mem[b].ADDR_WIDTH),
            .BURSTCOUNT_WIDTH(local_mem[b].BURST_CNT_WIDTH)
            )
          bridge
           (
            .clk(local_mem[b].clk),
            .reset(local_mem[b].reset),

            .s0_waitrequest(local_mem[b].waitrequest),
            .s0_readdata(local_mem[b].readdata),
            .s0_readdatavalid(local_mem[b].readdatavalid),
            .s0_response(),
            .s0_burstcount(local_mem[b].burstcount),
            .s0_writedata(local_mem[b].writedata),
            .s0_address(local_mem[b].address),
            .s0_write(local_mem[b].write),
            .s0_read(local_mem[b].read),
            .s0_byteenable(local_mem[b].byteenable),
            .s0_debugaccess(1'b0),

            .m0_waitrequest(emul_waitrequest[b]),
            .m0_readdata(emul_readdata[b]),
            .m0_readdatavalid(emul_readdatavalid[b]),
            .m0_response('x),
            .m0_burstcount(emul_burstcount[b]),
            .m0_writedata(emul_writedata[b]),
            .m0_address(emul_address[b]),
            .m0_write(emul_write[b]),
            .m0_read(emul_read[b]),
            .m0_byteenable(emul_byteenable[b]),
            .m0_debugaccess()
            );

         // Mostly used for debugging
         assign local_mem[b].bank_number = b;
      end
   endgenerate

endmodule // ase_sim_local_mem_avmm
