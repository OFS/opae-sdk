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




`define _get_pin_count(_loc) ( _loc[ 9 : 0 ] )
`define _get_pin_index(_loc, _port_i) ( _loc[ (_port_i + 1) * 10 +: 10 ] )

module altera_emif_arch_nf_bufs #(
   parameter PROTOCOL_ENUM                           = "",
   parameter PINS_PER_LANE                           = 1,
   parameter PINS_IN_RTL_TILES                       = 1,
   parameter LANES_IN_RTL_TILES                      = 1,
   parameter OCT_CONTROL_WIDTH                       = 1,
   parameter DQS_BUS_MODE_ENUM                       = "",
   parameter UNUSED_MEM_PINS_PINLOC                  = 10'b0000000000,
   parameter UNUSED_DQS_BUSES_LANELOC                = 10'b0000000000,

   // Definition of port widths for "mem" interface (auto-generated)
   //AUTOGEN_BEGIN: Definition of memory port widths
   parameter PORT_MEM_CK_WIDTH                       = 1,
   parameter PORT_MEM_CK_N_WIDTH                     = 1,
   parameter PORT_MEM_DK_WIDTH                       = 1,
   parameter PORT_MEM_DK_N_WIDTH                     = 1,
   parameter PORT_MEM_DKA_WIDTH                      = 1,
   parameter PORT_MEM_DKA_N_WIDTH                    = 1,
   parameter PORT_MEM_DKB_WIDTH                      = 1,
   parameter PORT_MEM_DKB_N_WIDTH                    = 1,
   parameter PORT_MEM_K_WIDTH                        = 1,
   parameter PORT_MEM_K_N_WIDTH                      = 1,
   parameter PORT_MEM_A_WIDTH                        = 1,
   parameter PORT_MEM_BA_WIDTH                       = 1,
   parameter PORT_MEM_BG_WIDTH                       = 1,
   parameter PORT_MEM_C_WIDTH                        = 1,
   parameter PORT_MEM_CKE_WIDTH                      = 1,
   parameter PORT_MEM_CS_N_WIDTH                     = 1,
   parameter PORT_MEM_RM_WIDTH                       = 1,
   parameter PORT_MEM_ODT_WIDTH                      = 1,
   parameter PORT_MEM_RAS_N_WIDTH                    = 1,
   parameter PORT_MEM_CAS_N_WIDTH                    = 1,
   parameter PORT_MEM_WE_N_WIDTH                     = 1,
   parameter PORT_MEM_RESET_N_WIDTH                  = 1,
   parameter PORT_MEM_ACT_N_WIDTH                    = 1,
   parameter PORT_MEM_PAR_WIDTH                      = 1,
   parameter PORT_MEM_CA_WIDTH                       = 1,
   parameter PORT_MEM_REF_N_WIDTH                    = 1,
   parameter PORT_MEM_WPS_N_WIDTH                    = 1,
   parameter PORT_MEM_RPS_N_WIDTH                    = 1,
   parameter PORT_MEM_DOFF_N_WIDTH                   = 1,
   parameter PORT_MEM_LDA_N_WIDTH                    = 1,
   parameter PORT_MEM_LDB_N_WIDTH                    = 1,
   parameter PORT_MEM_RWA_N_WIDTH                    = 1,
   parameter PORT_MEM_RWB_N_WIDTH                    = 1,
   parameter PORT_MEM_LBK0_N_WIDTH                   = 1,
   parameter PORT_MEM_LBK1_N_WIDTH                   = 1,
   parameter PORT_MEM_CFG_N_WIDTH                    = 1,
   parameter PORT_MEM_AP_WIDTH                       = 1,
   parameter PORT_MEM_AINV_WIDTH                     = 1,
   parameter PORT_MEM_DM_WIDTH                       = 1,
   parameter PORT_MEM_BWS_N_WIDTH                    = 1,
   parameter PORT_MEM_D_WIDTH                        = 1,
   parameter PORT_MEM_DQ_WIDTH                       = 1,
   parameter PORT_MEM_DBI_N_WIDTH                    = 1,
   parameter PORT_MEM_DQA_WIDTH                      = 1,
   parameter PORT_MEM_DQB_WIDTH                      = 1,
   parameter PORT_MEM_DINVA_WIDTH                    = 1,
   parameter PORT_MEM_DINVB_WIDTH                    = 1,
   parameter PORT_MEM_Q_WIDTH                        = 1,
   parameter PORT_MEM_DQS_WIDTH                      = 1,
   parameter PORT_MEM_DQS_N_WIDTH                    = 1,
   parameter PORT_MEM_QK_WIDTH                       = 1,
   parameter PORT_MEM_QK_N_WIDTH                     = 1,
   parameter PORT_MEM_QKA_WIDTH                      = 1,
   parameter PORT_MEM_QKA_N_WIDTH                    = 1,
   parameter PORT_MEM_QKB_WIDTH                      = 1,
   parameter PORT_MEM_QKB_N_WIDTH                    = 1,
   parameter PORT_MEM_CQ_WIDTH                       = 1,
   parameter PORT_MEM_CQ_N_WIDTH                     = 1,
   parameter PORT_MEM_ALERT_N_WIDTH                  = 1,
   parameter PORT_MEM_PE_N_WIDTH                     = 1,

   // Definition of parameters describing logical pin allocation
   //AUTOGEN_BEGIN: Definition of memory port pinlocs
   parameter PORT_MEM_CK_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_CK_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_DK_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_DK_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_DKA_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_DKA_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_DKB_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_DKB_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_K_PINLOC                       = 10'b0000000000,
   parameter PORT_MEM_K_N_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_A_PINLOC                       = 10'b0000000000,
   parameter PORT_MEM_BA_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_BG_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_C_PINLOC                       = 10'b0000000000,
   parameter PORT_MEM_CKE_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_CS_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_RM_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_ODT_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_RAS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_CAS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_WE_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_RESET_N_PINLOC                 = 10'b0000000000,
   parameter PORT_MEM_ACT_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_PAR_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_CA_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_REF_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_WPS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_RPS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_DOFF_N_PINLOC                  = 10'b0000000000,
   parameter PORT_MEM_LDA_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_LDB_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_RWA_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_RWB_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_LBK0_N_PINLOC                  = 10'b0000000000,
   parameter PORT_MEM_LBK1_N_PINLOC                  = 10'b0000000000,
   parameter PORT_MEM_CFG_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_AP_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_AINV_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_DM_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_BWS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_D_PINLOC                       = 10'b0000000000,
   parameter PORT_MEM_DQ_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_DBI_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_DQA_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_DQB_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_DINVA_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_DINVB_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_Q_PINLOC                       = 10'b0000000000,
   parameter PORT_MEM_DQS_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_DQS_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_QK_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_QK_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_QKA_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_QKA_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_QKB_PINLOC                     = 10'b0000000000,
   parameter PORT_MEM_QKB_N_PINLOC                   = 10'b0000000000,
   parameter PORT_MEM_CQ_PINLOC                      = 10'b0000000000,
   parameter PORT_MEM_CQ_N_PINLOC                    = 10'b0000000000,
   parameter PORT_MEM_ALERT_N_PINLOC                 = 10'b0000000000,
   parameter PORT_MEM_PE_N_PINLOC                    = 10'b0000000000,

   parameter PHY_CALIBRATED_OCT                      = 1,
   parameter PHY_AC_CALIBRATED_OCT                   = 1,
   parameter PHY_CK_CALIBRATED_OCT                   = 1,
   parameter PHY_DATA_CALIBRATED_OCT                 = 1
) (
   input  logic [PINS_IN_RTL_TILES-1:0]                       l2b_data,
   input  logic [PINS_IN_RTL_TILES-1:0]                       l2b_oe,
   input  logic [PINS_IN_RTL_TILES-1:0]                       l2b_dtc,
   output logic [PINS_IN_RTL_TILES-1:0]                       b2l_data,
   output logic [LANES_IN_RTL_TILES-1:0]                      b2t_dqs,
   output logic [LANES_IN_RTL_TILES-1:0]                      b2t_dqsb,

   // Ports for "mem" interface
   //AUTOGEN_BEGIN: Definition of memory ports
   output logic [PORT_MEM_CK_WIDTH-1:0]                       mem_ck,
   output logic [PORT_MEM_CK_N_WIDTH-1:0]                     mem_ck_n,
   output logic [PORT_MEM_DK_WIDTH-1:0]                       mem_dk,
   output logic [PORT_MEM_DK_N_WIDTH-1:0]                     mem_dk_n,
   output logic [PORT_MEM_DKA_WIDTH-1:0]                      mem_dka,
   output logic [PORT_MEM_DKA_N_WIDTH-1:0]                    mem_dka_n,
   output logic [PORT_MEM_DKB_WIDTH-1:0]                      mem_dkb,
   output logic [PORT_MEM_DKB_N_WIDTH-1:0]                    mem_dkb_n,
   output logic [PORT_MEM_K_WIDTH-1:0]                        mem_k,
   output logic [PORT_MEM_K_N_WIDTH-1:0]                      mem_k_n,
   output logic [PORT_MEM_A_WIDTH-1:0]                        mem_a,
   output logic [PORT_MEM_BA_WIDTH-1:0]                       mem_ba,
   output logic [PORT_MEM_BG_WIDTH-1:0]                       mem_bg,
   output logic [PORT_MEM_C_WIDTH-1:0]                        mem_c,
   output logic [PORT_MEM_CKE_WIDTH-1:0]                      mem_cke,
   output logic [PORT_MEM_CS_N_WIDTH-1:0]                     mem_cs_n,
   output logic [PORT_MEM_RM_WIDTH-1:0]                       mem_rm,
   output logic [PORT_MEM_ODT_WIDTH-1:0]                      mem_odt,
   output logic [PORT_MEM_RAS_N_WIDTH-1:0]                    mem_ras_n,
   output logic [PORT_MEM_CAS_N_WIDTH-1:0]                    mem_cas_n,
   output logic [PORT_MEM_WE_N_WIDTH-1:0]                     mem_we_n,
   output logic [PORT_MEM_RESET_N_WIDTH-1:0]                  mem_reset_n,
   output logic [PORT_MEM_ACT_N_WIDTH-1:0]                    mem_act_n,
   output logic [PORT_MEM_PAR_WIDTH-1:0]                      mem_par,
   output logic [PORT_MEM_CA_WIDTH-1:0]                       mem_ca,
   output logic [PORT_MEM_REF_N_WIDTH-1:0]                    mem_ref_n,
   output logic [PORT_MEM_WPS_N_WIDTH-1:0]                    mem_wps_n,
   output logic [PORT_MEM_RPS_N_WIDTH-1:0]                    mem_rps_n,
   output logic [PORT_MEM_DOFF_N_WIDTH-1:0]                   mem_doff_n,
   output logic [PORT_MEM_LDA_N_WIDTH-1:0]                    mem_lda_n,
   output logic [PORT_MEM_LDB_N_WIDTH-1:0]                    mem_ldb_n,
   output logic [PORT_MEM_RWA_N_WIDTH-1:0]                    mem_rwa_n,
   output logic [PORT_MEM_RWB_N_WIDTH-1:0]                    mem_rwb_n,
   output logic [PORT_MEM_LBK0_N_WIDTH-1:0]                   mem_lbk0_n,
   output logic [PORT_MEM_LBK1_N_WIDTH-1:0]                   mem_lbk1_n,
   output logic [PORT_MEM_CFG_N_WIDTH-1:0]                    mem_cfg_n,
   output logic [PORT_MEM_AP_WIDTH-1:0]                       mem_ap,
   output logic [PORT_MEM_AINV_WIDTH-1:0]                     mem_ainv,
   output logic [PORT_MEM_DM_WIDTH-1:0]                       mem_dm,
   output logic [PORT_MEM_BWS_N_WIDTH-1:0]                    mem_bws_n,
   output logic [PORT_MEM_D_WIDTH-1:0]                        mem_d,
   inout  tri   [PORT_MEM_DQ_WIDTH-1:0]                       mem_dq,
   inout  tri   [PORT_MEM_DBI_N_WIDTH-1:0]                    mem_dbi_n,
   inout  tri   [PORT_MEM_DQA_WIDTH-1:0]                      mem_dqa,
   inout  tri   [PORT_MEM_DQB_WIDTH-1:0]                      mem_dqb,
   inout  tri   [PORT_MEM_DINVA_WIDTH-1:0]                    mem_dinva,
   inout  tri   [PORT_MEM_DINVB_WIDTH-1:0]                    mem_dinvb,
   input  logic [PORT_MEM_Q_WIDTH-1:0]                        mem_q,
   inout  tri   [PORT_MEM_DQS_WIDTH-1:0]                      mem_dqs,
   inout  tri   [PORT_MEM_DQS_N_WIDTH-1:0]                    mem_dqs_n,
   input  logic [PORT_MEM_QK_WIDTH-1:0]                       mem_qk,
   input  logic [PORT_MEM_QK_N_WIDTH-1:0]                     mem_qk_n,
   input  logic [PORT_MEM_QKA_WIDTH-1:0]                      mem_qka,
   input  logic [PORT_MEM_QKA_N_WIDTH-1:0]                    mem_qka_n,
   input  logic [PORT_MEM_QKB_WIDTH-1:0]                      mem_qkb,
   input  logic [PORT_MEM_QKB_N_WIDTH-1:0]                    mem_qkb_n,
   input  logic [PORT_MEM_CQ_WIDTH-1:0]                       mem_cq,
   input  logic [PORT_MEM_CQ_N_WIDTH-1:0]                     mem_cq_n,
   input  logic [PORT_MEM_ALERT_N_WIDTH-1:0]                  mem_alert_n,
   input  logic [PORT_MEM_PE_N_WIDTH-1:0]                     mem_pe_n,

   input  logic [OCT_CONTROL_WIDTH-1:0]                       oct_stc,
   input  logic [OCT_CONTROL_WIDTH-1:0]                       oct_ptc
);
   timeunit 1ns;
   timeprecision 1ps;

   generate
      genvar port_i;

      for (port_i = 0; port_i < `_get_pin_count(UNUSED_MEM_PINS_PINLOC); ++port_i)
      begin : unused_pin
         altera_emif_arch_nf_buf_unused ub (.o(b2l_data[`_get_pin_index(UNUSED_MEM_PINS_PINLOC, port_i)]));
      end

      for (port_i = 0; port_i < `_get_pin_count(UNUSED_DQS_BUSES_LANELOC); ++port_i)
      begin : unused_dqs_bus
         altera_emif_arch_nf_buf_unused ub0 (.o(b2t_dqs[`_get_pin_index(UNUSED_DQS_BUSES_LANELOC, port_i)]));
         altera_emif_arch_nf_buf_unused ub1 (.o(b2t_dqsb[`_get_pin_index(UNUSED_DQS_BUSES_LANELOC, port_i)]));
      end

      if (`_get_pin_count(PORT_MEM_CK_PINLOC) != 0) begin : gen_mem_ck
         for (port_i = 0; port_i < PORT_MEM_CK_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_df_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_CK_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CK_PINLOC, port_i)]),
               .ibar(l2b_data[`_get_pin_index(PORT_MEM_CK_N_PINLOC, port_i)]),
               .o(mem_ck[port_i]),
               .obar(mem_ck_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ub0 (.o(b2l_data[`_get_pin_index(PORT_MEM_CK_PINLOC, port_i)]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_CK_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_ck
         assign {mem_ck, mem_ck_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_DK_PINLOC) != 0) begin : gen_mem_dk
         for (port_i = 0; port_i < PORT_MEM_DK_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_df_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_DK_PINLOC, port_i)]),
               .ibar(l2b_data[`_get_pin_index(PORT_MEM_DK_N_PINLOC, port_i)]),
               .o(mem_dk[port_i]),
               .obar(mem_dk_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ub0 (.o(b2l_data[`_get_pin_index(PORT_MEM_DK_PINLOC, port_i)]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_DK_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_dk
         assign {mem_dk, mem_dk_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_DKA_PINLOC) != 0) begin : gen_mem_dka
         for (port_i = 0; port_i < PORT_MEM_DKA_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_df_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_DKA_PINLOC, port_i)]),
               .ibar(l2b_data[`_get_pin_index(PORT_MEM_DKA_N_PINLOC, port_i)]),
               .o(mem_dka[port_i]),
               .obar(mem_dka_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ub0 (.o(b2l_data[`_get_pin_index(PORT_MEM_DKA_PINLOC, port_i)]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_DKA_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_dka
         assign {mem_dka, mem_dka_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_DKB_PINLOC) != 0) begin : gen_mem_dkb
         for (port_i = 0; port_i < PORT_MEM_DKB_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_df_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_DKB_PINLOC, port_i)]),
               .ibar(l2b_data[`_get_pin_index(PORT_MEM_DKB_N_PINLOC, port_i)]),
               .o(mem_dkb[port_i]),
               .obar(mem_dkb_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ub0 (.o(b2l_data[`_get_pin_index(PORT_MEM_DKB_PINLOC, port_i)]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_DKB_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_dkb
         assign {mem_dkb, mem_dkb_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_K_PINLOC) != 0) begin : gen_mem_k
         for (port_i = 0; port_i < PORT_MEM_K_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_df_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_CK_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_K_PINLOC, port_i)]),
               .ibar(l2b_data[`_get_pin_index(PORT_MEM_K_N_PINLOC, port_i)]),
               .o(mem_k[port_i]),
               .obar(mem_k_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ub0 (.o(b2l_data[`_get_pin_index(PORT_MEM_K_PINLOC, port_i)]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_K_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_k
         assign {mem_k, mem_k_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_A_PINLOC) != 0) begin : gen_mem_a
         for (port_i = 0; port_i < PORT_MEM_A_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_A_PINLOC, port_i)]),
               .o(mem_a[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_A_PINLOC, port_i)]));
         end
      end else begin : no_mem_a
         assign mem_a = '0;
      end

      if (`_get_pin_count(PORT_MEM_BA_PINLOC) != 0) begin : gen_mem_ba
         for (port_i = 0; port_i < PORT_MEM_BA_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_BA_PINLOC, port_i)]),
               .o(mem_ba[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_BA_PINLOC, port_i)]));
         end
      end else begin : no_mem_ba
         assign mem_ba = '0;
      end

      if (`_get_pin_count(PORT_MEM_BG_PINLOC) != 0) begin : gen_mem_bg
         for (port_i = 0; port_i < PORT_MEM_BG_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_BG_PINLOC, port_i)]),
               .o(mem_bg[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_BG_PINLOC, port_i)]));
         end
      end else begin : no_mem_bg
         assign mem_bg = '0;
      end

      if (`_get_pin_count(PORT_MEM_C_PINLOC) != 0) begin : gen_mem_c
         for (port_i = 0; port_i < PORT_MEM_C_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_C_PINLOC, port_i)]),
               .o(mem_c[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_C_PINLOC, port_i)]));
         end
      end else begin : no_mem_c
         assign mem_c = '0;
      end

      if (`_get_pin_count(PORT_MEM_CKE_PINLOC) != 0) begin : gen_mem_cke
         for (port_i = 0; port_i < PORT_MEM_CKE_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CKE_PINLOC, port_i)]),
               .o(mem_cke[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_CKE_PINLOC, port_i)]));
         end
      end else begin : no_mem_cke
         assign mem_cke = '0;
      end

      if (`_get_pin_count(PORT_MEM_CS_N_PINLOC) != 0) begin : gen_mem_cs_n
         for (port_i = 0; port_i < PORT_MEM_CS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CS_N_PINLOC, port_i)]),
               .o(mem_cs_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_CS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_cs_n
         assign mem_cs_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_RM_PINLOC) != 0) begin : gen_mem_rm
         for (port_i = 0; port_i < PORT_MEM_RM_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RM_PINLOC, port_i)]),
               .o(mem_rm[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RM_PINLOC, port_i)]));
         end
      end else begin : no_mem_rm
         assign mem_rm = '1;
      end

      if (`_get_pin_count(PORT_MEM_ODT_PINLOC) != 0) begin : gen_mem_odt
         for (port_i = 0; port_i < PORT_MEM_ODT_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_ODT_PINLOC, port_i)]),
               .o(mem_odt[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_ODT_PINLOC, port_i)]));
         end
      end else begin : no_mem_odt
         assign mem_odt = '0;
      end

      if (`_get_pin_count(PORT_MEM_RAS_N_PINLOC) != 0) begin : gen_mem_ras_n
         for (port_i = 0; port_i < PORT_MEM_RAS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RAS_N_PINLOC, port_i)]),
               .o(mem_ras_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RAS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_ras_n
         assign mem_ras_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_CAS_N_PINLOC) != 0) begin : gen_mem_cas_n
         for (port_i = 0; port_i < PORT_MEM_CAS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CAS_N_PINLOC, port_i)]),
               .o(mem_cas_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_CAS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_cas_n
         assign mem_cas_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_WE_N_PINLOC) != 0) begin : gen_mem_we_n
         for (port_i = 0; port_i < PORT_MEM_WE_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_WE_N_PINLOC, port_i)]),
               .o(mem_we_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_WE_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_we_n
         assign mem_we_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_RESET_N_PINLOC) != 0) begin : gen_mem_reset_n
         for (port_i = 0; port_i < PORT_MEM_RESET_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(0)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RESET_N_PINLOC, port_i)]),
               .o(mem_reset_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RESET_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_reset_n
         assign mem_reset_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_ACT_N_PINLOC) != 0) begin : gen_mem_act_n
         for (port_i = 0; port_i < PORT_MEM_ACT_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_ACT_N_PINLOC, port_i)]),
               .o(mem_act_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_ACT_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_act_n
         assign mem_act_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_PAR_PINLOC) != 0) begin : gen_mem_par
         for (port_i = 0; port_i < PORT_MEM_PAR_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_PAR_PINLOC, port_i)]),
               .o(mem_par[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_PAR_PINLOC, port_i)]));
         end
      end else begin : no_mem_par
         assign mem_par = '0;
      end

      if (`_get_pin_count(PORT_MEM_CA_PINLOC) != 0) begin : gen_mem_ca
         for (port_i = 0; port_i < PORT_MEM_CA_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CA_PINLOC, port_i)]),
               .o(mem_ca[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_CA_PINLOC, port_i)]));
         end
      end else begin : no_mem_ca
         assign mem_ca = '0;
      end

      if (`_get_pin_count(PORT_MEM_REF_N_PINLOC) != 0) begin : gen_mem_ref_n
         for (port_i = 0; port_i < PORT_MEM_REF_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_REF_N_PINLOC, port_i)]),
               .o(mem_ref_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_REF_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_ref_n
         assign mem_ref_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_WPS_N_PINLOC) != 0) begin : gen_mem_wps_n
         for (port_i = 0; port_i < PORT_MEM_WPS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_WPS_N_PINLOC, port_i)]),
               .o(mem_wps_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_WPS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_wps_n
         assign mem_wps_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_RPS_N_PINLOC) != 0) begin : gen_mem_rps_n
         for (port_i = 0; port_i < PORT_MEM_RPS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RPS_N_PINLOC, port_i)]),
               .o(mem_rps_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RPS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_rps_n
         assign mem_rps_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_LDA_N_PINLOC) != 0) begin : gen_mem_lda_n
         for (port_i = 0; port_i < PORT_MEM_LDA_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_LDA_N_PINLOC, port_i)]),
               .o(mem_lda_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_LDA_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_lda_n
         assign mem_lda_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_LDB_N_PINLOC) != 0) begin : gen_mem_ldb_n
         for (port_i = 0; port_i < PORT_MEM_LDB_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_LDB_N_PINLOC, port_i)]),
               .o(mem_ldb_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_LDB_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_ldb_n
         assign mem_ldb_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_RWA_N_PINLOC) != 0) begin : gen_mem_rwa_n
         for (port_i = 0; port_i < PORT_MEM_RWA_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RWA_N_PINLOC, port_i)]),
               .o(mem_rwa_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RWA_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_rwa_n
         assign mem_rwa_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_RWB_N_PINLOC) != 0) begin : gen_mem_rwb_n
         for (port_i = 0; port_i < PORT_MEM_RWB_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_RWB_N_PINLOC, port_i)]),
               .o(mem_rwb_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_RWB_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_rwb_n
         assign mem_rwb_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_LBK0_N_PINLOC) != 0) begin : gen_mem_lbk0_n
         for (port_i = 0; port_i < PORT_MEM_LBK0_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_LBK0_N_PINLOC, port_i)]),
               .o(mem_lbk0_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_LBK0_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_lbk0_n
         assign mem_lbk0_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_LBK1_N_PINLOC) != 0) begin : gen_mem_lbk1_n
         for (port_i = 0; port_i < PORT_MEM_LBK1_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_LBK1_N_PINLOC, port_i)]),
               .o(mem_lbk1_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_LBK1_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_lbk1_n
         assign mem_lbk1_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_AP_PINLOC) != 0) begin : gen_mem_ap
         for (port_i = 0; port_i < PORT_MEM_AP_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_AP_PINLOC, port_i)]),
               .o(mem_ap[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_AP_PINLOC, port_i)]));
         end
      end else begin : no_mem_ap
         assign mem_ap = '1;
      end

      if (`_get_pin_count(PORT_MEM_AINV_PINLOC) != 0) begin : gen_mem_ainv
         for (port_i = 0; port_i < PORT_MEM_AINV_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_AINV_PINLOC, port_i)]),
               .o(mem_ainv[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_AINV_PINLOC, port_i)]));
         end
      end else begin : no_mem_ainv
         assign mem_ainv = '1;
      end

      if (`_get_pin_count(PORT_MEM_CFG_N_PINLOC) != 0) begin : gen_mem_cfg_n
         for (port_i = 0; port_i < PORT_MEM_CFG_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_CFG_N_PINLOC, port_i)]),
               .o(mem_cfg_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_CFG_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_cfg_n
         assign mem_cfg_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_DOFF_N_PINLOC) != 0) begin : gen_mem_doff_n
         for (port_i = 0; port_i < PORT_MEM_DOFF_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_AC_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_DOFF_N_PINLOC, port_i)]),
               .o(mem_doff_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_DOFF_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_doff_n
         assign mem_doff_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_DM_PINLOC) != 0) begin : gen_mem_dm
         for (port_i = 0; port_i < PORT_MEM_DM_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_DM_PINLOC, port_i)]),
               .o(mem_dm[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_DM_PINLOC, port_i)]));
         end
      end else begin : no_mem_dm
         assign mem_dm = '0;
      end

      if (`_get_pin_count(PORT_MEM_BWS_N_PINLOC) != 0) begin : gen_mem_bws_n
         for (port_i = 0; port_i < PORT_MEM_BWS_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_BWS_N_PINLOC, port_i)]),
               .o(mem_bws_n[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_BWS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_bws_n
         assign mem_bws_n = '1;
      end

      if (`_get_pin_count(PORT_MEM_D_PINLOC) != 0) begin : gen_mem_d
         for (port_i = 0; port_i < PORT_MEM_D_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_o # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(l2b_data[`_get_pin_index(PORT_MEM_D_PINLOC, port_i)]),
               .o(mem_d[port_i]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            altera_emif_arch_nf_buf_unused ubuf (.o(b2l_data[`_get_pin_index(PORT_MEM_D_PINLOC, port_i)]));
         end
      end else begin : no_mem_d
         assign mem_d = '0;
      end

      if (`_get_pin_count(PORT_MEM_DQ_PINLOC) != 0) begin : gen_mem_dq
         for (port_i = 0; port_i < PORT_MEM_DQ_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dq[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DQ_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DQ_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DQ_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DQ_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dq
         assign mem_dq = '0;
      end

      if (`_get_pin_count(PORT_MEM_DBI_N_PINLOC) != 0) begin : gen_mem_dbi_n
         for (port_i = 0; port_i < PORT_MEM_DBI_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dbi_n[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DBI_N_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DBI_N_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DBI_N_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DBI_N_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dbi_n
         assign mem_dbi_n = '0;
      end

      if (`_get_pin_count(PORT_MEM_DQA_PINLOC) != 0) begin : gen_mem_dqa
         for (port_i = 0; port_i < PORT_MEM_DQA_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dqa[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DQA_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DQA_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DQA_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DQA_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dqa
         assign mem_dqa = '0;
      end

      if (`_get_pin_count(PORT_MEM_DQB_PINLOC) != 0) begin : gen_mem_dqb
         for (port_i = 0; port_i < PORT_MEM_DQB_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dqb[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DQB_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DQB_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DQB_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DQB_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dqb
         assign mem_dqb = '0;
      end

      if (`_get_pin_count(PORT_MEM_DINVA_PINLOC) != 0) begin : gen_mem_dinva
         for (port_i = 0; port_i < PORT_MEM_DINVA_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dinva[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DINVA_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DINVA_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DINVA_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DINVA_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dinva
         assign mem_dinva = '0;
      end

      if (`_get_pin_count(PORT_MEM_DINVB_PINLOC) != 0) begin : gen_mem_dinvb
         for (port_i = 0; port_i < PORT_MEM_DINVB_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_bdir_se # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dinvb[port_i]),
               .ibuf_o(b2l_data[`_get_pin_index(PORT_MEM_DINVB_PINLOC, port_i)]),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DINVB_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DINVB_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DINVB_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end else begin : no_mem_dinvb
         assign mem_dinvb = '0;
      end

      if (`_get_pin_count(PORT_MEM_Q_PINLOC) != 0) begin : gen_mem_q
         for (port_i = 0; port_i < PORT_MEM_Q_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_q[port_i]),
               .o(b2l_data[`_get_pin_index(PORT_MEM_Q_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end

      if (`_get_pin_count(PORT_MEM_ALERT_N_PINLOC) != 0) begin : gen_mem_alert_n
         for (port_i = 0; port_i < PORT_MEM_ALERT_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT((PROTOCOL_ENUM == "PROTOCOL_DDR4") ? 0 : PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_alert_n[port_i]),
               .o(b2l_data[`_get_pin_index(PORT_MEM_ALERT_N_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end

      if (`_get_pin_count(PORT_MEM_PE_N_PINLOC) != 0) begin : gen_mem_pe_n
         for (port_i = 0; port_i < PORT_MEM_PE_N_WIDTH; ++port_i)
         begin : inst
            altera_emif_arch_nf_buf_udir_se_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_pe_n[port_i]),
               .o(b2l_data[`_get_pin_index(PORT_MEM_PE_N_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );
         end
      end

      if (`_get_pin_count(PORT_MEM_DQS_PINLOC) != 0) begin : gen_mem_dqs
         for (port_i = 0; port_i < PORT_MEM_DQS_WIDTH; ++port_i)
         begin : inst
            logic sig;

            altera_emif_arch_nf_buf_bdir_df # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .io(mem_dqs[port_i]),
               .iobar(mem_dqs_n[port_i]),
               .ibuf_o(sig),
               .obuf_i(l2b_data[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i)]),
               .obuf_ibar(l2b_data[`_get_pin_index(PORT_MEM_DQS_N_PINLOC, port_i)]),
               .obuf_oe(l2b_oe[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i)]),
               .obuf_oebar(l2b_oe[`_get_pin_index(PORT_MEM_DQS_N_PINLOC, port_i)]),
               .obuf_dtc(l2b_dtc[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i)]),
               .obuf_dtcbar(l2b_dtc[`_get_pin_index(PORT_MEM_DQS_N_PINLOC, port_i)]),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            if (DQS_BUS_MODE_ENUM == "DQS_BUS_MODE_X4") begin : gen_x4
               if ((`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i) % PINS_PER_LANE) < (PINS_PER_LANE / 2)) begin : a
                  assign b2t_dqs[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i) / PINS_PER_LANE] = sig;
               end else begin : b
                  assign b2t_dqsb[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i) / PINS_PER_LANE] = sig;
               end
            end else begin : gen_x8
               assign b2t_dqs[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i) / PINS_PER_LANE] = sig;
               altera_emif_arch_nf_buf_unused ub0 (.o(b2t_dqsb[`_get_pin_index(PORT_MEM_DQS_N_PINLOC, port_i) / PINS_PER_LANE]));
            end

            assign b2l_data[`_get_pin_index(PORT_MEM_DQS_PINLOC, port_i)] = sig;
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_DQS_N_PINLOC, port_i)]));
         end
      end else begin : no_mem_dqs
         assign {mem_dqs, mem_dqs_n} = '0;
      end

      if (`_get_pin_count(PORT_MEM_QK_PINLOC) != 0) begin : gen_mem_qk
         for (port_i = 0; port_i < PORT_MEM_QK_WIDTH; ++port_i)
         begin : inst
            logic sig;

            altera_emif_arch_nf_buf_udir_df_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_qk[port_i]),
               .ibar(mem_qk_n[port_i]),
               .o(sig),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            assign b2t_dqs[`_get_pin_index(PORT_MEM_QK_PINLOC, port_i) / PINS_PER_LANE] = sig;
            assign b2l_data[`_get_pin_index(PORT_MEM_QK_PINLOC, port_i)] = sig;

            altera_emif_arch_nf_buf_unused ub0 (.o(b2t_dqsb[`_get_pin_index(PORT_MEM_QK_N_PINLOC, port_i) / PINS_PER_LANE]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_QK_N_PINLOC, port_i)]));
         end
      end

      if (`_get_pin_count(PORT_MEM_QKA_PINLOC) != 0) begin : gen_mem_qka
         for (port_i = 0; port_i < PORT_MEM_QKA_WIDTH; ++port_i)
         begin : inst
            logic sig;

            altera_emif_arch_nf_buf_udir_df_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_qka[port_i]),
               .ibar(mem_qka_n[port_i]),
               .o(sig),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            assign b2t_dqs[`_get_pin_index(PORT_MEM_QKA_PINLOC, port_i) / PINS_PER_LANE] = sig;
            assign b2l_data[`_get_pin_index(PORT_MEM_QKA_PINLOC, port_i)] = sig;

            altera_emif_arch_nf_buf_unused ub0 (.o(b2t_dqsb[`_get_pin_index(PORT_MEM_QKA_N_PINLOC, port_i) / PINS_PER_LANE]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_QKA_N_PINLOC, port_i)]));
         end
      end

      if (`_get_pin_count(PORT_MEM_QKB_PINLOC) != 0) begin : gen_mem_qkb
         for (port_i = 0; port_i < PORT_MEM_QKB_WIDTH; ++port_i)
         begin : inst
            logic sig;

            // For QDR-IV, connect mem_qkb to the negative input of the differential buffer,
            // and qkb_n to the positive input, to reverse the polarity of QKB. This is required
            // for proper capture of DQB data.
            altera_emif_arch_nf_buf_udir_df_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_qkb_n[port_i]),
               .ibar(mem_qkb[port_i]),
               .o(sig),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            assign b2t_dqs[`_get_pin_index(PORT_MEM_QKB_PINLOC, port_i) / PINS_PER_LANE] = sig;
            assign b2l_data[`_get_pin_index(PORT_MEM_QKB_PINLOC, port_i)] = sig;

            altera_emif_arch_nf_buf_unused ub0 (.o(b2t_dqsb[`_get_pin_index(PORT_MEM_QKB_N_PINLOC, port_i) / PINS_PER_LANE]));
            altera_emif_arch_nf_buf_unused ub1 (.o(b2l_data[`_get_pin_index(PORT_MEM_QKB_N_PINLOC, port_i)]));
         end
      end

      if (`_get_pin_count(PORT_MEM_CQ_PINLOC) != 0) begin : gen_mem_cq
         for (port_i = 0; port_i < PORT_MEM_CQ_WIDTH; ++port_i)
         begin : inst
            logic sig_p;
            logic sig_n;

            altera_emif_arch_nf_buf_udir_cp_i # (
               .OCT_CONTROL_WIDTH(OCT_CONTROL_WIDTH),
               .CALIBRATED_OCT(PHY_DATA_CALIBRATED_OCT)
            ) b (
               .i(mem_cq[port_i]),
               .ibar(mem_cq_n[port_i]),
               .o(sig_p),
               .obar(sig_n),
               .oct_stc(oct_stc),
               .oct_ptc(oct_ptc)
            );

            assign b2t_dqs[`_get_pin_index(PORT_MEM_CQ_PINLOC, port_i) / PINS_PER_LANE] = sig_p;
            assign b2t_dqsb[`_get_pin_index(PORT_MEM_CQ_N_PINLOC, port_i) / PINS_PER_LANE] = sig_n;

            assign b2l_data[`_get_pin_index(PORT_MEM_CQ_PINLOC, port_i)] = sig_p;
            assign b2l_data[`_get_pin_index(PORT_MEM_CQ_N_PINLOC, port_i)] = sig_n;
         end
      end
   endgenerate
endmodule
