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
// This module is responsible for exposing control signals from/to the
// sequencer.
//
///////////////////////////////////////////////////////////////////////////////

module altera_emif_arch_nf_seq_if #(
   parameter PHY_CONFIG_ENUM                         = "",
   parameter USER_CLK_RATIO                          = 1,
   parameter REGISTER_AFI                            = 0,
   parameter PORT_AFI_RLAT_WIDTH                     = 1,
   parameter PORT_AFI_WLAT_WIDTH                     = 1,
   parameter PORT_AFI_SEQ_BUSY_WIDTH                 = 1,
   parameter PORT_HPS_EMIF_H2E_GP_WIDTH              = 1,
   parameter PORT_HPS_EMIF_E2H_GP_WIDTH              = 1,
   parameter PHY_USERMODE_OCT                        = 0,
   parameter PHY_PERIODIC_OCT_RECAL                  = 0,
   parameter PHY_HAS_DCC                             = 0,
   parameter IS_HPS                                  = 0
) (
   input  logic                                                     afi_clk,
   input  logic                                                     afi_reset_n,
   input  logic                                                     emif_usr_clk,
   input  logic                                                     emif_usr_reset_n,
   output logic                                                     afi_cal_success,
   output logic                                                     afi_cal_fail,
   input  logic                                                     afi_cal_req,
   output logic [PORT_AFI_RLAT_WIDTH-1:0]                           afi_rlat,
   output logic [PORT_AFI_WLAT_WIDTH-1:0]                           afi_wlat,
   output logic [PORT_AFI_SEQ_BUSY_WIDTH-1:0]                       afi_seq_busy,
   input  logic                                                     afi_ctl_refresh_done,
   input  logic                                                     afi_ctl_long_idle,
   output logic [16:0]                                              c2t_afi,
   input  logic [25:0]                                              t2c_afi,
   input  logic [PORT_HPS_EMIF_H2E_GP_WIDTH-1:0]                    hps_to_emif_gp,
   output logic [PORT_HPS_EMIF_E2H_GP_WIDTH-1:0]                    emif_to_hps_gp,
   output logic                                                     oct_cal_req,
   input  logic                                                     oct_cal_rdy,
   input  logic                                                     oct_recal_req,
   output logic                                                     oct_s2pload_ena,
   input  logic                                                     oct_s2pload_rdy,
   output logic                                                     dcc_stable
);
   timeunit 1ns;
   timeprecision 1ps;

   logic clk;
   logic reset_n;

   generate
      if (PHY_CONFIG_ENUM == "CONFIG_PHY_AND_HARD_CTRL") begin : hmc
         assign clk = emif_usr_clk;
         assign reset_n = emif_usr_reset_n;
      end else begin : non_hmc
         assign clk = afi_clk;
         assign reset_n = afi_reset_n;
      end
   endgenerate

   assign c2t_afi[4:0]      = '0;

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (1)
   ) afi_cal_req_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (afi_cal_req),
      .data_out (c2t_afi[8])
   );

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (4)
   ) afi_ctl_refresh_done_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  ({4{afi_ctl_refresh_done}}),
      .data_out (c2t_afi[12:9])
   );

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (4)
   ) afi_ctl_long_idle_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  ({4{afi_ctl_long_idle}}),
      .data_out (c2t_afi[16:13])
   );

   generate
      if (PHY_USERMODE_OCT == 1) begin : gen_oct_cal_rdy
         if (IS_HPS == 0) begin : gen_oct_cal_rdy_no_hps
            altera_emif_arch_nf_regs # (
               .REGISTER       (REGISTER_AFI),
               .WIDTH          (1)
            ) oct_cal_rdy_regs (
               .clk      (clk),
               .reset_n  (reset_n),
               .data_in  (oct_cal_rdy),
               .data_out (c2t_afi[7])
            );
            if (PHY_PERIODIC_OCT_RECAL == 1) begin : gen_oct_recal_rdy
               altera_emif_arch_nf_regs # (
                  .REGISTER       (REGISTER_AFI),
                  .WIDTH          (1)
               ) oct_recal_req_regs (
                  .clk      (clk),
                  .reset_n  (reset_n),
                  .data_in  (oct_recal_req),
                  .data_out (c2t_afi[6])
               );
               altera_emif_arch_nf_regs # (
                  .REGISTER       (REGISTER_AFI),
                  .WIDTH          (1)
               ) oct_s2pload_ena_regs (
                  .clk      (clk),
                  .reset_n  (reset_n),
                  .data_in  (oct_s2pload_rdy),
                  .data_out (c2t_afi[5])
               );
         end else begin : gen_no_oct_recal_rdy
               assign c2t_afi[6] = 1'b0;
               assign c2t_afi[5] = 1'b1;
            end
            assign emif_to_hps_gp[0] = 1'b0;
         end else begin : gen_oct_cal_rdy_hps
            assign c2t_afi[7] = 1'b0;
            assign c2t_afi[6] = 1'b0;
            assign c2t_afi[5] = 1'b1;
            assign emif_to_hps_gp[0] = oct_cal_rdy;
         end
      end else begin : gen_no_oct_cal_rdy
         assign c2t_afi[7] = 1'b0;
         assign c2t_afi[6] = 1'b0;
         assign c2t_afi[5] = 1'b1;
         assign emif_to_hps_gp[0] = 1'b0;
      end
   endgenerate



   logic [PORT_AFI_RLAT_WIDTH-1:0] pre_adjusted_afi_rlat;

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (6)
   ) afi_rlat_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (t2c_afi[5:0]),
      .data_out (pre_adjusted_afi_rlat)
   );

   assign afi_rlat = (REGISTER_AFI ? (pre_adjusted_afi_rlat + 2'b10) : pre_adjusted_afi_rlat);

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (6)
   ) afi_wlat_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (t2c_afi[11:6]),
      .data_out (afi_wlat)
   );

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (4)
   ) afi_seq_busy_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (t2c_afi[23:20]),
      .data_out (afi_seq_busy)
   );

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (1)
   ) afi_cal_success_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (t2c_afi[24]),
      .data_out (afi_cal_success)
   );

   altera_emif_arch_nf_regs # (
      .REGISTER       (REGISTER_AFI),
      .WIDTH          (1)
   ) afi_cal_fail_regs (
      .clk      (clk),
      .reset_n  (reset_n),
      .data_in  (t2c_afi[25]),
      .data_out (afi_cal_fail)
   );

   generate
      if (PHY_USERMODE_OCT == 1) begin : gen_oct_cal_req
         if (IS_HPS == 0) begin : gen_oct_cal_req_no_hps
            altera_emif_arch_nf_regs # (
               .REGISTER       (REGISTER_AFI),
               .WIDTH          (1)
            ) oct_cal_req_regs (
               .clk      (clk),
               .reset_n  (reset_n),
               .data_in  (t2c_afi[19]),
               .data_out (oct_cal_req)
            );
            if (PHY_PERIODIC_OCT_RECAL == 1) begin : gen_oct_recal_ena
               altera_emif_arch_nf_regs # (
                  .REGISTER       (REGISTER_AFI),
                  .WIDTH          (1)
               ) oct_s2pload_ena_regs (
                  .clk      (clk),
                  .reset_n  (reset_n),
                  .data_in  (t2c_afi[17]),
                  .data_out (oct_s2pload_ena)
               );
            end else begin : gen_no_oct_recal_ena
               assign oct_s2pload_ena = 1'b1;
            end
         end else begin : gen_oct_cal_req_hps
            assign oct_cal_req = hps_to_emif_gp[0];
            assign oct_s2pload_ena = 1'b1;
         end
      end else begin : gen_no_oct_cal_req
         assign oct_cal_req = 1'b0;
         assign oct_s2pload_ena = 1'b1;
      end
   endgenerate

   generate
      if (PHY_HAS_DCC == 1) begin : gen_has_dcc
         if (IS_HPS == 0) begin : gen_has_dcc_no_hps
            assign dcc_stable = t2c_afi[18];
         end else begin : gen_has_dcc_hps
            assign dcc_stable = 1'b1;
         end
      end else begin : gen_no_dcc
         assign dcc_stable = 1'b1;
      end
   endgenerate
endmodule
