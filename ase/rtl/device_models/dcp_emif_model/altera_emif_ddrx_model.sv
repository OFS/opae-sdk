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



///////////////////////////////////////////////////////////////////////////////
// Top-level wrapper of memory model
//
///////////////////////////////////////////////////////////////////////////////
module altera_emif_ddrx_model # (
   parameter PROTOCOL_ENUM                           = "",
   parameter MEM_FORMAT_ENUM                         = "",
   parameter PHY_PING_PONG_EN                        = 0,
   parameter MEM_RANKS_PER_DIMM                      = 0,
   parameter MEM_NUM_OF_DIMMS                        = 0,
   parameter MEM_DM_EN                               = 0,
   parameter MEM_AC_PAR_EN                           = 0,

   parameter MEM_DISCRETE_CS_WIDTH                   = 1,
   parameter MEM_CHIP_ID_WIDTH                       = 0,
   parameter MEM_ROW_ADDR_WIDTH                      = 1,
   parameter MEM_COL_ADDR_WIDTH                      = 1,

   parameter MEM_TRTP                                = 0,
   parameter MEM_TRCD                                = 0,
   parameter MEM_DISCRETE_MIRROR_ADDRESSING_EN       = 0,
   parameter MEM_MIRROR_ADDRESSING_EN                = 0,
   parameter MEM_INIT_MRS0                           = 0,
   parameter MEM_INIT_MRS1                           = 0,
   parameter MEM_INIT_MRS2                           = 0,
   parameter MEM_INIT_MRS3                           = 0,
   parameter MEM_CFG_GEN_SBE                         = 0,
   parameter MEM_CFG_GEN_DBE                         = 0,
   parameter MEM_CLK_FREQUENCY                       = 0,

   parameter MEM_MICRON_AUTOMATA                     = 0,

   // Definition of port widths for "mem" interface
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
   parameter PORT_MEM_PE_N_WIDTH                     = 1,
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
   parameter PORT_MEM_ALERT_N_WIDTH                  = 1,
   parameter PORT_MEM_DQS_WIDTH                      = 1,
   parameter PORT_MEM_DQS_N_WIDTH                    = 1,
   parameter PORT_MEM_QK_WIDTH                       = 1,
   parameter PORT_MEM_QK_N_WIDTH                     = 1,
   parameter PORT_MEM_QKA_WIDTH                      = 1,
   parameter PORT_MEM_QKA_N_WIDTH                    = 1,
   parameter PORT_MEM_QKB_WIDTH                      = 1,
   parameter PORT_MEM_QKB_N_WIDTH                    = 1,
   parameter PORT_MEM_CQ_WIDTH                       = 1,
   parameter PORT_MEM_CQ_N_WIDTH                     = 1
) (
   // Ports for "mem" interface
   input  logic [PORT_MEM_CK_WIDTH-1:0]                       mem_ck,
   input  logic [PORT_MEM_CK_N_WIDTH-1:0]                     mem_ck_n,
   input  logic [PORT_MEM_DK_WIDTH-1:0]                       mem_dk,
   input  logic [PORT_MEM_DK_N_WIDTH-1:0]                     mem_dk_n,
   input  logic [PORT_MEM_DKA_WIDTH-1:0]                      mem_dka,
   input  logic [PORT_MEM_DKA_N_WIDTH-1:0]                    mem_dka_n,
   input  logic [PORT_MEM_DKB_WIDTH-1:0]                      mem_dkb,
   input  logic [PORT_MEM_DKB_N_WIDTH-1:0]                    mem_dkb_n,
   input  logic [PORT_MEM_K_WIDTH-1:0]                        mem_k,
   input  logic [PORT_MEM_K_N_WIDTH-1:0]                      mem_k_n,
   input  logic [PORT_MEM_A_WIDTH-1:0]                        mem_a,
   input  logic [PORT_MEM_BA_WIDTH-1:0]                       mem_ba,
   input  logic [PORT_MEM_BG_WIDTH-1:0]                       mem_bg,
   input  logic [PORT_MEM_C_WIDTH-1:0]                        mem_c,
   input  logic [PORT_MEM_CKE_WIDTH-1:0]                      mem_cke,
   input  logic [PORT_MEM_CS_N_WIDTH-1:0]                     mem_cs_n,
   input  logic [PORT_MEM_RM_WIDTH-1:0]                       mem_rm,
   input  logic [PORT_MEM_ODT_WIDTH-1:0]                      mem_odt,
   input  logic [PORT_MEM_RAS_N_WIDTH-1:0]                    mem_ras_n,
   input  logic [PORT_MEM_CAS_N_WIDTH-1:0]                    mem_cas_n,
   input  logic [PORT_MEM_WE_N_WIDTH-1:0]                     mem_we_n,
   input  logic [PORT_MEM_RESET_N_WIDTH-1:0]                  mem_reset_n,
   input  logic [PORT_MEM_ACT_N_WIDTH-1:0]                    mem_act_n,
   input  logic [PORT_MEM_PAR_WIDTH-1:0]                      mem_par,
   input  logic [PORT_MEM_CA_WIDTH-1:0]                       mem_ca,
   input  logic [PORT_MEM_REF_N_WIDTH-1:0]                    mem_ref_n,
   input  logic [PORT_MEM_WPS_N_WIDTH-1:0]                    mem_wps_n,
   input  logic [PORT_MEM_RPS_N_WIDTH-1:0]                    mem_rps_n,
   input  logic [PORT_MEM_DOFF_N_WIDTH-1:0]                   mem_doff_n,
   input  logic [PORT_MEM_LDA_N_WIDTH-1:0]                    mem_lda_n,
   input  logic [PORT_MEM_LDB_N_WIDTH-1:0]                    mem_ldb_n,
   input  logic [PORT_MEM_RWA_N_WIDTH-1:0]                    mem_rwa_n,
   input  logic [PORT_MEM_RWB_N_WIDTH-1:0]                    mem_rwb_n,
   input  logic [PORT_MEM_LBK0_N_WIDTH-1:0]                   mem_lbk0_n,
   input  logic [PORT_MEM_LBK1_N_WIDTH-1:0]                   mem_lbk1_n,
   input  logic [PORT_MEM_CFG_N_WIDTH-1:0]                    mem_cfg_n,
   input  logic [PORT_MEM_AP_WIDTH-1:0]                       mem_ap,
   output logic [PORT_MEM_PE_N_WIDTH-1:0]                     mem_pe_n,
   input  logic [PORT_MEM_AINV_WIDTH-1:0]                     mem_ainv,
   input  logic [PORT_MEM_DM_WIDTH-1:0]                       mem_dm,
   input  logic [PORT_MEM_BWS_N_WIDTH-1:0]                    mem_bws_n,
   input  logic [PORT_MEM_D_WIDTH-1:0]                        mem_d,
   inout  tri   [PORT_MEM_DQ_WIDTH-1:0]                       mem_dq,
   inout  tri   [PORT_MEM_DBI_N_WIDTH-1:0]                    mem_dbi_n,
   inout  tri   [PORT_MEM_DQA_WIDTH-1:0]                      mem_dqa,
   inout  tri   [PORT_MEM_DQB_WIDTH-1:0]                      mem_dqb,
   inout  tri   [PORT_MEM_DINVA_WIDTH-1:0]                    mem_dinva,
   inout  tri   [PORT_MEM_DINVB_WIDTH-1:0]                    mem_dinvb,
   output logic [PORT_MEM_Q_WIDTH-1:0]                        mem_q,
   output logic [PORT_MEM_ALERT_N_WIDTH-1:0]                  mem_alert_n,
   inout  tri   [PORT_MEM_DQS_WIDTH-1:0]                      mem_dqs,
   inout  tri   [PORT_MEM_DQS_N_WIDTH-1:0]                    mem_dqs_n,
   output logic [PORT_MEM_QK_WIDTH-1:0]                       mem_qk,
   output logic [PORT_MEM_QK_N_WIDTH-1:0]                     mem_qk_n,
   output logic [PORT_MEM_QKA_WIDTH-1:0]                      mem_qka,
   output logic [PORT_MEM_QKA_N_WIDTH-1:0]                    mem_qka_n,
   output logic [PORT_MEM_QKB_WIDTH-1:0]                      mem_qkb,
   output logic [PORT_MEM_QKB_N_WIDTH-1:0]                    mem_qkb_n,
   output logic [PORT_MEM_CQ_WIDTH-1:0]                       mem_cq,
   output logic [PORT_MEM_CQ_N_WIDTH-1:0]                     mem_cq_n
);
   timeunit 1ps;
   timeprecision 1ps;

   // The first level of the memory model (i.e. this module) acts as a bus
   // "splitter" for ping-pong PHY configuration. In ping-pong mode, the
   // memory bus consists of signals for two logically-independent
   // interfaces. The two interfaces however share the same physical
   // address/command bus through time-multiplexing. Certain signals, including
   // CS#/ODT/CKE/CK/CK# and the data signals, are not shared by the two
   // interfaces. This module is responsible for instantiating two underlying
   // memory models corresponding to the two logically-independent memory
   // interfaces, feeding shared signals to both models, and splitting the
   // non-shared signals before feeding them to each model.
   //
   // In non-ping-pong configuration, only one underlying model is
   // instantiated and this module is a pass-through.
   //(JCHOI)
   localparam NUM_OF_IFS = (PHY_PING_PONG_EN ? 2 : 1);

   // Calculate width of non-shared signals after splitting
   localparam PORT_MEM_CKE_WIDTH_PER_IF   = PORT_MEM_CKE_WIDTH / NUM_OF_IFS;
   localparam PORT_MEM_CS_N_WIDTH_PER_IF  = PORT_MEM_CS_N_WIDTH / NUM_OF_IFS;
   localparam PORT_MEM_ODT_WIDTH_PER_IF   = PORT_MEM_ODT_WIDTH / NUM_OF_IFS;
   localparam PORT_MEM_CK_WIDTH_PER_IF    = PORT_MEM_CK_WIDTH;
   localparam PORT_MEM_CK_N_WIDTH_PER_IF  = PORT_MEM_CK_N_WIDTH;
   localparam PORT_MEM_DQ_WIDTH_PER_IF    = PORT_MEM_DQ_WIDTH / NUM_OF_IFS;
   localparam PORT_MEM_DQS_WIDTH_PER_IF   = PORT_MEM_DQS_WIDTH / NUM_OF_IFS;
   localparam PORT_MEM_DQS_N_WIDTH_PER_IF = PORT_MEM_DQS_N_WIDTH / NUM_OF_IFS;

   // DBI#/DM are optional pins. If they're unused, we still generate a fake signal of width 1 to avoid
   // index range issue when declaring/selecting signals. Note that mem_dm is a DDR3-only issue and
   // mem_dbi_n is a DDR4-only signal.
   localparam PORT_MEM_DM_WIDTH_PER_IF    = (PORT_MEM_DM_WIDTH    == 1 && NUM_OF_IFS == 2) ? 1 : (PORT_MEM_DM_WIDTH    / NUM_OF_IFS);
   localparam PORT_MEM_DBI_N_WIDTH_PER_IF = (PORT_MEM_DBI_N_WIDTH == 1 && NUM_OF_IFS == 2) ? 1 : (PORT_MEM_DBI_N_WIDTH / NUM_OF_IFS);

   // Multiple alert# pins are meant to be daisy-chained on the board
   // to obtain a single logically-AND'ed version of alert# before
   // passing upward.
   logic [NUM_OF_IFS-1:0] alert_n;
   assign mem_alert_n = &alert_n;

   generate
      genvar inst_i;

      for (inst_i = 0; inst_i < NUM_OF_IFS; ++inst_i)
      begin : pp_gen
         altera_emif_ddrx_model_per_ping_pong # (
            .PROTOCOL_ENUM                     (PROTOCOL_ENUM),
            .MEM_FORMAT_ENUM                   (MEM_FORMAT_ENUM),
            .MEM_DISCRETE_CS_WIDTH             (MEM_DISCRETE_CS_WIDTH),
            .MEM_CHIP_ID_WIDTH                 (MEM_CHIP_ID_WIDTH),
            .MEM_RANKS_PER_DIMM                (MEM_RANKS_PER_DIMM),
            .MEM_NUM_OF_DIMMS                  (MEM_NUM_OF_DIMMS),
            .MEM_AC_PAR_EN                     (MEM_AC_PAR_EN),
            .MEM_DM_EN                         (MEM_DM_EN),
            .PORT_MEM_CKE_WIDTH                (PORT_MEM_CKE_WIDTH_PER_IF),
            .PORT_MEM_CK_WIDTH                 (PORT_MEM_CK_WIDTH_PER_IF),
            .PORT_MEM_CK_N_WIDTH               (PORT_MEM_CK_N_WIDTH_PER_IF),
            .PORT_MEM_BA_WIDTH                 (PORT_MEM_BA_WIDTH),
            .PORT_MEM_BG_WIDTH                 (PORT_MEM_BG_WIDTH),
            .PORT_MEM_C_WIDTH                  (PORT_MEM_C_WIDTH),
            .PORT_MEM_A_WIDTH                  (PORT_MEM_A_WIDTH),
            .PORT_MEM_CS_N_WIDTH               (PORT_MEM_CS_N_WIDTH_PER_IF),
            .PORT_MEM_RAS_N_WIDTH              (PORT_MEM_RAS_N_WIDTH),
            .PORT_MEM_CAS_N_WIDTH              (PORT_MEM_CAS_N_WIDTH),
            .PORT_MEM_WE_N_WIDTH               (PORT_MEM_WE_N_WIDTH),
            .PORT_MEM_ACT_N_WIDTH              (PORT_MEM_ACT_N_WIDTH),
            .PORT_MEM_DQS_WIDTH                (PORT_MEM_DQS_WIDTH_PER_IF),
            .PORT_MEM_DQS_N_WIDTH              (PORT_MEM_DQS_N_WIDTH_PER_IF),
            .PORT_MEM_DQ_WIDTH                 (PORT_MEM_DQ_WIDTH_PER_IF),
            .PORT_MEM_DM_WIDTH                 (PORT_MEM_DM_WIDTH_PER_IF),
            .PORT_MEM_DBI_N_WIDTH              (PORT_MEM_DBI_N_WIDTH_PER_IF),
            .PORT_MEM_RESET_N_WIDTH            (PORT_MEM_RESET_N_WIDTH),
            .PORT_MEM_PAR_WIDTH                (PORT_MEM_PAR_WIDTH),
            .PORT_MEM_ALERT_N_WIDTH            (1),
            .PORT_MEM_RM_WIDTH                 (PORT_MEM_RM_WIDTH),
            .PORT_MEM_ODT_WIDTH                (PORT_MEM_ODT_WIDTH / NUM_OF_IFS),
            .MEM_ROW_ADDR_WIDTH                (MEM_ROW_ADDR_WIDTH),
            .MEM_COL_ADDR_WIDTH                (MEM_COL_ADDR_WIDTH),
            .MEM_TRTP                          (MEM_TRTP),
            .MEM_TRCD                          (MEM_TRCD),
            .MEM_INIT_MRS0                     (MEM_INIT_MRS0),
            .MEM_INIT_MRS1                     (MEM_INIT_MRS1),
            .MEM_INIT_MRS2                     (MEM_INIT_MRS2),
            .MEM_INIT_MRS3                     (MEM_INIT_MRS3),
            .MEM_MIRROR_ADDRESSING_EN          (MEM_FORMAT_ENUM == "MEM_FORMAT_DISCRETE" ? MEM_DISCRETE_MIRROR_ADDRESSING_EN :  MEM_MIRROR_ADDRESSING_EN),
            .MEM_CFG_GEN_SBE                   (MEM_CFG_GEN_SBE),
            .MEM_CFG_GEN_DBE                   (MEM_CFG_GEN_DBE),
            .MEM_CLK_FREQUENCY                 (MEM_CLK_FREQUENCY),
            .MEM_MICRON_AUTOMATA               (MEM_MICRON_AUTOMATA)
         ) inst (
            .mem_cs_n                 (mem_cs_n [ (PORT_MEM_CS_N_WIDTH_PER_IF * inst_i)  +: PORT_MEM_CS_N_WIDTH_PER_IF]),
            .mem_cke                  (mem_cke  [ (PORT_MEM_CKE_WIDTH_PER_IF * inst_i)   +: PORT_MEM_CKE_WIDTH_PER_IF]),
            .mem_odt                  (mem_odt  [ (PORT_MEM_ODT_WIDTH_PER_IF * inst_i)   +: PORT_MEM_ODT_WIDTH_PER_IF]),
            .mem_dq                   (mem_dq   [ (PORT_MEM_DQ_WIDTH_PER_IF * inst_i)    +: PORT_MEM_DQ_WIDTH_PER_IF]),
            .mem_dqs                  (mem_dqs  [ (PORT_MEM_DQS_WIDTH_PER_IF * inst_i)   +: PORT_MEM_DQS_WIDTH_PER_IF]),
            .mem_dqs_n                (mem_dqs_n[ (PORT_MEM_DQS_N_WIDTH_PER_IF * inst_i) +: PORT_MEM_DQS_N_WIDTH_PER_IF]),
            .mem_dm                   (mem_dm   [ (PORT_MEM_DM_WIDTH_PER_IF    * ((PORT_MEM_DM_WIDTH    == 1 && NUM_OF_IFS == 2) ? 0 : inst_i)) +: PORT_MEM_DM_WIDTH_PER_IF]),
            .mem_dbi_n                (mem_dbi_n[ (PORT_MEM_DBI_N_WIDTH_PER_IF * ((PORT_MEM_DBI_N_WIDTH == 1 && NUM_OF_IFS == 2) ? 0 : inst_i)) +: PORT_MEM_DBI_N_WIDTH_PER_IF]),
            .mem_alert_n              (alert_n  [inst_i]),
            .mem_rm                   (mem_rm),
            .*
      );
   end
   endgenerate
endmodule
