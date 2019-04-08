// (C) 2001-2017 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.




////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Expose extra core clocks from IOPLL
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
module altera_emif_arch_nf_pll_extra_clks #(
   parameter PLL_NUM_OF_EXTRA_CLKS = 0,
   parameter DIAG_SIM_REGTEST_MODE = 0
) (
   input  logic                                               pll_locked,
   input  logic [8:0]                                         pll_c_counters,
   output logic                                               pll_extra_clk_0,
   output logic                                               pll_extra_clk_1,
   output logic                                               pll_extra_clk_2,
   output logic                                               pll_extra_clk_3,
   output logic                                               pll_extra_clk_diag_ok
);
   timeunit 1ns;
   timeprecision 1ps;

   logic [3:0] pll_extra_clks;

   // Extra core clocks to user logic.
   // These clocks are unrelated to EMIF core clock domains. The feature is intended as a
   // way to reuse EMIF PLL to generate core clocks for designs in which physical PLLs are scarce.
   assign pll_extra_clks   = pll_c_counters[8:5];
   assign pll_extra_clk_0  = pll_extra_clks[0];
   assign pll_extra_clk_1  = pll_extra_clks[1];
   assign pll_extra_clk_2  = pll_extra_clks[2];
   assign pll_extra_clk_3  = pll_extra_clks[3];

   // In internal test mode, generate additional counters clocked by the extra clocks
   generate
      genvar i;

      if (DIAG_SIM_REGTEST_MODE && PLL_NUM_OF_EXTRA_CLKS > 0) begin: test_mode
         logic [PLL_NUM_OF_EXTRA_CLKS-1:0] pll_extra_clk_diag_done;

         for (i = 0; i < PLL_NUM_OF_EXTRA_CLKS; ++i)
         begin : extra_clk
            logic [9:0] counter;

            always_ff @(posedge pll_extra_clks[i] or negedge pll_locked) begin
               if (~pll_locked) begin
                  counter <= '0;
                  pll_extra_clk_diag_done[i] <= 1'b0;
               end else begin
                  if (~counter[9]) begin
                     counter <= counter + 1'b1;
                  end
                  pll_extra_clk_diag_done[i] <= counter[9];
               end
            end
         end

         assign pll_extra_clk_diag_ok = &pll_extra_clk_diag_done;

      end else begin : normal_mode
         assign pll_extra_clk_diag_ok = 1'b1;
      end
   endgenerate

endmodule
