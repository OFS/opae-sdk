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
// Memory model representing either the "ping" or the "pong" side of
// the memory device for ping-pong topology. For non-ping-pong topology
// this is simply the top-level wrapper of the memory model.
//
///////////////////////////////////////////////////////////////////////////////
module altera_emif_ddrx_model_per_ping_pong #
(
   parameter PROTOCOL_ENUM                                                = "",
   parameter MEM_FORMAT_ENUM                                              = "",
   parameter MEM_DISCRETE_CS_WIDTH                                        = 1,
   parameter MEM_CHIP_ID_WIDTH                                            = 0,
   parameter MEM_RANKS_PER_DIMM                                           = 0,
   parameter MEM_NUM_OF_DIMMS                                             = 0,
   parameter MEM_AC_PAR_EN                                                = 0,
   parameter MEM_DM_EN                                                    = 0,

   parameter PORT_MEM_CKE_WIDTH                                           = 1,
   parameter PORT_MEM_CK_WIDTH                                            = 1,
   parameter PORT_MEM_CK_N_WIDTH                                          = 1,
   parameter PORT_MEM_BA_WIDTH                                            = 1,
   parameter PORT_MEM_BG_WIDTH                                            = 1,
   parameter PORT_MEM_C_WIDTH                                             = 1,
   parameter PORT_MEM_A_WIDTH                                             = 1,
   parameter PORT_MEM_CS_N_WIDTH                                          = 1,
   parameter PORT_MEM_RAS_N_WIDTH                                         = 1,
   parameter PORT_MEM_CAS_N_WIDTH                                         = 1,
   parameter PORT_MEM_WE_N_WIDTH                                          = 1,
   parameter PORT_MEM_ACT_N_WIDTH                                         = 1,
   parameter PORT_MEM_DQS_WIDTH                                           = 1,
   parameter PORT_MEM_DQS_N_WIDTH                                         = 1,
   parameter PORT_MEM_DQ_WIDTH                                            = 1,
   parameter PORT_MEM_DM_WIDTH                                            = 1,
   parameter PORT_MEM_DBI_N_WIDTH                                         = 1,
   parameter PORT_MEM_RESET_N_WIDTH                                       = 1,
   parameter PORT_MEM_PAR_WIDTH                                           = 1,
   parameter PORT_MEM_ALERT_N_WIDTH                                       = 1,
   parameter PORT_MEM_ODT_WIDTH                                           = 1,
   parameter PORT_MEM_RM_WIDTH                                            = 1,

   parameter MEM_ROW_ADDR_WIDTH                                           = 1,
   parameter MEM_COL_ADDR_WIDTH                                           = 1,
   parameter MEM_TRTP                                                     = 0,
   parameter MEM_TRCD                                                     = 0,
   parameter MEM_INIT_MRS0                                                = 0,
   parameter MEM_INIT_MRS1                                                = 0,
   parameter MEM_INIT_MRS2                                                = 0,
   parameter MEM_INIT_MRS3                                                = 0,
   parameter MEM_MIRROR_ADDRESSING_EN                                     = 0,
   parameter MEM_CFG_GEN_SBE                                              = 0,
   parameter MEM_CFG_GEN_DBE                                              = 0,
   parameter MEM_CLK_FREQUENCY                                            = 0,
   parameter MEM_MICRON_AUTOMATA                                          = 0
) (
   input  logic                        [PORT_MEM_A_WIDTH-1:0]            mem_a,
   input  logic                        [PORT_MEM_BA_WIDTH-1:0]           mem_ba,
   input  logic                        [PORT_MEM_BG_WIDTH-1:0]           mem_bg,
   input  logic                        [PORT_MEM_C_WIDTH-1:0]            mem_c,
   input  logic                        [PORT_MEM_CK_WIDTH-1:0]           mem_ck,
   input  logic                        [PORT_MEM_CK_N_WIDTH-1:0]         mem_ck_n,
   input  logic                        [PORT_MEM_CKE_WIDTH - 1:0]        mem_cke,
   input  logic                        [PORT_MEM_CS_N_WIDTH - 1:0]       mem_cs_n,
   input  logic                        [PORT_MEM_RAS_N_WIDTH - 1:0]      mem_ras_n,
   input  logic                        [PORT_MEM_CAS_N_WIDTH - 1:0]      mem_cas_n,
   input  logic                        [PORT_MEM_WE_N_WIDTH - 1:0]       mem_we_n,
   input  logic                        [PORT_MEM_ACT_N_WIDTH - 1:0]      mem_act_n,
   input  logic                        [PORT_MEM_RESET_N_WIDTH - 1:0]    mem_reset_n,
   input  logic                        [PORT_MEM_DM_WIDTH - 1:0]         mem_dm,
   inout  tri                          [PORT_MEM_DBI_N_WIDTH - 1:0]      mem_dbi_n,
   inout  tri                          [PORT_MEM_DQ_WIDTH - 1:0]         mem_dq,
   inout  tri                          [PORT_MEM_DQS_WIDTH - 1:0]        mem_dqs,
   inout  tri                          [PORT_MEM_DQS_N_WIDTH - 1:0]      mem_dqs_n,
   output logic                        [PORT_MEM_ALERT_N_WIDTH-1:0]      mem_alert_n,
   input  logic                        [PORT_MEM_PAR_WIDTH-1:0]          mem_par,
   input  logic                        [PORT_MEM_ODT_WIDTH-1:0]          mem_odt,
   input  logic                        [PORT_MEM_RM_WIDTH-1:0]           mem_rm
);
   timeunit 1ps;
   timeprecision 1ps;

   localparam MEM_MODEL_DEVICE_DEPTH = (MEM_FORMAT_ENUM == "MEM_FORMAT_RDIMM" || MEM_FORMAT_ENUM == "MEM_FORMAT_LRDIMM" || MEM_FORMAT_ENUM == "MEM_FORMAT_UDIMM"  || MEM_FORMAT_ENUM == "MEM_FORMAT_SODIMM" ) ? MEM_NUM_OF_DIMMS : 1 ;

   wire logic [MEM_MODEL_DEVICE_DEPTH - 1:0] alert_n;
   assign mem_alert_n = &alert_n;

   /* DDR4 Shared Address/Command Bus: {RAS_n, CAS_n, WE_n} = A[16:14]
   Interpret as RAS/CAS/WE when ACT_n = 1
   Interpret as A[16:14] when ACT_n = 0
   If DDR4, there should be no RAS/CAS/WE coming in. We copy A[16:14] to those signals here. */
   logic [PORT_MEM_RAS_N_WIDTH-1:0] int_mem_ras_n;
   logic [PORT_MEM_CAS_N_WIDTH-1:0] int_mem_cas_n;
   logic [PORT_MEM_WE_N_WIDTH-1:0] int_mem_we_n;
   initial begin
      assert(!(PROTOCOL_ENUM == "PROTOCOL_DDR4" && PORT_MEM_A_WIDTH < 17)) else $error("mem_a width must be at least 17 for DDR4");
   end

   generate
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         always_comb begin
            int_mem_ras_n <= {PORT_MEM_RAS_N_WIDTH{mem_a[16]}};
            int_mem_cas_n <= {PORT_MEM_CAS_N_WIDTH{mem_a[15]}};
            int_mem_we_n <= {PORT_MEM_WE_N_WIDTH{mem_a[14]}};
         end
      end else begin
         always_comb begin
            int_mem_ras_n <= mem_ras_n;
            int_mem_cas_n <= mem_cas_n;
            int_mem_we_n <= mem_we_n;
         end
      end
   endgenerate

   generate
      genvar depth;
      for (depth = 0; depth < MEM_MODEL_DEVICE_DEPTH; ++depth) begin : depth_gen

         altera_emif_ddrx_model_per_device #(
            .PROTOCOL_ENUM                               (PROTOCOL_ENUM),
            .PORT_MEM_CKE_WIDTH                          (PORT_MEM_CKE_WIDTH),
            .PORT_MEM_BA_WIDTH                           (PORT_MEM_BA_WIDTH),
            .PORT_MEM_BG_WIDTH                           (PORT_MEM_BG_WIDTH),
            .PORT_MEM_C_WIDTH                            (PORT_MEM_C_WIDTH),
            .PORT_MEM_A_WIDTH                            (PORT_MEM_A_WIDTH),
            .PORT_MEM_CS_N_WIDTH                         (PORT_MEM_CS_N_WIDTH / MEM_MODEL_DEVICE_DEPTH),
            .PORT_MEM_DQS_WIDTH                          (PORT_MEM_DQS_WIDTH),
            .PORT_MEM_DQS_N_WIDTH                        (PORT_MEM_DQS_N_WIDTH),
            .PORT_MEM_DQ_WIDTH                           (PORT_MEM_DQ_WIDTH),
            .PORT_MEM_RAS_N_WIDTH                        (PORT_MEM_RAS_N_WIDTH),
            .PORT_MEM_CAS_N_WIDTH                        (PORT_MEM_CAS_N_WIDTH),
            .PORT_MEM_WE_N_WIDTH                         (PORT_MEM_WE_N_WIDTH),
            .PORT_MEM_ACT_N_WIDTH                        (PORT_MEM_ACT_N_WIDTH),
            .PORT_MEM_DM_WIDTH                           (PORT_MEM_DM_WIDTH),
            .PORT_MEM_DBI_N_WIDTH                        (PORT_MEM_DBI_N_WIDTH),
            .PORT_MEM_RESET_N_WIDTH                      (PORT_MEM_RESET_N_WIDTH),
            .PORT_MEM_PAR_WIDTH                          (PORT_MEM_PAR_WIDTH),
            .PORT_MEM_ALERT_N_WIDTH                      (1),
            .PORT_MEM_RM_WIDTH                           (PORT_MEM_RM_WIDTH),
            .MEM_CHIP_ID_WIDTH                           (MEM_CHIP_ID_WIDTH),
            .MEM_ROW_ADDR_WIDTH                          (MEM_ROW_ADDR_WIDTH),
            .MEM_COL_ADDR_WIDTH                          (MEM_COL_ADDR_WIDTH),
            .MEM_TRTP                                    (MEM_TRTP),
            .MEM_TRCD                                    (MEM_TRCD),
            .MEM_INIT_MRS0                               (MEM_INIT_MRS0),
            .MEM_INIT_MRS1                               (MEM_INIT_MRS1),
            .MEM_INIT_MRS2                               (MEM_INIT_MRS2),
            .MEM_INIT_MRS3                               (MEM_INIT_MRS3),
            .MEM_DEPTH_IDX                               (depth),
            .MEM_FORMAT_ENUM                             (MEM_FORMAT_ENUM),
            .MEM_RANKS_PER_DIMM                          (MEM_RANKS_PER_DIMM),
            .MEM_DM_EN                                   (MEM_DM_EN),
            .MEM_MIRROR_ADDRESSING_EN                    (MEM_MIRROR_ADDRESSING_EN),
            .MEM_AC_PAR_EN                               (MEM_AC_PAR_EN),
            .MEM_CFG_GEN_SBE                             (MEM_CFG_GEN_SBE),
            .MEM_CFG_GEN_DBE                             (MEM_CFG_GEN_DBE),
            .MEM_CLK_FREQUENCY                           (MEM_CLK_FREQUENCY),
            .MEM_MICRON_AUTOMATA                         (MEM_MICRON_AUTOMATA)
         ) mem_inst (
            .mem_a                                    (mem_a),
            .mem_ba                                   (mem_ba),
            .mem_bg                                   (mem_bg),
            .mem_c                                    (mem_c),
            .mem_ck                                   (mem_ck[0]),
            .mem_ck_n                                 (mem_ck_n[0]),
            .mem_cke                                  (mem_cke),
            .mem_cs_n                                 (mem_cs_n[PORT_MEM_CS_N_WIDTH/MEM_MODEL_DEVICE_DEPTH*(depth+1)-1:PORT_MEM_CS_N_WIDTH/MEM_MODEL_DEVICE_DEPTH*depth]),
            .mem_ras_n                                (int_mem_ras_n),
            .mem_cas_n                                (int_mem_cas_n),
            .mem_we_n                                 (int_mem_we_n),
            .mem_act_n                                (mem_act_n),
            .mem_reset_n                              (mem_reset_n),
            .mem_dm                                   (mem_dm),
            .mem_dbi_n                                (mem_dbi_n),
            .mem_dq                                   (mem_dq),
            .mem_dqs                                  (mem_dqs),
            .mem_dqs_n                                (mem_dqs_n),
            .mem_par                                  (mem_par),
            .mem_alert_n                              (alert_n[depth]),
            .mem_odt                                  (mem_odt[0]),
            .mem_rm                                   (mem_rm)
         );
      end
   endgenerate
endmodule
