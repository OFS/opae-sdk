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
   parameter NUM_BANKS = 2
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
            .ddr4a_avmm_waitrequest                (local_mem[b].waitrequest),
            .ddr4a_avmm_readdata                   (local_mem[b].readdata),
            .ddr4a_avmm_readdatavalid              (local_mem[b].readdatavalid),
            .ddr4a_avmm_burstcount                 (local_mem[b].burstcount),
            .ddr4a_avmm_writedata                  (local_mem[b].writedata),
            .ddr4a_avmm_address                    (local_mem[b].address),
            .ddr4a_avmm_write                      (local_mem[b].write),
            .ddr4a_avmm_read                       (local_mem[b].read),
            .ddr4a_avmm_byteenable                 (local_mem[b].byteenable),
            .ddr4a_avmm_clk_clk                    (clks[b]),

            .ddr4a_global_reset_reset_sink_reset_n (ddr_reset_n),
            .ddr4a_pll_ref_clk_clock_sink_clk      (ddr_pll_ref_clk[b]),

            .ddr4b_avmm_waitrequest                (local_mem[b+1].waitrequest),
            .ddr4b_avmm_readdata                   (local_mem[b+1].readdata),
            .ddr4b_avmm_readdatavalid              (local_mem[b+1].readdatavalid),
            .ddr4b_avmm_burstcount                 (local_mem[b+1].burstcount),
            .ddr4b_avmm_writedata                  (local_mem[b+1].writedata),
            .ddr4b_avmm_address                    (local_mem[b+1].address),
            .ddr4b_avmm_write                      (local_mem[b+1].write),
            .ddr4b_avmm_read                       (local_mem[b+1].read),
            .ddr4b_avmm_byteenable                 (local_mem[b+1].byteenable),
            .ddr4b_avmm_clk_clk                    (clks[b+1])
         );

         // Mostly used for debugging
         assign local_mem[b].bank_number = b;
         assign local_mem[b+1].bank_number = b+1;
      end
   endgenerate

endmodule // ase_sim_local_mem_avmm
