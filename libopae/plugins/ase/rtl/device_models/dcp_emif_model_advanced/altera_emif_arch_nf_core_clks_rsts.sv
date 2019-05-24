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




///////////////////////////////////////////////////////////////////////////////
// This module handles the creation and wiring of the core clock/reset signals.
//
///////////////////////////////////////////////////////////////////////////////

// altera message_off 10036

 module altera_emif_arch_nf_core_clks_rsts #(
   parameter PHY_CONFIG_ENUM                         = "",
   parameter PHY_CORE_CLKS_SHARING_ENUM              = "",
   parameter IS_VID                                  = 0,
   parameter PHY_PING_PONG_EN                        = 0,
   parameter C2P_P2C_CLK_RATIO                       = 1,
   parameter USER_CLK_RATIO                          = 1,
   parameter PORT_CLKS_SHARING_MASTER_OUT_WIDTH      = 32,
   parameter PORT_CLKS_SHARING_SLAVE_IN_WIDTH        = 32,
   parameter DIAG_CPA_OUT_1_EN                       = 0,
   parameter DIAG_USE_CPA_LOCK                       = 1,
   parameter DIAG_SYNTH_FOR_SIM                      = 1,
   parameter PORT_DFT_NF_CORE_CLK_BUF_OUT_WIDTH      = 1,
   parameter PORT_DFT_NF_CORE_CLK_LOCKED_WIDTH       = 1
) (
   // For a master interface, the PLL ref clock and the global reset signal
   // come from an external source from user logic, via the following ports.
   // For slave interfaces, they come from the master via the sharing interface.
   // The connectivity ensures that all interfaces in a master/slave
   // configuration share the same ref clock and global reset, which is
   // one of the requirements for core-clock sharing.
   input  logic                                                 pll_ref_clk,
   input  logic                                                 global_reset_n,

   // This signal is connected to the SmartVID controller, and signals EMIF to
   // only calibrate after SmartVID is done voltage calibration.
   input  logic                                                 vid_cal_done_persist,

   // For a master interface, core clocks come from the clock phase alignment
   // block of the current interface, via the following ports. Note that the
   // CPA block also expects feedback signals after the clock signals have
   // propagated through core clock networks.
   // For slave interfaces, the core clock signals come from the master
   // via the sharing interface.
   input  logic [1:0]                                           core_clks_from_cpa_pri,
   input  logic [1:0]                                           core_clks_locked_cpa_pri,
   output logic [1:0]                                           core_clks_fb_to_cpa_pri,

   input  logic [1:0]                                           core_clks_from_cpa_sec,
   input  logic [1:0]                                           core_clks_locked_cpa_sec,
   output logic [1:0]                                           core_clks_fb_to_cpa_sec,

   // Since CPA clocks are generated from VCOPH clocks, which aren't stable
   // until after duty-cycle-correction circuitry has stablized, we must gate
   // core until using the follwing signal from the sequencer.
   input logic                                                  dcc_stable,

   // PLL lock signal
   input  logic                                                 pll_locked,

   // PLL c-counters
   input  logic [8:0]                                           pll_c_counters,

   // For a master interface, the core reset signal is generated by synchronizing
   // the deassertion of the async reset coming from the hard PHY via the following
   // port, to the core clock signal.
   // For slave interfaces, the core reset signal comes from the master
   // via the sharing interface.
   input  logic                                                 phy_reset_n,

   // The following is the master/slave sharing interfaces.
   input  logic [PORT_CLKS_SHARING_SLAVE_IN_WIDTH-1:0]          clks_sharing_slave_in,
   output logic [PORT_CLKS_SHARING_MASTER_OUT_WIDTH-1:0]        clks_sharing_master_out,

   // The following are the actual pll ref clock and global reset signals that
   // will be used internally by the rest of the IP.
   output logic                                                 pll_ref_clk_int,
   output logic                                                 global_reset_n_int,

   // The following are all the possible core clock/reset signals.
   // afi_* only exists in PHY-only mode (or if soft controller is used).
   // emif_usr_* only exists if hard memory controller is used.
   output logic                                                 afi_clk,
   output logic                                                 afi_half_clk,
   output logic                                                 afi_reset_n,

   output logic                                                 emif_usr_clk,
   output logic                                                 emif_usr_half_clk,
   output logic                                                 emif_usr_reset_n,

   output logic                                                 emif_usr_clk_sec,
   output logic                                                 emif_usr_half_clk_sec,
   output logic                                                 emif_usr_reset_n_sec,

   // The calibration slave core clock domain is used by core logic that serves
   // as Avalon slaves for the sequencer CPU. The clock comes directly from PLL C-counter
   // instead of from the CPA.
   output logic                                                 cal_slave_clk,
   output logic                                                 cal_slave_reset_n,

   // The calibration master core clock domain is used by core logic that serves
   // as Avalon masters for the sequencer calbus. The clock comes directly from PLL C-counter
   // instead of from the CPA.
   output logic                                                 cal_master_clk,
   output logic                                                 cal_master_reset_n,

   // DFT
   output logic [PORT_DFT_NF_CORE_CLK_BUF_OUT_WIDTH-1:0]        dft_core_clk_buf_out,
   output logic [PORT_DFT_NF_CORE_CLK_LOCKED_WIDTH-1:0]         dft_core_clk_locked
);
   timeunit 1ns;
   timeprecision 1ps;

   // This is the length of the core reset synchronizer chain.
   // It is set higher than the length of the reset chain
   // in the periphery (5+), to avoid the core from getting out of reset
   // earlier than the hard PHY/sequencer/controller.
   // This is for extra safety, but isn't actually necessary, because
   // soft logic must either wait for the hard controller's assertion of
   // the ready signal, or, in case the hard controller isn't used,
   // for the sequencer to assert the afi_cal_success, prior to accessing
   // the hard circuitries. The minimal requirement for reset deassertion
   // is a guarantee that the core clock is stable.
   localparam CPA_RESET_SYNC_LENGTH = 7;

   // Reset synchronizer chain length for PLL-based core clocks
   localparam PLL_RESET_SYNC_LENGTH = 3;

   /////////////////////////////////////////////////////////////
   // Get signals to/from the master/slave sharing interface.
   logic pll_locked_int;
   logic phy_reset_n_int;
   logic counter_lock;
   logic cpa_lock_pri;
   logic cpa_lock_sec;
   logic async_reset_n_pri;
   logic async_reset_n_sec;

   assign async_reset_n_pri = (DIAG_USE_CPA_LOCK ? cpa_lock_pri : counter_lock) & dcc_stable;
   assign async_reset_n_sec = (DIAG_USE_CPA_LOCK ? cpa_lock_sec : counter_lock) & dcc_stable;

   logic pll_ref_clk_slave_in;
   logic pll_locked_slave_in;
   logic global_reset_n_slave_in;
   logic cpa_lock_pri_slave_in;
   logic cpa_lock_sec_slave_in;
   logic phy_reset_n_slave_in;
   logic afi_clk_slave_in;
   logic afi_half_clk_slave_in;
   logic afi_reset_n_pre_reg_slave_in;
   logic afi_reset_n_pre_reg;
   logic counter_lock_slave_in;
   logic emif_usr_clk_slave_in;
   logic emif_usr_half_clk_slave_in;
   logic emif_usr_reset_n_pri_pre_reg_slave_in;
   logic emif_usr_reset_n_pri_pre_reg;
   logic emif_usr_clk_sec_slave_in;
   logic emif_usr_half_clk_sec_slave_in;
   logic emif_usr_reset_n_sec_pre_reg_slave_in;
   logic emif_usr_reset_n_sec_pre_reg;
   logic cal_slave_clk_slave_in;
   logic cal_master_clk_slave_in;
   logic cal_master_reset_n_slave_in;

   /////////////////////////////////////////////////////////////
   // Generate connectivity for PLL ref clk and reset.
   generate
      if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
      begin : slave
         assign pll_ref_clk_int                   = pll_ref_clk_slave_in;
         assign pll_locked_int                    = pll_locked_slave_in;
         assign global_reset_n_int                = global_reset_n_slave_in;
         assign phy_reset_n_int                   = phy_reset_n_slave_in;

         assign pll_ref_clk_slave_in                  = clks_sharing_slave_in[0];
         assign global_reset_n_slave_in               = clks_sharing_slave_in[1];
         assign phy_reset_n_slave_in                  = clks_sharing_slave_in[2];
         assign cpa_lock_pri_slave_in                 = clks_sharing_slave_in[3];
         assign afi_clk_slave_in                      = clks_sharing_slave_in[4];
         assign afi_half_clk_slave_in                 = clks_sharing_slave_in[5];
         assign afi_reset_n_pre_reg_slave_in          = clks_sharing_slave_in[6];
         assign counter_lock_slave_in                 = clks_sharing_slave_in[7];
         assign emif_usr_clk_slave_in                 = clks_sharing_slave_in[8];
         assign cal_slave_clk_slave_in                = clks_sharing_slave_in[9];
         assign emif_usr_reset_n_pri_pre_reg_slave_in = clks_sharing_slave_in[10];
         assign emif_usr_half_clk_slave_in            = clks_sharing_slave_in[11];
         assign pll_locked_slave_in                   = clks_sharing_slave_in[12];
         assign cpa_lock_sec_slave_in                 = clks_sharing_slave_in[13];
         assign emif_usr_clk_sec_slave_in             = clks_sharing_slave_in[14];
         assign emif_usr_reset_n_sec_pre_reg_slave_in = clks_sharing_slave_in[15];
         assign emif_usr_half_clk_sec_slave_in        = clks_sharing_slave_in[16];
         assign cal_master_clk_slave_in               = clks_sharing_slave_in[17];
         assign cal_master_reset_n_slave_in           = clks_sharing_slave_in[18];

         assign clks_sharing_master_out           = '0;
      end else
      begin : master

         logic probe_global_reset_n;

`ifdef ALTERA_EMIF_ENABLE_ISSP
         altsource_probe #(
            .sld_auto_instance_index ("YES"),
            .sld_instance_index      (0),
            .instance_id             ("RSTN"),
            .probe_width             (0),
            .source_width            (1),
            .source_initial_value    ("1"),
            .enable_metastability    ("NO")
         ) global_reset_n_issp (
            .source  (probe_global_reset_n)
         );

         altsource_probe #(
            .sld_auto_instance_index ("YES"),
            .sld_instance_index      (0),
            .instance_id             ("PALP"),
            .probe_width             (1),
            .source_width            (0),
            .source_initial_value    ("0"),
            .enable_metastability    ("NO")
         ) cpa_lock_pri_issp (
            .probe  (cpa_lock_pri)
         );

         altsource_probe #(
            .sld_auto_instance_index ("YES"),
            .sld_instance_index      (0),
            .instance_id             ("PALS"),
            .probe_width             (1),
            .source_width            (0),
            .source_initial_value    ("0"),
            .enable_metastability    ("NO")
         ) cpa_lock_sec_issp (
            .probe  (cpa_lock_sec)
         );
`else
         assign probe_global_reset_n = 1'b1;
`endif

         assign phy_reset_n_int    = phy_reset_n;
         assign pll_ref_clk_int    = pll_ref_clk;
         assign pll_locked_int     = pll_locked;

         if (IS_VID) begin : use_vid_persist_reset
            // Create register to capture path false path internal to EMIF block
            reg vid_cal_done_persist_reg;
            always_ff @(posedge pll_ref_clk) begin
               vid_cal_done_persist_reg <= vid_cal_done_persist;
            end
            assign global_reset_n_int = global_reset_n & probe_global_reset_n & vid_cal_done_persist_reg;
         end else begin : default_reset
            assign global_reset_n_int = global_reset_n & probe_global_reset_n;
         end

         assign clks_sharing_master_out[0] = pll_ref_clk_int;
         assign clks_sharing_master_out[1] = global_reset_n_int;
         assign clks_sharing_master_out[2] = phy_reset_n_int;
         assign clks_sharing_master_out[3] = cpa_lock_pri;
         assign clks_sharing_master_out[4] = afi_clk;
         assign clks_sharing_master_out[5] = afi_half_clk;
         assign clks_sharing_master_out[6] = afi_reset_n_pre_reg;
         assign clks_sharing_master_out[7] = counter_lock;
         assign clks_sharing_master_out[8] = emif_usr_clk;
         assign clks_sharing_master_out[9] = cal_slave_clk;
         assign clks_sharing_master_out[10] = emif_usr_reset_n_pri_pre_reg;
         assign clks_sharing_master_out[11] = emif_usr_half_clk;
         assign clks_sharing_master_out[12] = pll_locked;
         assign clks_sharing_master_out[13] = cpa_lock_sec;
         assign clks_sharing_master_out[14] = emif_usr_clk_sec;
         assign clks_sharing_master_out[15] = emif_usr_reset_n_sec_pre_reg;
         assign clks_sharing_master_out[16] = emif_usr_half_clk_sec;
         assign clks_sharing_master_out[17] = cal_master_clk;
         assign clks_sharing_master_out[18] = cal_master_reset_n;

         assign clks_sharing_master_out[PORT_CLKS_SHARING_MASTER_OUT_WIDTH-1:19] = '0;
      end
   endgenerate

   /////////////////////////////////////////////////////////////
   // Generate core clock lock signal if CPA lock isn't used
   generate
      if (DIAG_USE_CPA_LOCK)
      begin : use_cpa_lock
         assign counter_lock = 1'b0;
      end
      else
      begin : use_counter_lock
         if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
         begin : counter_lock_gen_slave
            assign counter_lock = counter_lock_slave_in;
         end else
         begin : counter_lock_gen_master

            // Synchronize PLL lock signal to PLL ref clock domain.
            // This may not be necessary but we do it for extra safety.
            logic pll_ref_clk_reset_n;
            logic pll_ref_clk_reset_n_sync_r;
            logic pll_ref_clk_reset_n_sync_rr;
            logic pll_ref_clk_reset_n_sync_rrr;

            assign pll_ref_clk_reset_n = pll_ref_clk_reset_n_sync_rrr;

            always_ff @(posedge pll_ref_clk_int or negedge pll_locked_int) begin
               if (~pll_locked_int) begin
                  pll_ref_clk_reset_n_sync_r   <= 1'b0;
                  pll_ref_clk_reset_n_sync_rr  <= 1'b0;
                  pll_ref_clk_reset_n_sync_rrr <= 1'b0;
               end else begin
                  pll_ref_clk_reset_n_sync_r   <= 1'b1;
                  pll_ref_clk_reset_n_sync_rr  <= pll_ref_clk_reset_n_sync_r;
                  pll_ref_clk_reset_n_sync_rrr <= pll_ref_clk_reset_n_sync_rr;
               end
            end

            // CPA takes ~50k core clock cycles to lock. Obviously we can't use a potentially
            // unstable core clock to clock the counter. We need to use the ref clock instead.
            // The fastest legal ref clock can run at the same rate as core clock, so we simply
            // count 64k PLL ref clock cycles.
            logic [16:0] cpa_count_to_lock;

            // The following is evaluated for simulation. Don't wait too long during simulation.
            // synthesis translate_off
            localparam COUNTER_LOCK_EXP = 9;
            // synthesis translate_on

            // The following is evaluated for synthesis. Don't wait too long when DIAG_SYNTH_FOR_SIM enabled.
            // synthesis read_comments_as_HDL on
            // localparam COUNTER_LOCK_EXP  = DIAG_SYNTH_FOR_SIM ? 9 : 16;
            // synthesis read_comments_as_HDL off

            always_ff @(posedge pll_ref_clk_int or negedge pll_ref_clk_reset_n) begin
               if (~pll_ref_clk_reset_n) begin
                  cpa_count_to_lock <= '0;
                  counter_lock <= 1'b0;
               end else begin
                  if (~cpa_count_to_lock[COUNTER_LOCK_EXP]) begin
                     cpa_count_to_lock <= cpa_count_to_lock + 1'b1;
                  end
                  counter_lock <= cpa_count_to_lock[COUNTER_LOCK_EXP];
               end
            end
         end
      end
   endgenerate

   /////////////////////////////////////////////////////////////
   // Generate CPA-based core clock signals
   logic [1:0] core_clks_from_cpa_pri_buffered;
   logic [1:0] core_clks_from_cpa_sec_buffered;

   /////////////////////////////////////////////////////////////
   // Assign signals for DFT
   assign dft_core_clk_locked = DIAG_USE_CPA_LOCK ? core_clks_locked_cpa_pri : {2{counter_lock}};
   assign dft_core_clk_buf_out = core_clks_from_cpa_pri_buffered;

   generate
      if (PHY_CONFIG_ENUM == "CONFIG_PHY_AND_HARD_CTRL")
      begin : clk_gen_hmc

         // If HMC is used, there's no AFI clock
         assign afi_half_clk = 1'b0;
         assign afi_clk      = 1'b0;

         if (USER_CLK_RATIO == 2 && C2P_P2C_CLK_RATIO == 4)
         begin : bridge_2x
            // For 2x-bridge mode, expose two core clocks:
            //    0) A half-rate clock (i.e. emif_usr_clk)
            //    1) A quarter-rate clock (i.e. emif_usr_half_clk)

            if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
            begin : clk_gen_slave
               assign core_clks_from_cpa_pri_buffered = {emif_usr_half_clk_slave_in, emif_usr_clk_slave_in};
               assign core_clks_from_cpa_sec_buffered = {emif_usr_half_clk_sec_slave_in, emif_usr_clk_sec_slave_in};
               assign core_clks_fb_to_cpa_pri = '0;
               assign core_clks_fb_to_cpa_sec = '0;
               assign cpa_lock_pri = cpa_lock_pri_slave_in;
               assign cpa_lock_sec = cpa_lock_sec_slave_in;
            end else
            begin : clk_gen_master

               twentynm_clkena # (
                  .clock_type ("GLOBAL CLOCK")
               ) emif_usr_clk_buf (
                  .inclk  (core_clks_from_cpa_pri[0]),
                  .outclk (core_clks_from_cpa_pri_buffered[0]),
                  .ena    (1'b1),
                  .enaout ()
               );

               twentynm_clkena # (
                  .clock_type ("GLOBAL CLOCK")
               ) emif_usr_half_clk_buf (
                  .inclk  (core_clks_from_cpa_pri[1]),
                  .outclk (core_clks_from_cpa_pri_buffered[1]),
                  .ena    (1'b1),
                  .enaout ()
               );

               assign cpa_lock_pri = core_clks_locked_cpa_pri[0] & core_clks_locked_cpa_pri[1];
               assign core_clks_fb_to_cpa_pri = core_clks_from_cpa_pri_buffered;

               if (PHY_PING_PONG_EN) begin : gen_sec_clk
                  twentynm_clkena # (
                     .clock_type ("GLOBAL CLOCK")
                  ) emif_usr_clk_buf (
                     .inclk  (core_clks_from_cpa_sec[0]),
                     .outclk (core_clks_from_cpa_sec_buffered[0]),
                     .ena    (1'b1),
                     .enaout ()
                  );

                  twentynm_clkena # (
                     .clock_type ("GLOBAL CLOCK")
                  ) emif_usr_half_clk_buf (
                     .inclk  (core_clks_from_cpa_sec[1]),
                     .outclk (core_clks_from_cpa_sec_buffered[1]),
                     .ena    (1'b1),
                     .enaout ()
                  );

                  assign cpa_lock_sec = core_clks_locked_cpa_sec[0] & core_clks_locked_cpa_sec[1];
                  assign core_clks_fb_to_cpa_sec = core_clks_from_cpa_sec_buffered;

               end else begin : non_pp
                  assign cpa_lock_sec = 1'b0;
                  assign core_clks_fb_to_cpa_sec = '0;
                  assign core_clks_from_cpa_sec_buffered = '0;
               end
            end

            assign emif_usr_clk          = core_clks_from_cpa_pri_buffered[0];
            assign emif_usr_half_clk     = core_clks_from_cpa_pri_buffered[1];
            assign emif_usr_clk_sec      = core_clks_from_cpa_sec_buffered[0];
            assign emif_usr_half_clk_sec = core_clks_from_cpa_sec_buffered[1];

         end else
         begin : hr_qr

            // For half/quarter-rate, expose one core clock (i.e. emif_usr_clk)
            // running at the user-requested rate
            if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
            begin : clk_gen_slave
               assign core_clks_from_cpa_pri_buffered = {emif_usr_half_clk_slave_in, emif_usr_clk_slave_in};
               assign core_clks_from_cpa_sec_buffered = {emif_usr_half_clk_sec_slave_in, emif_usr_clk_sec_slave_in};
               assign core_clks_fb_to_cpa_pri = '0;
               assign core_clks_fb_to_cpa_sec = '0;
               assign cpa_lock_pri = cpa_lock_pri_slave_in;
               assign cpa_lock_sec = cpa_lock_sec_slave_in;
            end else
            begin : clk_gen_master

               twentynm_clkena # (
                  .clock_type ("GLOBAL CLOCK")
               ) emif_usr_clk_buf (
                  .inclk  (core_clks_from_cpa_pri[0]),
                  .outclk (core_clks_from_cpa_pri_buffered[0]),
                  .ena    (1'b1),
                  .enaout ()
               );

               if (DIAG_CPA_OUT_1_EN)
               begin : force_cpa_out_1_en
                  twentynm_clkena # (
                     .clock_type ("GLOBAL CLOCK")
                  ) emif_usr_extra_clk_buf (
                     .inclk  (core_clks_from_cpa_pri[1]),
                     .outclk (core_clks_from_cpa_pri_buffered[1]),
                     .ena    (1'b1),
                     .enaout ()
                  );
                  assign cpa_lock_pri = core_clks_locked_cpa_pri[0] & core_clks_locked_cpa_pri[1];

               end else begin : normal
                  assign core_clks_from_cpa_pri_buffered[1] = core_clks_from_cpa_pri_buffered[0];
                  assign cpa_lock_pri = core_clks_locked_cpa_pri[0];
               end

               assign core_clks_fb_to_cpa_pri = core_clks_from_cpa_pri_buffered;

               if (PHY_PING_PONG_EN) begin : gen_sec_clk
                  twentynm_clkena # (
                     .clock_type ("GLOBAL CLOCK")
                  ) emif_usr_clk_buf (
                     .inclk  (core_clks_from_cpa_sec[0]),
                     .outclk (core_clks_from_cpa_sec_buffered[0]),
                     .ena    (1'b1),
                     .enaout ()
                  );

                  assign cpa_lock_sec = core_clks_locked_cpa_sec[0];
                  assign core_clks_fb_to_cpa_sec = core_clks_from_cpa_sec_buffered;
                  assign core_clks_from_cpa_sec_buffered[1] = core_clks_from_cpa_sec_buffered[0];

               end else begin : non_pp
                  assign cpa_lock_sec = 1'b0;
                  assign core_clks_fb_to_cpa_sec = '0;
                  assign core_clks_from_cpa_sec_buffered = '0;
               end
            end

            assign emif_usr_clk          = core_clks_from_cpa_pri_buffered[0];
            assign emif_usr_half_clk     = core_clks_from_cpa_pri_buffered[1];
            assign emif_usr_clk_sec      = core_clks_from_cpa_sec_buffered[0];
            assign emif_usr_half_clk_sec = core_clks_from_cpa_sec_buffered[1];

         end
      end else
      begin : clk_gen_non_hmc

         // If HMC isn't used, there's no emif_usr_* clocks
         assign emif_usr_clk          = 1'b0;
         assign emif_usr_half_clk     = 1'b0;
         assign emif_usr_clk_sec      = 1'b0;
         assign emif_usr_half_clk_sec = 1'b0;

         // Always expose both afi_clk and afi_half_clk
         if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
         begin : clk_gen_slave
            assign core_clks_from_cpa_pri_buffered = {afi_clk_slave_in, afi_half_clk_slave_in};
            assign core_clks_from_cpa_sec_buffered = '0;
            assign core_clks_fb_to_cpa_pri = '0;
            assign core_clks_fb_to_cpa_sec = '0;
            assign cpa_lock_pri = cpa_lock_pri_slave_in;
            assign cpa_lock_sec = cpa_lock_sec_slave_in;
         end else
         begin : clk_gen_master

            twentynm_clkena # (
               .clock_type ("GLOBAL CLOCK")
            ) afi_half_clk_buf (
               .inclk  (core_clks_from_cpa_pri[0]),
               .outclk (core_clks_from_cpa_pri_buffered[0]),
               .ena    (1'b1),
               .enaout ()
            );

            twentynm_clkena # (
               .clock_type ("GLOBAL CLOCK")
            ) afi_clk_buf (
               .inclk  (core_clks_from_cpa_pri[1]),
               .outclk (core_clks_from_cpa_pri_buffered[1]),
               .ena    (1'b1),
               .enaout ()
            );

            assign core_clks_fb_to_cpa_pri = core_clks_from_cpa_pri_buffered;
            assign core_clks_fb_to_cpa_sec = '0;
            assign cpa_lock_pri = core_clks_locked_cpa_pri[0] & core_clks_locked_cpa_pri[1];
            assign cpa_lock_sec = 1'b0;
         end

         assign afi_half_clk = core_clks_from_cpa_pri_buffered[0];
         assign afi_clk      = core_clks_from_cpa_pri_buffered[1];
      end
   endgenerate

   /////////////////////////////////////////////////////////////
   // Generate core reset signals for CPA-based core clocks
   logic sync_clk_pri;
   logic sync_clk_sec;
   logic sync_reset_n_pri_pre_reg;
   logic sync_reset_n_sec_pre_reg;

   // Every interface flops the synchronized reset signal locally,
   // to avoid recovery/removal timing issue due to high fanout.
   // The flop is marked to prevent from being optimized away.
   (* altera_attribute = {"-name GLOBAL_SIGNAL OFF"}*) logic sync_reset_n /* synthesis dont_merge syn_noprune syn_preserve = 1 */;
   always_ff @(posedge sync_clk_pri or negedge async_reset_n_pri) begin
      if (~async_reset_n_pri) begin
         sync_reset_n <= '0;
      end else begin
         sync_reset_n <= sync_reset_n_pri_pre_reg;
      end
   end

   logic sync_reset_n_sec_ext;
   generate
      if (PHY_PING_PONG_EN) begin : pp
         (* altera_attribute = {"-name GLOBAL_SIGNAL OFF"}*) logic sync_reset_n_sec /* synthesis dont_merge syn_noprune syn_preserve = 1 */;
         always_ff @(posedge sync_clk_sec or negedge async_reset_n_sec) begin
            if (~async_reset_n_sec) begin
               sync_reset_n_sec <= '0;
            end else begin
               sync_reset_n_sec <= sync_reset_n_sec_pre_reg;
            end
         end
         assign sync_reset_n_sec_ext = sync_reset_n_sec;
      end else begin : no_pp
         assign sync_reset_n_sec_ext = 1'b0;
      end
   endgenerate

   generate
      if (PHY_CONFIG_ENUM == "CONFIG_PHY_AND_HARD_CTRL")
      begin : reset_gen_hmc
         // Use the slower clock to synchronize the reset to ease
         // recovery/removal in the slow clock domain.
         assign sync_clk_pri                 = (USER_CLK_RATIO == 2 && C2P_P2C_CLK_RATIO == 4 ? emif_usr_half_clk : emif_usr_clk);
         assign sync_clk_sec                 = (USER_CLK_RATIO == 2 && C2P_P2C_CLK_RATIO == 4 ? emif_usr_half_clk_sec : emif_usr_clk_sec);
         assign emif_usr_reset_n_pri_pre_reg = sync_reset_n_pri_pre_reg;
         assign emif_usr_reset_n_sec_pre_reg = sync_reset_n_sec_pre_reg;
         assign emif_usr_reset_n             = sync_reset_n;
         assign emif_usr_reset_n_sec         = sync_reset_n_sec_ext;
         assign afi_reset_n_pre_reg          = 1'b0;
         assign afi_reset_n                  = 1'b0;
      end else
      begin: reset_gen_non_hmc
         // afi_half_clk is the slower clock compared to afi_clk. Use the
         // slower clock to synchronize the reset to ease recovery/removal
         // in the slow clock domain.
         assign sync_clk_pri                 = afi_half_clk;
         assign sync_clk_sec                 = 1'b0;
         assign afi_reset_n_pre_reg          = sync_reset_n_pri_pre_reg;
         assign afi_reset_n                  = sync_reset_n;
         assign emif_usr_reset_n_pri_pre_reg = 1'b0;
         assign emif_usr_reset_n             = 1'b0;
         assign emif_usr_reset_n_sec_pre_reg = 1'b0;
         assign emif_usr_reset_n_sec         = 1'b0;
      end

      if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
      begin : reset_gen_slave
         // The master exposes a synchronized reset signal for the slaves
         if (PHY_CONFIG_ENUM == "CONFIG_PHY_AND_HARD_CTRL") begin
            assign sync_reset_n_pri_pre_reg = emif_usr_reset_n_pri_pre_reg_slave_in;
            assign sync_reset_n_sec_pre_reg = emif_usr_reset_n_sec_pre_reg_slave_in;
         end else begin
            assign sync_reset_n_pri_pre_reg = afi_reset_n_pre_reg_slave_in;
            assign sync_reset_n_sec_pre_reg = 1'b0;
         end
      end else
      begin : reset_gen_master

         // Synchronize reset deassertion to core clock
         logic [CPA_RESET_SYNC_LENGTH-1:0] reset_sync_pri;
         always_ff @(posedge sync_clk_pri or negedge async_reset_n_pri) begin
            if (~async_reset_n_pri) begin
               reset_sync_pri <= '0;
            end else begin
               reset_sync_pri[0] <= 1'b1;
               reset_sync_pri[CPA_RESET_SYNC_LENGTH-1:1] <= reset_sync_pri[CPA_RESET_SYNC_LENGTH-2:0];
            end
         end
         assign sync_reset_n_pri_pre_reg = reset_sync_pri[CPA_RESET_SYNC_LENGTH-1];

         if (PHY_PING_PONG_EN) begin : gen_sec_rst_sync
            logic [CPA_RESET_SYNC_LENGTH-1:0] reset_sync_sec;
            always_ff @(posedge sync_clk_sec or negedge async_reset_n_sec) begin
               if (~async_reset_n_sec) begin
                  reset_sync_sec <= '0;
               end else begin
                  reset_sync_sec[0] <= 1'b1;
                  reset_sync_sec[CPA_RESET_SYNC_LENGTH-1:1] <= reset_sync_sec[CPA_RESET_SYNC_LENGTH-2:0];
               end
            end
            assign sync_reset_n_sec_pre_reg = reset_sync_sec[CPA_RESET_SYNC_LENGTH-1];
         end else begin : no_pp
            assign sync_reset_n_sec_pre_reg = 1'b0;
         end
      end
   endgenerate

   /////////////////////////////////////////////////////////////
   // Generate PLL-based core clock and reset signals
   generate
      if (PHY_CORE_CLKS_SHARING_ENUM == "CORE_CLKS_SHARING_SLAVE")
      begin : pll_clk_gen_slave
         assign cal_slave_clk = cal_slave_clk_slave_in;
         assign cal_master_clk = cal_master_clk_slave_in;
         assign cal_master_reset_n = cal_master_reset_n_slave_in;
      end else
      begin : pll_clk_gen_master
         assign cal_slave_clk = pll_c_counters[3];
         assign cal_master_clk = pll_c_counters[4];

         logic [PLL_RESET_SYNC_LENGTH-1:0] reset_sync;
         logic async_reset_n;

         assign cal_master_reset_n = reset_sync[PLL_RESET_SYNC_LENGTH-1];
         assign async_reset_n      = (global_reset_n_int & pll_locked);

         always_ff @(posedge cal_master_clk or negedge async_reset_n) begin
            if (~async_reset_n) begin
               reset_sync <= '0;
            end else begin
               reset_sync[0] <= 1'b1;
               reset_sync[PLL_RESET_SYNC_LENGTH-1:1] <= reset_sync[PLL_RESET_SYNC_LENGTH-2:0];
            end
         end
      end

      (* altera_attribute = {"-name GLOBAL_SIGNAL OFF"}*) logic [PLL_RESET_SYNC_LENGTH-1:0] per_if_cal_slave_reset_sync /* synthesis dont_merge */;
      logic async_reset_n;

      assign cal_slave_reset_n = per_if_cal_slave_reset_sync[PLL_RESET_SYNC_LENGTH-1];
      assign async_reset_n     = (global_reset_n_int & pll_locked_int);

      always_ff @(posedge cal_slave_clk or negedge async_reset_n) begin
         if (~async_reset_n) begin
            per_if_cal_slave_reset_sync <= '0;
         end else begin
            per_if_cal_slave_reset_sync[0] <= 1'b1;
            per_if_cal_slave_reset_sync[PLL_RESET_SYNC_LENGTH-1:1] <= per_if_cal_slave_reset_sync[PLL_RESET_SYNC_LENGTH-2:0];
         end
      end
   endgenerate
endmodule
