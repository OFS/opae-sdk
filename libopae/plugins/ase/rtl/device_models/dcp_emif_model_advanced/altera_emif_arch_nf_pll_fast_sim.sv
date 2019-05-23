// Copyright(c) 2017, Intel Corporation
//
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
//  EMIF IOPLL instantiation for 20nm families
//
//  The following table describes the usage of IOPLL by EMIF.
//
//  PLL Counter    Fanouts                          Usage
//  =====================================================================================
//  VCO Outputs    vcoph[7:0] -> phy_clk_phs[7:0]   FR clocks, 8 phases (45-deg apart)
//                 vcoph[0] -> DLL                  FR clock to DLL
//  C-counter 0    lvds_clk[0] -> phy_clk[1]        Secondary PHY clock tree (C2P/P2C rate)
//  C-counter 1    loaden[0] -> phy_clk[0]          Primary PHY clock tree (PHY/HMC rate)
//  C-counter 2    phy_clk[2]                       Feedback PHY clock tree (slowest phy clock in system)
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
module altera_emif_arch_nf_pll_fast_sim #(
   parameter PLL_SIM_VCO_FREQ_PS                     = 0,
   parameter PLL_SIM_PHYCLK_0_FREQ_PS                = 0,
   parameter PLL_SIM_PHYCLK_1_FREQ_PS                = 0,
   parameter PLL_SIM_PHYCLK_FB_FREQ_PS               = 0,
   parameter PLL_SIM_PHY_CLK_VCO_PHASE_PS            = 0,
   parameter PLL_SIM_CAL_SLAVE_CLK_FREQ_PS           = 0,
   parameter PLL_SIM_CAL_MASTER_CLK_FREQ_PS          = 0,
   parameter PORT_DFT_NF_PLL_CNTSEL_WIDTH            = 1,
   parameter PORT_DFT_NF_PLL_NUM_SHIFT_WIDTH         = 1

) (
   input  logic                                               global_reset_n_int,
   input  logic                                               pll_ref_clk_int,
   output logic                                               pll_locked,
   output logic                                               pll_dll_clk,
   output logic [7:0]                                         phy_clk_phs,
   output logic [1:0]                                         phy_clk,
   output logic                                               phy_fb_clk_to_tile,
   input  logic                                               phy_fb_clk_to_pll,
   output logic [8:0]                                         pll_c_counters,
   input  logic                                               pll_phase_en,
   input  logic                                               pll_up_dn,
   input  logic [PORT_DFT_NF_PLL_CNTSEL_WIDTH-1:0]            pll_cnt_sel,
   input  logic [PORT_DFT_NF_PLL_NUM_SHIFT_WIDTH-1:0]         pll_num_phase_shifts,
   output logic                                               pll_phase_done
);
   timeunit 1ps;
   timeprecision 1ps;

   localparam VCO_PHASES = 8;

   reg vco_out, phyclk0_out, phyclk1_out, fbclk_out, cal_slave_clk_out, cal_master_clk_out;
   reg [4:0] pll_lock_count;
   initial begin
      vco_out <= 1'b1;
      forever #(PLL_SIM_VCO_FREQ_PS/2) vco_out <= ~vco_out;
   end
   initial begin
      phyclk0_out <= 1'b1;
      #(PLL_SIM_VCO_FREQ_PS*PLL_SIM_PHY_CLK_VCO_PHASE_PS/VCO_PHASES);
      forever #(PLL_SIM_PHYCLK_0_FREQ_PS/2) phyclk0_out <= ~phyclk0_out;
   end
   initial begin
      phyclk1_out <= 1'b1;
      #(PLL_SIM_VCO_FREQ_PS*PLL_SIM_PHY_CLK_VCO_PHASE_PS/VCO_PHASES);
      forever #(PLL_SIM_PHYCLK_1_FREQ_PS/2) phyclk1_out <= ~phyclk1_out;
   end
   initial begin
      fbclk_out <= 1'b1;
      #(PLL_SIM_VCO_FREQ_PS*PLL_SIM_PHY_CLK_VCO_PHASE_PS/VCO_PHASES);
      forever #(PLL_SIM_PHYCLK_FB_FREQ_PS/2) fbclk_out <= ~fbclk_out;
   end
   initial begin
      cal_slave_clk_out <= 1'b1;
      forever #(PLL_SIM_CAL_SLAVE_CLK_FREQ_PS/2) cal_slave_clk_out <= ~cal_slave_clk_out;
   end
   initial begin
      cal_master_clk_out <= 1'b1;
      forever #(PLL_SIM_CAL_MASTER_CLK_FREQ_PS/2) cal_master_clk_out <= ~cal_master_clk_out;
   end

   always @ (posedge vco_out or negedge global_reset_n_int) begin
      if (~global_reset_n_int) begin
         pll_lock_count <= 5'b0;
      end else if (pll_lock_count != 5'b11111) begin
         pll_lock_count <= pll_lock_count + 1;
      end
   end

   assign pll_locked = global_reset_n_int & (pll_lock_count == 5'b11111);
   assign pll_dll_clk = global_reset_n_int & vco_out;
   assign phy_clk_phs[0] = global_reset_n_int & vco_out;
   always @ (*) begin
      phy_clk_phs[1] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[0];
      phy_clk_phs[2] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[1];
      phy_clk_phs[3] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[2];
      phy_clk_phs[4] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[3];
      phy_clk_phs[5] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[4];
      phy_clk_phs[6] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[5];
      phy_clk_phs[7] <= #(PLL_SIM_VCO_FREQ_PS/VCO_PHASES) phy_clk_phs[6];
   end
   assign phy_clk = {global_reset_n_int & phyclk1_out, global_reset_n_int & phyclk0_out};
   assign phy_fb_clk_to_tile = global_reset_n_int & fbclk_out;
   assign pll_c_counters[0] =  global_reset_n_int & phyclk1_out;
   assign pll_c_counters[1] =  global_reset_n_int & phyclk0_out;
   assign pll_c_counters[2] =  global_reset_n_int & fbclk_out;
   assign pll_c_counters[3] =  global_reset_n_int & cal_slave_clk_out;
   assign pll_c_counters[4] =  global_reset_n_int & cal_master_clk_out;
   assign pll_c_counters[8:5] = 5'b0;
   assign pll_phase_done = 1'b1;

endmodule
