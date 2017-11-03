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
// Wrapper module for IO tiles
//
///////////////////////////////////////////////////////////////////////////////


module altera_emif_arch_nf_io_tiles_wrap #(
   parameter DIAG_SYNTH_FOR_SIM                      = 0,
   parameter DIAG_CPA_OUT_1_EN                       = 0,
   parameter DIAG_FAST_SIM                           = 0,
   parameter IS_HPS                                  = 0,
   parameter SILICON_REV                             = "",
   parameter PROTOCOL_ENUM                           = "",
   parameter PHY_PING_PONG_EN                        = 0,
   parameter DQS_BUS_MODE_ENUM                       = "",
   parameter USER_CLK_RATIO                          = 1,
   parameter PHY_HMC_CLK_RATIO                       = 1,
   parameter C2P_P2C_CLK_RATIO                       = 1,
   parameter PLL_VCO_TO_MEM_CLK_FREQ_RATIO           = 1,
   parameter PLL_VCO_FREQ_MHZ_INT                    = 0,
   parameter MEM_BURST_LENGTH                        = 0,
   parameter MEM_DATA_MASK_EN                        = 1,
   parameter PINS_PER_LANE                           = 1,
   parameter LANES_PER_TILE                          = 1,
   parameter PINS_IN_RTL_TILES                       = 1,
   parameter LANES_IN_RTL_TILES                      = 1,
   parameter NUM_OF_RTL_TILES                        = 1,
   parameter AC_PIN_MAP_SCHEME                       = "",
   parameter PRI_AC_TILE_INDEX                       = -1,
   parameter SEC_AC_TILE_INDEX                       = -1,
   parameter PRI_HMC_DBC_SHADOW_LANE_INDEX           = -1,
   parameter NUM_OF_HMC_PORTS                        = 1,
   parameter HMC_AVL_PROTOCOL_ENUM                   = "",
   parameter HMC_CTRL_DIMM_TYPE                      = "",
   parameter           PRI_HMC_CFG_ENABLE_ECC                      = "",
   parameter           PRI_HMC_CFG_REORDER_DATA                    = "",
   parameter           PRI_HMC_CFG_REORDER_READ                    = "",
   parameter           PRI_HMC_CFG_REORDER_RDATA                   = "",
   parameter [  5:  0] PRI_HMC_CFG_STARVE_LIMIT                    = 0,
   parameter           PRI_HMC_CFG_DQS_TRACKING_EN                 = "",
   parameter           PRI_HMC_CFG_ARBITER_TYPE                    = "",
   parameter           PRI_HMC_CFG_OPEN_PAGE_EN                    = "",
   parameter           PRI_HMC_CFG_GEAR_DOWN_EN                    = "",
   parameter           PRI_HMC_CFG_RLD3_MULTIBANK_MODE             = "",
   parameter           PRI_HMC_CFG_PING_PONG_MODE                  = "",
   parameter [  1:  0] PRI_HMC_CFG_SLOT_ROTATE_EN                  = 0,
   parameter [  1:  0] PRI_HMC_CFG_SLOT_OFFSET                     = 0,
   parameter [  3:  0] PRI_HMC_CFG_COL_CMD_SLOT                    = 0,
   parameter [  3:  0] PRI_HMC_CFG_ROW_CMD_SLOT                    = 0,
   parameter           PRI_HMC_CFG_ENABLE_RC                       = "",
   parameter [ 15:  0] PRI_HMC_CFG_CS_TO_CHIP_MAPPING              = 0,
   parameter [  6:  0] PRI_HMC_CFG_RB_RESERVED_ENTRY               = 0,
   parameter [  6:  0] PRI_HMC_CFG_WB_RESERVED_ENTRY               = 0,
   parameter [  6:  0] PRI_HMC_CFG_TCL                             = 0,
   parameter [  5:  0] PRI_HMC_CFG_POWER_SAVING_EXIT_CYC           = 0,
   parameter [  5:  0] PRI_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC       = 0,
   parameter [ 15:  0] PRI_HMC_CFG_WRITE_ODT_CHIP                  = 0,
   parameter [ 15:  0] PRI_HMC_CFG_READ_ODT_CHIP                   = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_ODT_ON                       = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_ODT_ON                       = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_ODT_PERIOD                   = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_ODT_PERIOD                   = 0,
   parameter [ 15:  0] PRI_HMC_CFG_RLD3_REFRESH_SEQ0               = 0,
   parameter [ 15:  0] PRI_HMC_CFG_RLD3_REFRESH_SEQ1               = 0,
   parameter [ 15:  0] PRI_HMC_CFG_RLD3_REFRESH_SEQ2               = 0,
   parameter [ 15:  0] PRI_HMC_CFG_RLD3_REFRESH_SEQ3               = 0,
   parameter           PRI_HMC_CFG_SRF_ZQCAL_DISABLE               = "",
   parameter           PRI_HMC_CFG_MPS_ZQCAL_DISABLE               = "",
   parameter           PRI_HMC_CFG_MPS_DQSTRK_DISABLE              = "",
   parameter           PRI_HMC_CFG_SHORT_DQSTRK_CTRL_EN            = "",
   parameter           PRI_HMC_CFG_PERIOD_DQSTRK_CTRL_EN           = "",
   parameter [ 15:  0] PRI_HMC_CFG_PERIOD_DQSTRK_INTERVAL          = 0,
   parameter [  7:  0] PRI_HMC_CFG_DQSTRK_TO_VALID_LAST            = 0,
   parameter [  7:  0] PRI_HMC_CFG_DQSTRK_TO_VALID                 = 0,
   parameter [  6:  0] PRI_HMC_CFG_RFSH_WARN_THRESHOLD             = 0,
   parameter           PRI_HMC_CFG_SB_CG_DISABLE                   = "",
   parameter           PRI_HMC_CFG_USER_RFSH_EN                    = "",
   parameter           PRI_HMC_CFG_SRF_AUTOEXIT_EN                 = "",
   parameter           PRI_HMC_CFG_SRF_ENTRY_EXIT_BLOCK            = "",
   parameter [ 19:  0] PRI_HMC_CFG_SB_DDR4_MR3                     = 0,
   parameter [ 19:  0] PRI_HMC_CFG_SB_DDR4_MR4                     = 0,
   parameter [ 15:  0] PRI_HMC_CFG_SB_DDR4_MR5                     = 0,
   parameter [  0:  0] PRI_HMC_CFG_DDR4_MPS_ADDR_MIRROR            = 0,
   parameter           PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH            = "",
   parameter           PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH            = "",
   parameter           PRI_HMC_CFG_MEM_IF_BANKADDR_WIDTH           = "",
   parameter           PRI_HMC_CFG_MEM_IF_BGADDR_WIDTH             = "",
   parameter           PRI_HMC_CFG_LOCAL_IF_CS_WIDTH               = "",
   parameter           PRI_HMC_CFG_ADDR_ORDER                      = "",
   parameter [  5:  0] PRI_HMC_CFG_ACT_TO_RDWR                     = 0,
   parameter [  5:  0] PRI_HMC_CFG_ACT_TO_PCH                      = 0,
   parameter [  5:  0] PRI_HMC_CFG_ACT_TO_ACT                      = 0,
   parameter [  5:  0] PRI_HMC_CFG_ACT_TO_ACT_DIFF_BANK            = 0,
   parameter [  5:  0] PRI_HMC_CFG_ACT_TO_ACT_DIFF_BG              = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_RD                        = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_RD_DIFF_CHIP              = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_RD_DIFF_BG                = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_WR                        = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_WR_DIFF_CHIP              = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_WR_DIFF_BG                = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_TO_PCH                       = 0,
   parameter [  5:  0] PRI_HMC_CFG_RD_AP_TO_VALID                  = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_WR                        = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_WR_DIFF_CHIP              = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_WR_DIFF_BG                = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_RD                        = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_RD_DIFF_CHIP              = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_RD_DIFF_BG                = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_TO_PCH                       = 0,
   parameter [  5:  0] PRI_HMC_CFG_WR_AP_TO_VALID                  = 0,
   parameter [  5:  0] PRI_HMC_CFG_PCH_TO_VALID                    = 0,
   parameter [  5:  0] PRI_HMC_CFG_PCH_ALL_TO_VALID                = 0,
   parameter [  7:  0] PRI_HMC_CFG_ARF_TO_VALID                    = 0,
   parameter [  5:  0] PRI_HMC_CFG_PDN_TO_VALID                    = 0,
   parameter [  9:  0] PRI_HMC_CFG_SRF_TO_VALID                    = 0,
   parameter [  9:  0] PRI_HMC_CFG_SRF_TO_ZQ_CAL                   = 0,
   parameter [ 12:  0] PRI_HMC_CFG_ARF_PERIOD                      = 0,
   parameter [ 15:  0] PRI_HMC_CFG_PDN_PERIOD                      = 0,
   parameter [  8:  0] PRI_HMC_CFG_ZQCL_TO_VALID                   = 0,
   parameter [  6:  0] PRI_HMC_CFG_ZQCS_TO_VALID                   = 0,
   parameter [  3:  0] PRI_HMC_CFG_MRS_TO_VALID                    = 0,
   parameter [  9:  0] PRI_HMC_CFG_MPS_TO_VALID                    = 0,
   parameter [  3:  0] PRI_HMC_CFG_MRR_TO_VALID                    = 0,
   parameter [  4:  0] PRI_HMC_CFG_MPR_TO_VALID                    = 0,
   parameter [  3:  0] PRI_HMC_CFG_MPS_EXIT_CS_TO_CKE              = 0,
   parameter [  3:  0] PRI_HMC_CFG_MPS_EXIT_CKE_TO_CS              = 0,
   parameter [  2:  0] PRI_HMC_CFG_RLD3_MULTIBANK_REF_DELAY        = 0,
   parameter [  7:  0] PRI_HMC_CFG_MMR_CMD_TO_VALID                = 0,
   parameter [  7:  0] PRI_HMC_CFG_4_ACT_TO_ACT                    = 0,
   parameter [  7:  0] PRI_HMC_CFG_16_ACT_TO_ACT                   = 0,

   parameter           SEC_HMC_CFG_ENABLE_ECC                      = "",
   parameter           SEC_HMC_CFG_REORDER_DATA                    = "",
   parameter           SEC_HMC_CFG_REORDER_READ                    = "",
   parameter           SEC_HMC_CFG_REORDER_RDATA                   = "",
   parameter [  5:  0] SEC_HMC_CFG_STARVE_LIMIT                    = 0,
   parameter           SEC_HMC_CFG_DQS_TRACKING_EN                 = "",
   parameter           SEC_HMC_CFG_ARBITER_TYPE                    = "",
   parameter           SEC_HMC_CFG_OPEN_PAGE_EN                    = "",
   parameter           SEC_HMC_CFG_GEAR_DOWN_EN                    = "",
   parameter           SEC_HMC_CFG_RLD3_MULTIBANK_MODE             = "",
   parameter           SEC_HMC_CFG_PING_PONG_MODE                  = "",
   parameter [  1:  0] SEC_HMC_CFG_SLOT_ROTATE_EN                  = 0,
   parameter [  1:  0] SEC_HMC_CFG_SLOT_OFFSET                     = 0,
   parameter [  3:  0] SEC_HMC_CFG_COL_CMD_SLOT                    = 0,
   parameter [  3:  0] SEC_HMC_CFG_ROW_CMD_SLOT                    = 0,
   parameter           SEC_HMC_CFG_ENABLE_RC                       = "",
   parameter [ 15:  0] SEC_HMC_CFG_CS_TO_CHIP_MAPPING              = 0,
   parameter [  6:  0] SEC_HMC_CFG_RB_RESERVED_ENTRY               = 0,
   parameter [  6:  0] SEC_HMC_CFG_WB_RESERVED_ENTRY               = 0,
   parameter [  6:  0] SEC_HMC_CFG_TCL                             = 0,
   parameter [  5:  0] SEC_HMC_CFG_POWER_SAVING_EXIT_CYC           = 0,
   parameter [  5:  0] SEC_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC       = 0,
   parameter [ 15:  0] SEC_HMC_CFG_WRITE_ODT_CHIP                  = 0,
   parameter [ 15:  0] SEC_HMC_CFG_READ_ODT_CHIP                   = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_ODT_ON                       = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_ODT_ON                       = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_ODT_PERIOD                   = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_ODT_PERIOD                   = 0,
   parameter [ 15:  0] SEC_HMC_CFG_RLD3_REFRESH_SEQ0               = 0,
   parameter [ 15:  0] SEC_HMC_CFG_RLD3_REFRESH_SEQ1               = 0,
   parameter [ 15:  0] SEC_HMC_CFG_RLD3_REFRESH_SEQ2               = 0,
   parameter [ 15:  0] SEC_HMC_CFG_RLD3_REFRESH_SEQ3               = 0,
   parameter           SEC_HMC_CFG_SRF_ZQCAL_DISABLE               = "",
   parameter           SEC_HMC_CFG_MPS_ZQCAL_DISABLE               = "",
   parameter           SEC_HMC_CFG_MPS_DQSTRK_DISABLE              = "",
   parameter           SEC_HMC_CFG_SHORT_DQSTRK_CTRL_EN            = "",
   parameter           SEC_HMC_CFG_PERIOD_DQSTRK_CTRL_EN           = "",
   parameter [ 15:  0] SEC_HMC_CFG_PERIOD_DQSTRK_INTERVAL          = 0,
   parameter [  7:  0] SEC_HMC_CFG_DQSTRK_TO_VALID_LAST            = 0,
   parameter [  7:  0] SEC_HMC_CFG_DQSTRK_TO_VALID                 = 0,
   parameter [  6:  0] SEC_HMC_CFG_RFSH_WARN_THRESHOLD             = 0,
   parameter           SEC_HMC_CFG_SB_CG_DISABLE                   = "",
   parameter           SEC_HMC_CFG_USER_RFSH_EN                    = "",
   parameter           SEC_HMC_CFG_SRF_AUTOEXIT_EN                 = "",
   parameter           SEC_HMC_CFG_SRF_ENTRY_EXIT_BLOCK            = "",
   parameter [ 19:  0] SEC_HMC_CFG_SB_DDR4_MR3                     = 0,
   parameter [ 19:  0] SEC_HMC_CFG_SB_DDR4_MR4                     = 0,
   parameter [ 15:  0] SEC_HMC_CFG_SB_DDR4_MR5                     = 0,
   parameter [  0:  0] SEC_HMC_CFG_DDR4_MPS_ADDR_MIRROR            = 0,
   parameter           SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH            = "",
   parameter           SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH            = "",
   parameter           SEC_HMC_CFG_MEM_IF_BANKADDR_WIDTH           = "",
   parameter           SEC_HMC_CFG_MEM_IF_BGADDR_WIDTH             = "",
   parameter           SEC_HMC_CFG_LOCAL_IF_CS_WIDTH               = "",
   parameter           SEC_HMC_CFG_ADDR_ORDER                      = "",
   parameter [  5:  0] SEC_HMC_CFG_ACT_TO_RDWR                     = 0,
   parameter [  5:  0] SEC_HMC_CFG_ACT_TO_PCH                      = 0,
   parameter [  5:  0] SEC_HMC_CFG_ACT_TO_ACT                      = 0,
   parameter [  5:  0] SEC_HMC_CFG_ACT_TO_ACT_DIFF_BANK            = 0,
   parameter [  5:  0] SEC_HMC_CFG_ACT_TO_ACT_DIFF_BG              = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_RD                        = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_RD_DIFF_CHIP              = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_RD_DIFF_BG                = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_WR                        = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_WR_DIFF_CHIP              = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_WR_DIFF_BG                = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_TO_PCH                       = 0,
   parameter [  5:  0] SEC_HMC_CFG_RD_AP_TO_VALID                  = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_WR                        = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_WR_DIFF_CHIP              = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_WR_DIFF_BG                = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_RD                        = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_RD_DIFF_CHIP              = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_RD_DIFF_BG                = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_TO_PCH                       = 0,
   parameter [  5:  0] SEC_HMC_CFG_WR_AP_TO_VALID                  = 0,
   parameter [  5:  0] SEC_HMC_CFG_PCH_TO_VALID                    = 0,
   parameter [  5:  0] SEC_HMC_CFG_PCH_ALL_TO_VALID                = 0,
   parameter [  7:  0] SEC_HMC_CFG_ARF_TO_VALID                    = 0,
   parameter [  5:  0] SEC_HMC_CFG_PDN_TO_VALID                    = 0,
   parameter [  9:  0] SEC_HMC_CFG_SRF_TO_VALID                    = 0,
   parameter [  9:  0] SEC_HMC_CFG_SRF_TO_ZQ_CAL                   = 0,
   parameter [ 12:  0] SEC_HMC_CFG_ARF_PERIOD                      = 0,
   parameter [ 15:  0] SEC_HMC_CFG_PDN_PERIOD                      = 0,
   parameter [  8:  0] SEC_HMC_CFG_ZQCL_TO_VALID                   = 0,
   parameter [  6:  0] SEC_HMC_CFG_ZQCS_TO_VALID                   = 0,
   parameter [  3:  0] SEC_HMC_CFG_MRS_TO_VALID                    = 0,
   parameter [  9:  0] SEC_HMC_CFG_MPS_TO_VALID                    = 0,
   parameter [  3:  0] SEC_HMC_CFG_MRR_TO_VALID                    = 0,
   parameter [  4:  0] SEC_HMC_CFG_MPR_TO_VALID                    = 0,
   parameter [  3:  0] SEC_HMC_CFG_MPS_EXIT_CS_TO_CKE              = 0,
   parameter [  3:  0] SEC_HMC_CFG_MPS_EXIT_CKE_TO_CS              = 0,
   parameter [  2:  0] SEC_HMC_CFG_RLD3_MULTIBANK_REF_DELAY        = 0,
   parameter [  7:  0] SEC_HMC_CFG_MMR_CMD_TO_VALID                = 0,
   parameter [  7:  0] SEC_HMC_CFG_4_ACT_TO_ACT                    = 0,
   parameter [  7:  0] SEC_HMC_CFG_16_ACT_TO_ACT                   = 0,
   parameter LANES_USAGE                             = 1'b0,
   parameter PINS_USAGE                              = 1'b0,
   parameter PINS_RATE                               = 1'b0,
   parameter PINS_WDB                                = 1'b0,
   parameter PINS_DB_IN_BYPASS                       = 1'b0,
   parameter PINS_DB_OUT_BYPASS                      = 1'b0,
   parameter PINS_DB_OE_BYPASS                       = 1'b0,
   parameter PINS_INVERT_WR                          = 1'b0,
   parameter PINS_INVERT_OE                          = 1'b0,
   parameter PINS_AC_HMC_DATA_OVERRIDE_ENA           = 1'b0,
   parameter PINS_DATA_IN_MODE                       = 1'b0,
   parameter PINS_OCT_MODE                           = 1'b0,
   parameter PINS_GPIO_MODE                          = 1'b0,
   parameter CENTER_TIDS                             = 1'b0,
   parameter HMC_TIDS                                = 1'b0,
   parameter LANE_TIDS                               = 1'b0,
   parameter PREAMBLE_MODE                           = "",
   parameter DBI_WR_ENABLE                           = "",
   parameter DBI_RD_ENABLE                           = "",
   parameter CRC_EN                                  = "",
   parameter SWAP_DQS_A_B                            = "",
   parameter DQS_PACK_MODE                           = "",
   parameter OCT_SIZE	                             = "",
   parameter [6:0] DBC_WB_RESERVED_ENTRY             = 4,
   parameter DLL_MODE                                = "",
   parameter DLL_CODEWORD                            = 0,
   parameter PORT_MEM_DQ_WIDTH                       = 1,
   parameter PORT_MEM_DQS_WIDTH                      = 1,
   parameter PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH     = 1,
   parameter PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH    = 1,
   parameter PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH     = 1,
   parameter PORT_MEM_A_PINLOC                       = 0,
   parameter PORT_MEM_BA_PINLOC                      = 0,
   parameter PORT_MEM_BG_PINLOC                      = 0,
   parameter PORT_MEM_CS_N_PINLOC                    = 0,
   parameter PORT_MEM_ACT_N_PINLOC                   = 0,
   parameter PORT_MEM_DQ_PINLOC                      = 0,
   parameter PORT_MEM_DM_PINLOC                      = 0,
   parameter PORT_MEM_DBI_N_PINLOC                   = 0,
   parameter PORT_MEM_RAS_N_PINLOC                   = 0,
   parameter PORT_MEM_CAS_N_PINLOC                   = 0,
   parameter PORT_MEM_WE_N_PINLOC                    = 0,
   parameter PORT_MEM_REF_N_PINLOC                   = 0,

   parameter PORT_MEM_WPS_N_PINLOC                   = 0,
   parameter PORT_MEM_RPS_N_PINLOC                   = 0,
   parameter PORT_MEM_BWS_N_PINLOC                   = 0,
   parameter PORT_MEM_DQA_PINLOC                     = 0,
   parameter PORT_MEM_DQB_PINLOC                     = 0,
   parameter PORT_MEM_Q_PINLOC                       = 0,
   parameter PORT_MEM_D_PINLOC                       = 0,
   parameter PORT_MEM_RWA_N_PINLOC                   = 0,
   parameter PORT_MEM_RWB_N_PINLOC                   = 0,
   parameter PORT_MEM_QKA_PINLOC                     = 0,
   parameter PORT_MEM_QKB_PINLOC                     = 0,
   parameter PORT_MEM_LDA_N_PINLOC                   = 0,
   parameter PORT_MEM_LDB_N_PINLOC                   = 0,
   parameter PORT_MEM_CK_PINLOC                      = 0,
   parameter PORT_MEM_DINVA_PINLOC                   = 0,
   parameter PORT_MEM_DINVB_PINLOC                   = 0,
   parameter PORT_MEM_AINV_PINLOC                    = 0,

   parameter PORT_MEM_A_WIDTH                        = 0,
   parameter PORT_MEM_BA_WIDTH                       = 0,
   parameter PORT_MEM_BG_WIDTH                       = 0,
   parameter PORT_MEM_CS_N_WIDTH                     = 0,
   parameter PORT_MEM_ACT_N_WIDTH                    = 0,
   parameter PORT_MEM_DBI_N_WIDTH                    = 0,
   parameter PORT_MEM_RAS_N_WIDTH                    = 0,
   parameter PORT_MEM_CAS_N_WIDTH                    = 0,
   parameter PORT_MEM_WE_N_WIDTH                     = 0,
   parameter PORT_MEM_DM_WIDTH                       = 0,
   parameter PORT_MEM_REF_N_WIDTH                    = 0,
   parameter PORT_MEM_WPS_N_WIDTH                    = 0,
   parameter PORT_MEM_RPS_N_WIDTH                    = 0,
   parameter PORT_MEM_BWS_N_WIDTH                    = 0,
   parameter PORT_MEM_DQA_WIDTH                      = 0,
   parameter PORT_MEM_DQB_WIDTH                      = 0,
   parameter PORT_MEM_Q_WIDTH                        = 0,
   parameter PORT_MEM_D_WIDTH                        = 0,
   parameter PORT_MEM_RWA_N_WIDTH                    = 0,
   parameter PORT_MEM_RWB_N_WIDTH                    = 0,
   parameter PORT_MEM_QKA_WIDTH                      = 0,
   parameter PORT_MEM_QKB_WIDTH                      = 0,
   parameter PORT_MEM_LDA_N_WIDTH                    = 0,
   parameter PORT_MEM_LDB_N_WIDTH                    = 0,
   parameter PORT_MEM_CK_WIDTH                       = 0,
   parameter PORT_MEM_DINVA_WIDTH                    = 0,
   parameter PORT_MEM_DINVB_WIDTH                    = 0,
   parameter PORT_MEM_AINV_WIDTH                     = 0,
   parameter DIAG_USE_ABSTRACT_PHY                   = 0,
   parameter DIAG_ABSTRACT_PHY_WLAT                  = 0,
   parameter DIAG_ABSTRACT_PHY_RLAT                  = 0,
   parameter ABPHY_WRITE_PROTOCOL                    = 1
) (
   // Reset
   input  logic                                                                                  global_reset_n_int,  //__ACDS_USER_COMMNET__ Async reset signal from user
   output logic                                                                                  phy_reset_n,         // Async reset signal from reset circuitry in the tile

   // Signals for various signals from PLL
   input  logic                                                                                  pll_locked,          // Indicates PLL lock status
   input  logic                                                                                  pll_dll_clk,         // PLL -> DLL output clock
   input  logic [7:0]                                                                            phy_clk_phs,         // FR PHY clock signals (8 phases, 45-deg apart)
   input  logic [1:0]                                                                            phy_clk,             // {phy_clk[1], phy_clk[0]}
   input  logic                                                                                  phy_fb_clk_to_tile,  // PHY feedback clock (to tile)
   output logic                                                                                  phy_fb_clk_to_pll,   // PHY feedback clock (to PLL)

   // Core clock signals from/to the Clock Phase Alignment (CPA) block
   output logic [1:0]                                                                            core_clks_from_cpa_pri,   // Core clock signals from the CPA of primary interface
   output logic [1:0]                                                                            core_clks_locked_cpa_pri, // Core clock locked signals from the CPA of primary interface
   input  logic [1:0]                                                                            core_clks_fb_to_cpa_pri,  // Core clock feedback signals to the CPA of primary interface
   output logic [1:0]                                                                            core_clks_from_cpa_sec,   // Core clock signals from the CPA of secondary interface (ping-pong only)
   output logic [1:0]                                                                            core_clks_locked_cpa_sec, // Core clock locked signals from the CPA of secondary interface (ping-pong only)
   input  logic [1:0]                                                                            core_clks_fb_to_cpa_sec,  // Core clock feedback signals to the CPA of secondary interface (ping-pong only)

   // Avalon interfaces between core and HMC
   input  logic [59:0]                                                                           core2ctl_avl_0,
   input  logic [59:0]                                                                           core2ctl_avl_1,
   input  logic                                                                                  core2ctl_avl_rd_data_ready_0,
   input  logic                                                                                  core2ctl_avl_rd_data_ready_1,
   output logic                                                                                  ctl2core_avl_cmd_ready_0,
   output logic                                                                                  ctl2core_avl_cmd_ready_1,
   output logic [12:0]                                                                           ctl2core_avl_rdata_id_0,
   output logic [12:0]                                                                           ctl2core_avl_rdata_id_1,
   input  logic                                                                                  core2l_wr_data_vld_ast_0,
   input  logic                                                                                  core2l_wr_data_vld_ast_1,
   input  logic                                                                                  core2l_rd_data_rdy_ast_0,
   input  logic                                                                                  core2l_rd_data_rdy_ast_1,

   // Avalon interfaces between core and lanes
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_rd_data_vld_avl0,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_wr_data_rdy_ast,

   // ECC signals between core and lanes
   input  logic [12:0]                                                                           core2l_wr_ecc_info_0,
   input  logic [12:0]                                                                           core2l_wr_ecc_info_1,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][11:0]                                 l2core_wb_pointer_for_ecc,

   // Signals between core and data lanes
   input  logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 8 - 1:0]              core2l_data,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 8 - 1:0]              l2core_data,
   input  logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 4 - 1:0]              core2l_oe,
   input  logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][3:0]                                  core2l_rdata_en_full,
   input  logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][15:0]                                 core2l_mrnk_read,
   input  logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][15:0]                                 core2l_mrnk_write,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][3:0]                                  l2core_rdata_valid,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_rlat,
   output logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_wlat,

   // AFI signals between tile and core
   input  [16:0]                                                                                 c2t_afi,
   output [25:0]                                                                                 t2c_afi,

   // Side-band signals between core and HMC
   input  logic [41:0]                                                                           core2ctl_sideband_0,
   output logic [13:0]                                                                           ctl2core_sideband_0,
   input  logic [41:0]                                                                           core2ctl_sideband_1,
   output logic [13:0]                                                                           ctl2core_sideband_1,

   // MMR signals between core and HMC
   output logic [33:0]                                                                           ctl2core_mmr_0,
   input  logic [50:0]                                                                           core2ctl_mmr_0,
   output logic [33:0]                                                                           ctl2core_mmr_1,
   input  logic [50:0]                                                                           core2ctl_mmr_1,

   // Signals between I/O buffers and lanes/tiles
   output logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_data,         // lane-to-buffer data
   output logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_oe,           // lane-to-buffer output-enable
   output logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_dtc,          // lane-to-buffer dynamic-termination-control
   input  logic [PINS_IN_RTL_TILES-1:0]                                                          b2l_data,         // buffer-to-lane data
   input  logic [LANES_IN_RTL_TILES-1:0]                                                         b2t_dqs,          // buffer-to-tile DQS
   input  logic [LANES_IN_RTL_TILES-1:0]                                                         b2t_dqsb,         // buffer-to-tile DQSb

   // Avalon-MM bus for the calibration commands between io_aux and tiles
   input  logic                                                                                  cal_bus_clk,
   input  logic                                                                                  cal_bus_avl_read,
   input  logic                                                                                  cal_bus_avl_write,
   input  logic [19:0]                                                                           cal_bus_avl_address,
   output logic [31:0]                                                                           cal_bus_avl_read_data,
   input  logic [31:0]                                                                           cal_bus_avl_write_data,

   // Ports for internal test and debug
   input  logic                                                                                  pa_dprio_clk,
   input  logic                                                                                  pa_dprio_read,
   input  logic [PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH-1:0]                                        pa_dprio_reg_addr,
   input  logic                                                                                  pa_dprio_rst_n,
   input  logic                                                                                  pa_dprio_write,
   input  logic [PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH-1:0]                                       pa_dprio_writedata,
   output logic                                                                                  pa_dprio_block_select,
   output logic [PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH-1:0]                                        pa_dprio_readdata,
   output logic                                                                                  runAbstractPhySim,
   input logic                                                                                   afi_cal_success
);
   timeunit 1ns;
   timeprecision 1ps;

     logic                                                                                  phy_reset_n_abphy;
     logic                                                                                  phy_fb_clk_to_pll_abphy;
     logic [1:0]                                                                            core_clks_from_cpa_pri_abphy;
     logic [1:0]                                                                            core_clks_locked_cpa_pri_abphy;
     logic [1:0]                                                                            core_clks_from_cpa_sec_abphy;
     logic [1:0]                                                                            core_clks_locked_cpa_sec_abphy;
     logic                                                                                  ctl2core_avl_cmd_ready_0_abphy;
     logic                                                                                  ctl2core_avl_cmd_ready_1_abphy;
     logic [12:0]                                                                           ctl2core_avl_rdata_id_0_abphy;
     logic [12:0]                                                                           ctl2core_avl_rdata_id_1_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_rd_data_vld_avl0_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_wr_data_rdy_ast_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][11:0]                                 l2core_wb_pointer_for_ecc_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 8 - 1:0]              l2core_data_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][3:0]                                  l2core_rdata_valid_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_rlat_abphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_wlat_abphy;
     logic [25:0]                                                                           t2c_afi_abphy;
     logic [13:0]                                                                           ctl2core_sideband_0_abphy;
     logic [13:0]                                                                           ctl2core_sideband_1_abphy;
     logic [33:0]                                                                           ctl2core_mmr_0_abphy;
     logic [33:0]                                                                           ctl2core_mmr_1_abphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_data_abphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_oe_abphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_dtc_abphy;
     logic                                                                                  pa_dprio_block_select_abphy;
     logic [PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH-1:0]                                        pa_dprio_readdata_abphy;
     logic                                                                                  phy_reset_n_nonabphy;
     logic                                                                                  phy_fb_clk_to_pll_nonabphy;
     logic [1:0]                                                                            core_clks_from_cpa_pri_nonabphy;
     logic [1:0]                                                                            core_clks_locked_cpa_pri_nonabphy;
     logic [1:0]                                                                            core_clks_from_cpa_sec_nonabphy;
     logic [1:0]                                                                            core_clks_locked_cpa_sec_nonabphy;
     logic                                                                                  ctl2core_avl_cmd_ready_0_nonabphy;
     logic                                                                                  ctl2core_avl_cmd_ready_1_nonabphy;
     logic [12:0]                                                                           ctl2core_avl_rdata_id_0_nonabphy;
     logic [12:0]                                                                           ctl2core_avl_rdata_id_1_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_rd_data_vld_avl0_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0]                                       l2core_wr_data_rdy_ast_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][11:0]                                 l2core_wb_pointer_for_ecc_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 8 - 1:0]              l2core_data_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][3:0]                                  l2core_rdata_valid_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_rlat_nonabphy;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                                  l2core_afi_wlat_nonabphy;
     logic [25:0]                                                                           t2c_afi_nonabphy;
     logic [13:0]                                                                           ctl2core_sideband_0_nonabphy;
     logic [13:0]                                                                           ctl2core_sideband_1_nonabphy;
     logic [33:0]                                                                           ctl2core_mmr_0_nonabphy;
     logic [33:0]                                                                           ctl2core_mmr_1_nonabphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_data_nonabphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_oe_nonabphy;
     logic [PINS_IN_RTL_TILES-1:0]                                                          l2b_dtc_nonabphy;
     logic                                                                                  pa_dprio_block_select_nonabphy;
     logic [PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH-1:0]                                        pa_dprio_readdata_nonabphy;

     logic                                                                                  global_reset_n_int_iotile_in;
     logic                                                                                  pll_locked_iotile_in;
     logic                                                                                  pll_dll_clk_iotile_in;
     logic [7:0]                                                                            phy_clk_phs_iotile_in;
     logic [1:0]                                                                            phy_clk_iotile_in;
     logic                                                                                  phy_fb_clk_to_tile_iotile_in;
     logic [1:0]                                                                            core_clks_fb_to_cpa_pri_iotile_in;
     logic [1:0]                                                                            core_clks_fb_to_cpa_sec_iotile_in;
     logic [59:0]                                                                           core2ctl_avl_0_iotile_in;
     logic [59:0]                                                                           core2ctl_avl_1_iotile_in;
     logic                                                                                  core2ctl_avl_rd_data_ready_0_iotile_in;
     logic                                                                                  core2ctl_avl_rd_data_ready_1_iotile_in;
     logic                                                                                  core2l_wr_data_vld_ast_0_iotile_in;
     logic                                                                                  core2l_wr_data_vld_ast_1_iotile_in;
     logic                                                                                  core2l_rd_data_rdy_ast_0_iotile_in;
     logic                                                                                  core2l_rd_data_rdy_ast_1_iotile_in;
     logic [12:0]                                                                           core2l_wr_ecc_info_0_iotile_in;
     logic [12:0]                                                                           core2l_wr_ecc_info_1_iotile_in;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 8 - 1:0]              core2l_data_iotile_in;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][PINS_PER_LANE * 4 - 1:0]              core2l_oe_iotile_in;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][3:0]                                  core2l_rdata_en_full_iotile_in;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][15:0]                                 core2l_mrnk_read_iotile_in;
     logic [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][15:0]                                 core2l_mrnk_write_iotile_in;
     logic [16:0]                                                                           c2t_afi_iotile_in;
     logic [41:0]                                                                           core2ctl_sideband_0_iotile_in;
     logic [41:0]                                                                           core2ctl_sideband_1_iotile_in;
     logic [50:0]                                                                           core2ctl_mmr_0_iotile_in;
     logic [50:0]                                                                           core2ctl_mmr_1_iotile_in;
     logic [PINS_IN_RTL_TILES-1:0]                                                          b2l_data_iotile_in;
     logic [LANES_IN_RTL_TILES-1:0]                                                         b2t_dqs_iotile_in;
     logic [LANES_IN_RTL_TILES-1:0]                                                         b2t_dqsb_iotile_in;
     logic                                                                                  cal_bus_clk_iotile_in;
     logic                                                                                  cal_bus_avl_read_iotile_in;
     logic                                                                                  cal_bus_avl_write_iotile_in;
     logic [19:0]                                                                           cal_bus_avl_address_iotile_in;
     logic [31:0]                                                                           cal_bus_avl_write_data_iotile_in;
     logic                                                                                  pa_dprio_clk_iotile_in;
     logic                                                                                  pa_dprio_read_iotile_in;
     logic [PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH-1:0]                                        pa_dprio_reg_addr_iotile_in;
     logic                                                                                  pa_dprio_rst_n_iotile_in;
     logic                                                                                  pa_dprio_write_iotile_in;
     logic [PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH-1:0]                                       pa_dprio_writedata_iotile_in;


   ////////////////////////////////////////////////////////////////////////////
   // Tiles and Lanes
   ////////////////////////////////////////////////////////////////////////////
   altera_emif_arch_nf_io_tiles # (
      .DIAG_SYNTH_FOR_SIM                   (DIAG_SYNTH_FOR_SIM),
      .DIAG_CPA_OUT_1_EN                    (DIAG_CPA_OUT_1_EN),
      .DIAG_FAST_SIM                        (DIAG_FAST_SIM),
      .IS_HPS                               (IS_HPS),
      .SILICON_REV                          (SILICON_REV),
      .PROTOCOL_ENUM                        (PROTOCOL_ENUM),
      .PHY_PING_PONG_EN                     (PHY_PING_PONG_EN),
      .DQS_BUS_MODE_ENUM                    (DQS_BUS_MODE_ENUM),
      .USER_CLK_RATIO                       (USER_CLK_RATIO),
      .PHY_HMC_CLK_RATIO                    (PHY_HMC_CLK_RATIO),
      .C2P_P2C_CLK_RATIO                    (C2P_P2C_CLK_RATIO),
      .PLL_VCO_FREQ_MHZ_INT                 (PLL_VCO_FREQ_MHZ_INT),
      .PLL_VCO_TO_MEM_CLK_FREQ_RATIO        (PLL_VCO_TO_MEM_CLK_FREQ_RATIO),
      .MEM_BURST_LENGTH                     (MEM_BURST_LENGTH),
      .MEM_DATA_MASK_EN                     (MEM_DATA_MASK_EN),
      .NUM_OF_HMC_PORTS                     (NUM_OF_HMC_PORTS),
      .HMC_AVL_PROTOCOL_ENUM                (HMC_AVL_PROTOCOL_ENUM),
      .HMC_CTRL_DIMM_TYPE                   (HMC_CTRL_DIMM_TYPE),
      .PRI_HMC_CFG_ENABLE_ECC               (PRI_HMC_CFG_ENABLE_ECC),
      .PRI_HMC_CFG_REORDER_DATA             (PRI_HMC_CFG_REORDER_DATA),
      .PRI_HMC_CFG_REORDER_READ             (PRI_HMC_CFG_REORDER_READ),
      .PRI_HMC_CFG_REORDER_RDATA            (PRI_HMC_CFG_REORDER_RDATA),
      .PRI_HMC_CFG_STARVE_LIMIT             (PRI_HMC_CFG_STARVE_LIMIT),
      .PRI_HMC_CFG_DQS_TRACKING_EN          (PRI_HMC_CFG_DQS_TRACKING_EN),
      .PRI_HMC_CFG_ARBITER_TYPE             (PRI_HMC_CFG_ARBITER_TYPE),
      .PRI_HMC_CFG_OPEN_PAGE_EN             (PRI_HMC_CFG_OPEN_PAGE_EN),
      .PRI_HMC_CFG_GEAR_DOWN_EN             (PRI_HMC_CFG_GEAR_DOWN_EN),
      .PRI_HMC_CFG_RLD3_MULTIBANK_MODE      (PRI_HMC_CFG_RLD3_MULTIBANK_MODE),
      .PRI_HMC_CFG_PING_PONG_MODE           (PRI_HMC_CFG_PING_PONG_MODE),
      .PRI_HMC_CFG_SLOT_ROTATE_EN           (PRI_HMC_CFG_SLOT_ROTATE_EN),
      .PRI_HMC_CFG_SLOT_OFFSET              (PRI_HMC_CFG_SLOT_OFFSET),
      .PRI_HMC_CFG_COL_CMD_SLOT             (PRI_HMC_CFG_COL_CMD_SLOT),
      .PRI_HMC_CFG_ROW_CMD_SLOT             (PRI_HMC_CFG_ROW_CMD_SLOT),
      .PRI_HMC_CFG_ENABLE_RC                (PRI_HMC_CFG_ENABLE_RC),
      .PRI_HMC_CFG_CS_TO_CHIP_MAPPING       (PRI_HMC_CFG_CS_TO_CHIP_MAPPING),
      .PRI_HMC_CFG_RB_RESERVED_ENTRY        (PRI_HMC_CFG_RB_RESERVED_ENTRY),
      .PRI_HMC_CFG_WB_RESERVED_ENTRY        (PRI_HMC_CFG_WB_RESERVED_ENTRY),
      .PRI_HMC_CFG_TCL                      (PRI_HMC_CFG_TCL),
      .PRI_HMC_CFG_POWER_SAVING_EXIT_CYC    (PRI_HMC_CFG_POWER_SAVING_EXIT_CYC),
      .PRI_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC(PRI_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC),
      .PRI_HMC_CFG_WRITE_ODT_CHIP           (PRI_HMC_CFG_WRITE_ODT_CHIP),
      .PRI_HMC_CFG_READ_ODT_CHIP            (PRI_HMC_CFG_READ_ODT_CHIP),
      .PRI_HMC_CFG_WR_ODT_ON                (PRI_HMC_CFG_WR_ODT_ON),
      .PRI_HMC_CFG_RD_ODT_ON                (PRI_HMC_CFG_RD_ODT_ON),
      .PRI_HMC_CFG_WR_ODT_PERIOD            (PRI_HMC_CFG_WR_ODT_PERIOD),
      .PRI_HMC_CFG_RD_ODT_PERIOD            (PRI_HMC_CFG_RD_ODT_PERIOD),
      .PRI_HMC_CFG_RLD3_REFRESH_SEQ0        (PRI_HMC_CFG_RLD3_REFRESH_SEQ0),
      .PRI_HMC_CFG_RLD3_REFRESH_SEQ1        (PRI_HMC_CFG_RLD3_REFRESH_SEQ1),
      .PRI_HMC_CFG_RLD3_REFRESH_SEQ2        (PRI_HMC_CFG_RLD3_REFRESH_SEQ2),
      .PRI_HMC_CFG_RLD3_REFRESH_SEQ3        (PRI_HMC_CFG_RLD3_REFRESH_SEQ3),
      .PRI_HMC_CFG_SRF_ZQCAL_DISABLE        (PRI_HMC_CFG_SRF_ZQCAL_DISABLE),
      .PRI_HMC_CFG_MPS_ZQCAL_DISABLE        (PRI_HMC_CFG_MPS_ZQCAL_DISABLE),
      .PRI_HMC_CFG_MPS_DQSTRK_DISABLE       (PRI_HMC_CFG_MPS_DQSTRK_DISABLE),
      .PRI_HMC_CFG_SHORT_DQSTRK_CTRL_EN     (PRI_HMC_CFG_SHORT_DQSTRK_CTRL_EN),
      .PRI_HMC_CFG_PERIOD_DQSTRK_CTRL_EN    (PRI_HMC_CFG_PERIOD_DQSTRK_CTRL_EN),
      .PRI_HMC_CFG_PERIOD_DQSTRK_INTERVAL   (PRI_HMC_CFG_PERIOD_DQSTRK_INTERVAL),
      .PRI_HMC_CFG_DQSTRK_TO_VALID_LAST     (PRI_HMC_CFG_DQSTRK_TO_VALID_LAST),
      .PRI_HMC_CFG_DQSTRK_TO_VALID          (PRI_HMC_CFG_DQSTRK_TO_VALID),
      .PRI_HMC_CFG_RFSH_WARN_THRESHOLD      (PRI_HMC_CFG_RFSH_WARN_THRESHOLD),
      .PRI_HMC_CFG_SB_CG_DISABLE            (PRI_HMC_CFG_SB_CG_DISABLE),
      .PRI_HMC_CFG_USER_RFSH_EN             (PRI_HMC_CFG_USER_RFSH_EN),
      .PRI_HMC_CFG_SRF_AUTOEXIT_EN          (PRI_HMC_CFG_SRF_AUTOEXIT_EN),
      .PRI_HMC_CFG_SRF_ENTRY_EXIT_BLOCK     (PRI_HMC_CFG_SRF_ENTRY_EXIT_BLOCK),
      .PRI_HMC_CFG_SB_DDR4_MR3              (PRI_HMC_CFG_SB_DDR4_MR3),
      .PRI_HMC_CFG_SB_DDR4_MR4              (PRI_HMC_CFG_SB_DDR4_MR4),
      .PRI_HMC_CFG_SB_DDR4_MR5              (PRI_HMC_CFG_SB_DDR4_MR5),
      .PRI_HMC_CFG_DDR4_MPS_ADDR_MIRROR     (PRI_HMC_CFG_DDR4_MPS_ADDR_MIRROR),
      .PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH     (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH),
      .PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH     (PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH),
      .PRI_HMC_CFG_MEM_IF_BANKADDR_WIDTH    (PRI_HMC_CFG_MEM_IF_BANKADDR_WIDTH),
      .PRI_HMC_CFG_MEM_IF_BGADDR_WIDTH      (PRI_HMC_CFG_MEM_IF_BGADDR_WIDTH),
      .PRI_HMC_CFG_LOCAL_IF_CS_WIDTH        (PRI_HMC_CFG_LOCAL_IF_CS_WIDTH),
      .PRI_HMC_CFG_ADDR_ORDER               (PRI_HMC_CFG_ADDR_ORDER),
      .PRI_HMC_CFG_ACT_TO_RDWR              (PRI_HMC_CFG_ACT_TO_RDWR),
      .PRI_HMC_CFG_ACT_TO_PCH               (PRI_HMC_CFG_ACT_TO_PCH),
      .PRI_HMC_CFG_ACT_TO_ACT               (PRI_HMC_CFG_ACT_TO_ACT),
      .PRI_HMC_CFG_ACT_TO_ACT_DIFF_BANK     (PRI_HMC_CFG_ACT_TO_ACT_DIFF_BANK),
      .PRI_HMC_CFG_ACT_TO_ACT_DIFF_BG       (PRI_HMC_CFG_ACT_TO_ACT_DIFF_BG),
      .PRI_HMC_CFG_RD_TO_RD                 (PRI_HMC_CFG_RD_TO_RD),
      .PRI_HMC_CFG_RD_TO_RD_DIFF_CHIP       (PRI_HMC_CFG_RD_TO_RD_DIFF_CHIP),
      .PRI_HMC_CFG_RD_TO_RD_DIFF_BG         (PRI_HMC_CFG_RD_TO_RD_DIFF_BG),
      .PRI_HMC_CFG_RD_TO_WR                 (PRI_HMC_CFG_RD_TO_WR),
      .PRI_HMC_CFG_RD_TO_WR_DIFF_CHIP       (PRI_HMC_CFG_RD_TO_WR_DIFF_CHIP),
      .PRI_HMC_CFG_RD_TO_WR_DIFF_BG         (PRI_HMC_CFG_RD_TO_WR_DIFF_BG),
      .PRI_HMC_CFG_RD_TO_PCH                (PRI_HMC_CFG_RD_TO_PCH),
      .PRI_HMC_CFG_RD_AP_TO_VALID           (PRI_HMC_CFG_RD_AP_TO_VALID),
      .PRI_HMC_CFG_WR_TO_WR                 (PRI_HMC_CFG_WR_TO_WR),
      .PRI_HMC_CFG_WR_TO_WR_DIFF_CHIP       (PRI_HMC_CFG_WR_TO_WR_DIFF_CHIP),
      .PRI_HMC_CFG_WR_TO_WR_DIFF_BG         (PRI_HMC_CFG_WR_TO_WR_DIFF_BG),
      .PRI_HMC_CFG_WR_TO_RD                 (PRI_HMC_CFG_WR_TO_RD),
      .PRI_HMC_CFG_WR_TO_RD_DIFF_CHIP       (PRI_HMC_CFG_WR_TO_RD_DIFF_CHIP),
      .PRI_HMC_CFG_WR_TO_RD_DIFF_BG         (PRI_HMC_CFG_WR_TO_RD_DIFF_BG),
      .PRI_HMC_CFG_WR_TO_PCH                (PRI_HMC_CFG_WR_TO_PCH),
      .PRI_HMC_CFG_WR_AP_TO_VALID           (PRI_HMC_CFG_WR_AP_TO_VALID),
      .PRI_HMC_CFG_PCH_TO_VALID             (PRI_HMC_CFG_PCH_TO_VALID),
      .PRI_HMC_CFG_PCH_ALL_TO_VALID         (PRI_HMC_CFG_PCH_ALL_TO_VALID),
      .PRI_HMC_CFG_ARF_TO_VALID             (PRI_HMC_CFG_ARF_TO_VALID),
      .PRI_HMC_CFG_PDN_TO_VALID             (PRI_HMC_CFG_PDN_TO_VALID),
      .PRI_HMC_CFG_SRF_TO_VALID             (PRI_HMC_CFG_SRF_TO_VALID),
      .PRI_HMC_CFG_SRF_TO_ZQ_CAL            (PRI_HMC_CFG_SRF_TO_ZQ_CAL),
      .PRI_HMC_CFG_ARF_PERIOD               (PRI_HMC_CFG_ARF_PERIOD),
      .PRI_HMC_CFG_PDN_PERIOD               (PRI_HMC_CFG_PDN_PERIOD),
      .PRI_HMC_CFG_ZQCL_TO_VALID            (PRI_HMC_CFG_ZQCL_TO_VALID),
      .PRI_HMC_CFG_ZQCS_TO_VALID            (PRI_HMC_CFG_ZQCS_TO_VALID),
      .PRI_HMC_CFG_MRS_TO_VALID             (PRI_HMC_CFG_MRS_TO_VALID),
      .PRI_HMC_CFG_MPS_TO_VALID             (PRI_HMC_CFG_MPS_TO_VALID),
      .PRI_HMC_CFG_MRR_TO_VALID             (PRI_HMC_CFG_MRR_TO_VALID),
      .PRI_HMC_CFG_MPR_TO_VALID             (PRI_HMC_CFG_MPR_TO_VALID),
      .PRI_HMC_CFG_MPS_EXIT_CS_TO_CKE       (PRI_HMC_CFG_MPS_EXIT_CS_TO_CKE),
      .PRI_HMC_CFG_MPS_EXIT_CKE_TO_CS       (PRI_HMC_CFG_MPS_EXIT_CKE_TO_CS),
      .PRI_HMC_CFG_RLD3_MULTIBANK_REF_DELAY (PRI_HMC_CFG_RLD3_MULTIBANK_REF_DELAY),
      .PRI_HMC_CFG_MMR_CMD_TO_VALID         (PRI_HMC_CFG_MMR_CMD_TO_VALID),
      .PRI_HMC_CFG_4_ACT_TO_ACT             (PRI_HMC_CFG_4_ACT_TO_ACT),
      .PRI_HMC_CFG_16_ACT_TO_ACT            (PRI_HMC_CFG_16_ACT_TO_ACT),

      .SEC_HMC_CFG_ENABLE_ECC               (SEC_HMC_CFG_ENABLE_ECC),
      .SEC_HMC_CFG_REORDER_DATA             (SEC_HMC_CFG_REORDER_DATA),
      .SEC_HMC_CFG_REORDER_READ             (SEC_HMC_CFG_REORDER_READ),
      .SEC_HMC_CFG_REORDER_RDATA            (SEC_HMC_CFG_REORDER_RDATA),
      .SEC_HMC_CFG_STARVE_LIMIT             (SEC_HMC_CFG_STARVE_LIMIT),
      .SEC_HMC_CFG_DQS_TRACKING_EN          (SEC_HMC_CFG_DQS_TRACKING_EN),
      .SEC_HMC_CFG_ARBITER_TYPE             (SEC_HMC_CFG_ARBITER_TYPE),
      .SEC_HMC_CFG_OPEN_PAGE_EN             (SEC_HMC_CFG_OPEN_PAGE_EN),
      .SEC_HMC_CFG_GEAR_DOWN_EN             (SEC_HMC_CFG_GEAR_DOWN_EN),
      .SEC_HMC_CFG_RLD3_MULTIBANK_MODE      (SEC_HMC_CFG_RLD3_MULTIBANK_MODE),
      .SEC_HMC_CFG_PING_PONG_MODE           (SEC_HMC_CFG_PING_PONG_MODE),
      .SEC_HMC_CFG_SLOT_ROTATE_EN           (SEC_HMC_CFG_SLOT_ROTATE_EN),
      .SEC_HMC_CFG_SLOT_OFFSET              (SEC_HMC_CFG_SLOT_OFFSET),
      .SEC_HMC_CFG_COL_CMD_SLOT             (SEC_HMC_CFG_COL_CMD_SLOT),
      .SEC_HMC_CFG_ROW_CMD_SLOT             (SEC_HMC_CFG_ROW_CMD_SLOT),
      .SEC_HMC_CFG_ENABLE_RC                (SEC_HMC_CFG_ENABLE_RC),
      .SEC_HMC_CFG_CS_TO_CHIP_MAPPING       (SEC_HMC_CFG_CS_TO_CHIP_MAPPING),
      .SEC_HMC_CFG_RB_RESERVED_ENTRY        (SEC_HMC_CFG_RB_RESERVED_ENTRY),
      .SEC_HMC_CFG_WB_RESERVED_ENTRY        (SEC_HMC_CFG_WB_RESERVED_ENTRY),
      .SEC_HMC_CFG_TCL                      (SEC_HMC_CFG_TCL),
      .SEC_HMC_CFG_POWER_SAVING_EXIT_CYC    (SEC_HMC_CFG_POWER_SAVING_EXIT_CYC),
      .SEC_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC(SEC_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC),
      .SEC_HMC_CFG_WRITE_ODT_CHIP           (SEC_HMC_CFG_WRITE_ODT_CHIP),
      .SEC_HMC_CFG_READ_ODT_CHIP            (SEC_HMC_CFG_READ_ODT_CHIP),
      .SEC_HMC_CFG_WR_ODT_ON                (SEC_HMC_CFG_WR_ODT_ON),
      .SEC_HMC_CFG_RD_ODT_ON                (SEC_HMC_CFG_RD_ODT_ON),
      .SEC_HMC_CFG_WR_ODT_PERIOD            (SEC_HMC_CFG_WR_ODT_PERIOD),
      .SEC_HMC_CFG_RD_ODT_PERIOD            (SEC_HMC_CFG_RD_ODT_PERIOD),
      .SEC_HMC_CFG_RLD3_REFRESH_SEQ0        (SEC_HMC_CFG_RLD3_REFRESH_SEQ0),
      .SEC_HMC_CFG_RLD3_REFRESH_SEQ1        (SEC_HMC_CFG_RLD3_REFRESH_SEQ1),
      .SEC_HMC_CFG_RLD3_REFRESH_SEQ2        (SEC_HMC_CFG_RLD3_REFRESH_SEQ2),
      .SEC_HMC_CFG_RLD3_REFRESH_SEQ3        (SEC_HMC_CFG_RLD3_REFRESH_SEQ3),
      .SEC_HMC_CFG_SRF_ZQCAL_DISABLE        (SEC_HMC_CFG_SRF_ZQCAL_DISABLE),
      .SEC_HMC_CFG_MPS_ZQCAL_DISABLE        (SEC_HMC_CFG_MPS_ZQCAL_DISABLE),
      .SEC_HMC_CFG_MPS_DQSTRK_DISABLE       (SEC_HMC_CFG_MPS_DQSTRK_DISABLE),
      .SEC_HMC_CFG_SHORT_DQSTRK_CTRL_EN     (SEC_HMC_CFG_SHORT_DQSTRK_CTRL_EN),
      .SEC_HMC_CFG_PERIOD_DQSTRK_CTRL_EN    (SEC_HMC_CFG_PERIOD_DQSTRK_CTRL_EN),
      .SEC_HMC_CFG_PERIOD_DQSTRK_INTERVAL   (SEC_HMC_CFG_PERIOD_DQSTRK_INTERVAL),
      .SEC_HMC_CFG_DQSTRK_TO_VALID_LAST     (SEC_HMC_CFG_DQSTRK_TO_VALID_LAST),
      .SEC_HMC_CFG_DQSTRK_TO_VALID          (SEC_HMC_CFG_DQSTRK_TO_VALID),
      .SEC_HMC_CFG_RFSH_WARN_THRESHOLD      (SEC_HMC_CFG_RFSH_WARN_THRESHOLD),
      .SEC_HMC_CFG_SB_CG_DISABLE            (SEC_HMC_CFG_SB_CG_DISABLE),
      .SEC_HMC_CFG_USER_RFSH_EN             (SEC_HMC_CFG_USER_RFSH_EN),
      .SEC_HMC_CFG_SRF_AUTOEXIT_EN          (SEC_HMC_CFG_SRF_AUTOEXIT_EN),
      .SEC_HMC_CFG_SRF_ENTRY_EXIT_BLOCK     (SEC_HMC_CFG_SRF_ENTRY_EXIT_BLOCK),
      .SEC_HMC_CFG_SB_DDR4_MR3              (SEC_HMC_CFG_SB_DDR4_MR3),
      .SEC_HMC_CFG_SB_DDR4_MR4              (SEC_HMC_CFG_SB_DDR4_MR4),
      .SEC_HMC_CFG_SB_DDR4_MR5              (SEC_HMC_CFG_SB_DDR4_MR5),
      .SEC_HMC_CFG_DDR4_MPS_ADDR_MIRROR     (SEC_HMC_CFG_DDR4_MPS_ADDR_MIRROR),
      .SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH     (SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH),
      .SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH     (SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH),
      .SEC_HMC_CFG_MEM_IF_BANKADDR_WIDTH    (SEC_HMC_CFG_MEM_IF_BANKADDR_WIDTH),
      .SEC_HMC_CFG_MEM_IF_BGADDR_WIDTH      (SEC_HMC_CFG_MEM_IF_BGADDR_WIDTH),
      .SEC_HMC_CFG_LOCAL_IF_CS_WIDTH        (SEC_HMC_CFG_LOCAL_IF_CS_WIDTH),
      .SEC_HMC_CFG_ADDR_ORDER               (SEC_HMC_CFG_ADDR_ORDER),
      .SEC_HMC_CFG_ACT_TO_RDWR              (SEC_HMC_CFG_ACT_TO_RDWR),
      .SEC_HMC_CFG_ACT_TO_PCH               (SEC_HMC_CFG_ACT_TO_PCH),
      .SEC_HMC_CFG_ACT_TO_ACT               (SEC_HMC_CFG_ACT_TO_ACT),
      .SEC_HMC_CFG_ACT_TO_ACT_DIFF_BANK     (SEC_HMC_CFG_ACT_TO_ACT_DIFF_BANK),
      .SEC_HMC_CFG_ACT_TO_ACT_DIFF_BG       (SEC_HMC_CFG_ACT_TO_ACT_DIFF_BG),
      .SEC_HMC_CFG_RD_TO_RD                 (SEC_HMC_CFG_RD_TO_RD),
      .SEC_HMC_CFG_RD_TO_RD_DIFF_CHIP       (SEC_HMC_CFG_RD_TO_RD_DIFF_CHIP),
      .SEC_HMC_CFG_RD_TO_RD_DIFF_BG         (SEC_HMC_CFG_RD_TO_RD_DIFF_BG),
      .SEC_HMC_CFG_RD_TO_WR                 (SEC_HMC_CFG_RD_TO_WR),
      .SEC_HMC_CFG_RD_TO_WR_DIFF_CHIP       (SEC_HMC_CFG_RD_TO_WR_DIFF_CHIP),
      .SEC_HMC_CFG_RD_TO_WR_DIFF_BG         (SEC_HMC_CFG_RD_TO_WR_DIFF_BG),
      .SEC_HMC_CFG_RD_TO_PCH                (SEC_HMC_CFG_RD_TO_PCH),
      .SEC_HMC_CFG_RD_AP_TO_VALID           (SEC_HMC_CFG_RD_AP_TO_VALID),
      .SEC_HMC_CFG_WR_TO_WR                 (SEC_HMC_CFG_WR_TO_WR),
      .SEC_HMC_CFG_WR_TO_WR_DIFF_CHIP       (SEC_HMC_CFG_WR_TO_WR_DIFF_CHIP),
      .SEC_HMC_CFG_WR_TO_WR_DIFF_BG         (SEC_HMC_CFG_WR_TO_WR_DIFF_BG),
      .SEC_HMC_CFG_WR_TO_RD                 (SEC_HMC_CFG_WR_TO_RD),
      .SEC_HMC_CFG_WR_TO_RD_DIFF_CHIP       (SEC_HMC_CFG_WR_TO_RD_DIFF_CHIP),
      .SEC_HMC_CFG_WR_TO_RD_DIFF_BG         (SEC_HMC_CFG_WR_TO_RD_DIFF_BG),
      .SEC_HMC_CFG_WR_TO_PCH                (SEC_HMC_CFG_WR_TO_PCH),
      .SEC_HMC_CFG_WR_AP_TO_VALID           (SEC_HMC_CFG_WR_AP_TO_VALID),
      .SEC_HMC_CFG_PCH_TO_VALID             (SEC_HMC_CFG_PCH_TO_VALID),
      .SEC_HMC_CFG_PCH_ALL_TO_VALID         (SEC_HMC_CFG_PCH_ALL_TO_VALID),
      .SEC_HMC_CFG_ARF_TO_VALID             (SEC_HMC_CFG_ARF_TO_VALID),
      .SEC_HMC_CFG_PDN_TO_VALID             (SEC_HMC_CFG_PDN_TO_VALID),
      .SEC_HMC_CFG_SRF_TO_VALID             (SEC_HMC_CFG_SRF_TO_VALID),
      .SEC_HMC_CFG_SRF_TO_ZQ_CAL            (SEC_HMC_CFG_SRF_TO_ZQ_CAL),
      .SEC_HMC_CFG_ARF_PERIOD               (SEC_HMC_CFG_ARF_PERIOD),
      .SEC_HMC_CFG_PDN_PERIOD               (SEC_HMC_CFG_PDN_PERIOD),
      .SEC_HMC_CFG_ZQCL_TO_VALID            (SEC_HMC_CFG_ZQCL_TO_VALID),
      .SEC_HMC_CFG_ZQCS_TO_VALID            (SEC_HMC_CFG_ZQCS_TO_VALID),
      .SEC_HMC_CFG_MRS_TO_VALID             (SEC_HMC_CFG_MRS_TO_VALID),
      .SEC_HMC_CFG_MPS_TO_VALID             (SEC_HMC_CFG_MPS_TO_VALID),
      .SEC_HMC_CFG_MRR_TO_VALID             (SEC_HMC_CFG_MRR_TO_VALID),
      .SEC_HMC_CFG_MPR_TO_VALID             (SEC_HMC_CFG_MPR_TO_VALID),
      .SEC_HMC_CFG_MPS_EXIT_CS_TO_CKE       (SEC_HMC_CFG_MPS_EXIT_CS_TO_CKE),
      .SEC_HMC_CFG_MPS_EXIT_CKE_TO_CS       (SEC_HMC_CFG_MPS_EXIT_CKE_TO_CS),
      .SEC_HMC_CFG_RLD3_MULTIBANK_REF_DELAY (SEC_HMC_CFG_RLD3_MULTIBANK_REF_DELAY),
      .SEC_HMC_CFG_MMR_CMD_TO_VALID         (SEC_HMC_CFG_MMR_CMD_TO_VALID),
      .SEC_HMC_CFG_4_ACT_TO_ACT             (SEC_HMC_CFG_4_ACT_TO_ACT),
      .SEC_HMC_CFG_16_ACT_TO_ACT            (SEC_HMC_CFG_16_ACT_TO_ACT),
      .PINS_PER_LANE                        (PINS_PER_LANE),
      .LANES_PER_TILE                       (LANES_PER_TILE),
      .PINS_IN_RTL_TILES                    (PINS_IN_RTL_TILES),
      .LANES_IN_RTL_TILES                   (LANES_IN_RTL_TILES),
      .NUM_OF_RTL_TILES                     (NUM_OF_RTL_TILES),
      .AC_PIN_MAP_SCHEME                    (AC_PIN_MAP_SCHEME),
      .PRI_AC_TILE_INDEX                    (PRI_AC_TILE_INDEX),
      .SEC_AC_TILE_INDEX                    (SEC_AC_TILE_INDEX),
      .PRI_HMC_DBC_SHADOW_LANE_INDEX        (PRI_HMC_DBC_SHADOW_LANE_INDEX),
      .LANES_USAGE                          (LANES_USAGE),
      .PINS_USAGE                           (PINS_USAGE),
      .PINS_RATE                            (PINS_RATE),
      .PINS_WDB                             (PINS_WDB),
      .PINS_DB_IN_BYPASS                    (PINS_DB_IN_BYPASS),
      .PINS_DB_OUT_BYPASS                   (PINS_DB_OUT_BYPASS),
      .PINS_DB_OE_BYPASS                    (PINS_DB_OE_BYPASS),
      .PINS_INVERT_WR                       (PINS_INVERT_WR),
      .PINS_INVERT_OE                       (PINS_INVERT_OE),
      .PINS_AC_HMC_DATA_OVERRIDE_ENA        (PINS_AC_HMC_DATA_OVERRIDE_ENA),
      .PINS_DATA_IN_MODE                    (PINS_DATA_IN_MODE),
      .PINS_OCT_MODE                        (PINS_OCT_MODE),
      .PINS_GPIO_MODE                       (PINS_GPIO_MODE),
      .CENTER_TIDS                          (CENTER_TIDS),
      .HMC_TIDS                             (HMC_TIDS),
      .LANE_TIDS                            (LANE_TIDS),
      .PREAMBLE_MODE                        (PREAMBLE_MODE),
      .DBI_WR_ENABLE                        (DBI_WR_ENABLE),
      .DBI_RD_ENABLE                        (DBI_RD_ENABLE),
      .CRC_EN                               (CRC_EN),
      .SWAP_DQS_A_B                         (SWAP_DQS_A_B),
      .DQS_PACK_MODE                        (DQS_PACK_MODE),
      .OCT_SIZE                             (OCT_SIZE),
      .DBC_WB_RESERVED_ENTRY                (DBC_WB_RESERVED_ENTRY),
      .DLL_MODE                             (DLL_MODE),
      .DLL_CODEWORD                         (DLL_CODEWORD),
      .PORT_MEM_DQS_WIDTH                   (PORT_MEM_DQS_WIDTH),
      .PORT_MEM_DQ_WIDTH                    (PORT_MEM_DQ_WIDTH),
      .PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH  (PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH),
      .PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH (PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH),
      .PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH  (PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH)
   ) io_tiles_inst (
      .global_reset_n_int                      (global_reset_n_int_iotile_in),
      .pll_locked                              (pll_locked_iotile_in),
      .pll_dll_clk                             (pll_dll_clk_iotile_in),
      .phy_clk_phs                             (phy_clk_phs_iotile_in),
      .phy_clk                                 (phy_clk_iotile_in),
      .phy_fb_clk_to_tile                      (phy_fb_clk_to_tile_iotile_in),
      .core_clks_fb_to_cpa_pri                 (core_clks_fb_to_cpa_pri_iotile_in),
      .core_clks_fb_to_cpa_sec                 (core_clks_fb_to_cpa_sec_iotile_in),
      .core2ctl_avl_0                          (core2ctl_avl_0_iotile_in),
      .core2ctl_avl_1                          (core2ctl_avl_1_iotile_in),
      .core2ctl_avl_rd_data_ready_0            (core2ctl_avl_rd_data_ready_0_iotile_in),
      .core2ctl_avl_rd_data_ready_1            (core2ctl_avl_rd_data_ready_1_iotile_in),
      .core2l_wr_data_vld_ast_0                (core2l_wr_data_vld_ast_0_iotile_in),
      .core2l_wr_data_vld_ast_1                (core2l_wr_data_vld_ast_1_iotile_in),
      .core2l_rd_data_rdy_ast_0                (core2l_rd_data_rdy_ast_0_iotile_in),
      .core2l_rd_data_rdy_ast_1                (core2l_rd_data_rdy_ast_1_iotile_in),
      .core2l_wr_ecc_info_0                    (core2l_wr_ecc_info_0_iotile_in),
      .core2l_wr_ecc_info_1                    (core2l_wr_ecc_info_1_iotile_in),
      .core2l_data                             (core2l_data_iotile_in),
      .core2l_oe                               (core2l_oe_iotile_in),
      .core2l_rdata_en_full                    (core2l_rdata_en_full_iotile_in),
      .core2l_mrnk_read                        (core2l_mrnk_read_iotile_in),
      .core2l_mrnk_write                       (core2l_mrnk_write_iotile_in),
      .c2t_afi                                 (c2t_afi_iotile_in),
      .core2ctl_sideband_0                     (core2ctl_sideband_0_iotile_in),
      .core2ctl_sideband_1                     (core2ctl_sideband_1_iotile_in),
      .core2ctl_mmr_0                          (core2ctl_mmr_0_iotile_in),
      .core2ctl_mmr_1                          (core2ctl_mmr_1_iotile_in),
      .b2l_data                                (b2l_data_iotile_in),
      .b2t_dqs                                 (b2t_dqs_iotile_in),
      .b2t_dqsb                                (b2t_dqsb_iotile_in),
      .cal_bus_clk                             (cal_bus_clk_iotile_in),
      .cal_bus_avl_read                        (cal_bus_avl_read_iotile_in),
      .cal_bus_avl_write                       (cal_bus_avl_write_iotile_in),
      .cal_bus_avl_address                     (cal_bus_avl_address_iotile_in),
      .cal_bus_avl_write_data                  (cal_bus_avl_write_data_iotile_in),
      .pa_dprio_clk                            (pa_dprio_clk_iotile_in),
      .pa_dprio_read                           (pa_dprio_read_iotile_in),
      .pa_dprio_reg_addr                       (pa_dprio_reg_addr_iotile_in),
      .pa_dprio_rst_n                          (pa_dprio_rst_n_iotile_in),
      .pa_dprio_write                          (pa_dprio_write_iotile_in),
      .pa_dprio_writedata                      (pa_dprio_writedata_iotile_in),
      .*
   );

   generate
     if ( DIAG_USE_ABSTRACT_PHY==1 ) begin : abphy_tiles
       altera_emif_arch_nf_io_tiles_abphy # (
          .DIAG_SYNTH_FOR_SIM                   (DIAG_SYNTH_FOR_SIM),
          .DIAG_CPA_OUT_1_EN                    (DIAG_CPA_OUT_1_EN),
          .DIAG_FAST_SIM                        (DIAG_FAST_SIM),
          .IS_HPS                               (IS_HPS),
          .SILICON_REV                          (SILICON_REV),
          .PROTOCOL_ENUM                        (PROTOCOL_ENUM),
          .PHY_PING_PONG_EN                     (PHY_PING_PONG_EN),
          .DQS_BUS_MODE_ENUM                    (DQS_BUS_MODE_ENUM),
          .USER_CLK_RATIO                       (USER_CLK_RATIO),
          .PHY_HMC_CLK_RATIO                    (PHY_HMC_CLK_RATIO),
          .C2P_P2C_CLK_RATIO                    (C2P_P2C_CLK_RATIO),
          .PLL_VCO_FREQ_MHZ_INT                 (PLL_VCO_FREQ_MHZ_INT),
          .PLL_VCO_TO_MEM_CLK_FREQ_RATIO        (PLL_VCO_TO_MEM_CLK_FREQ_RATIO),
          .MEM_BURST_LENGTH                     (MEM_BURST_LENGTH),
          .MEM_DATA_MASK_EN                     (MEM_DATA_MASK_EN),
          .NUM_OF_HMC_PORTS                     (NUM_OF_HMC_PORTS),
          .HMC_AVL_PROTOCOL_ENUM                (HMC_AVL_PROTOCOL_ENUM),
          .HMC_CTRL_DIMM_TYPE                   (HMC_CTRL_DIMM_TYPE),
          .PRI_HMC_CFG_ENABLE_ECC               (PRI_HMC_CFG_ENABLE_ECC),
          .PRI_HMC_CFG_REORDER_DATA             (PRI_HMC_CFG_REORDER_DATA),
          .PRI_HMC_CFG_REORDER_READ             (PRI_HMC_CFG_REORDER_READ),
          .PRI_HMC_CFG_REORDER_RDATA            (PRI_HMC_CFG_REORDER_RDATA),
          .PRI_HMC_CFG_STARVE_LIMIT             (PRI_HMC_CFG_STARVE_LIMIT),
          .PRI_HMC_CFG_DQS_TRACKING_EN          (PRI_HMC_CFG_DQS_TRACKING_EN),
          .PRI_HMC_CFG_ARBITER_TYPE             (PRI_HMC_CFG_ARBITER_TYPE),
          .PRI_HMC_CFG_OPEN_PAGE_EN             (PRI_HMC_CFG_OPEN_PAGE_EN),
          .PRI_HMC_CFG_GEAR_DOWN_EN             (PRI_HMC_CFG_GEAR_DOWN_EN),
          .PRI_HMC_CFG_RLD3_MULTIBANK_MODE      (PRI_HMC_CFG_RLD3_MULTIBANK_MODE),
          .PRI_HMC_CFG_PING_PONG_MODE           (PRI_HMC_CFG_PING_PONG_MODE),
          .PRI_HMC_CFG_SLOT_ROTATE_EN           (PRI_HMC_CFG_SLOT_ROTATE_EN),
          .PRI_HMC_CFG_SLOT_OFFSET              (PRI_HMC_CFG_SLOT_OFFSET),
          .PRI_HMC_CFG_COL_CMD_SLOT             (PRI_HMC_CFG_COL_CMD_SLOT),
          .PRI_HMC_CFG_ROW_CMD_SLOT             (PRI_HMC_CFG_ROW_CMD_SLOT),
          .PRI_HMC_CFG_ENABLE_RC                (PRI_HMC_CFG_ENABLE_RC),
          .PRI_HMC_CFG_CS_TO_CHIP_MAPPING       (PRI_HMC_CFG_CS_TO_CHIP_MAPPING),
          .PRI_HMC_CFG_RB_RESERVED_ENTRY        (PRI_HMC_CFG_RB_RESERVED_ENTRY),
          .PRI_HMC_CFG_WB_RESERVED_ENTRY        (PRI_HMC_CFG_WB_RESERVED_ENTRY),
          .PRI_HMC_CFG_TCL                      (PRI_HMC_CFG_TCL),
          .PRI_HMC_CFG_POWER_SAVING_EXIT_CYC    (PRI_HMC_CFG_POWER_SAVING_EXIT_CYC),
          .PRI_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC(PRI_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC),
          .PRI_HMC_CFG_WRITE_ODT_CHIP           (PRI_HMC_CFG_WRITE_ODT_CHIP),
          .PRI_HMC_CFG_READ_ODT_CHIP            (PRI_HMC_CFG_READ_ODT_CHIP),
          .PRI_HMC_CFG_WR_ODT_ON                (PRI_HMC_CFG_WR_ODT_ON),
          .PRI_HMC_CFG_RD_ODT_ON                (PRI_HMC_CFG_RD_ODT_ON),
          .PRI_HMC_CFG_WR_ODT_PERIOD            (PRI_HMC_CFG_WR_ODT_PERIOD),
          .PRI_HMC_CFG_RD_ODT_PERIOD            (PRI_HMC_CFG_RD_ODT_PERIOD),
          .PRI_HMC_CFG_RLD3_REFRESH_SEQ0        (PRI_HMC_CFG_RLD3_REFRESH_SEQ0),
          .PRI_HMC_CFG_RLD3_REFRESH_SEQ1        (PRI_HMC_CFG_RLD3_REFRESH_SEQ1),
          .PRI_HMC_CFG_RLD3_REFRESH_SEQ2        (PRI_HMC_CFG_RLD3_REFRESH_SEQ2),
          .PRI_HMC_CFG_RLD3_REFRESH_SEQ3        (PRI_HMC_CFG_RLD3_REFRESH_SEQ3),
          .PRI_HMC_CFG_SRF_ZQCAL_DISABLE        (PRI_HMC_CFG_SRF_ZQCAL_DISABLE),
          .PRI_HMC_CFG_MPS_ZQCAL_DISABLE        (PRI_HMC_CFG_MPS_ZQCAL_DISABLE),
          .PRI_HMC_CFG_MPS_DQSTRK_DISABLE       (PRI_HMC_CFG_MPS_DQSTRK_DISABLE),
          .PRI_HMC_CFG_SHORT_DQSTRK_CTRL_EN     (PRI_HMC_CFG_SHORT_DQSTRK_CTRL_EN),
          .PRI_HMC_CFG_PERIOD_DQSTRK_CTRL_EN    (PRI_HMC_CFG_PERIOD_DQSTRK_CTRL_EN),
          .PRI_HMC_CFG_PERIOD_DQSTRK_INTERVAL   (PRI_HMC_CFG_PERIOD_DQSTRK_INTERVAL),
          .PRI_HMC_CFG_DQSTRK_TO_VALID_LAST     (PRI_HMC_CFG_DQSTRK_TO_VALID_LAST),
          .PRI_HMC_CFG_DQSTRK_TO_VALID          (PRI_HMC_CFG_DQSTRK_TO_VALID),
          .PRI_HMC_CFG_RFSH_WARN_THRESHOLD      (PRI_HMC_CFG_RFSH_WARN_THRESHOLD),
          .PRI_HMC_CFG_SB_CG_DISABLE            (PRI_HMC_CFG_SB_CG_DISABLE),
          .PRI_HMC_CFG_USER_RFSH_EN             (PRI_HMC_CFG_USER_RFSH_EN),
          .PRI_HMC_CFG_SRF_AUTOEXIT_EN          (PRI_HMC_CFG_SRF_AUTOEXIT_EN),
          .PRI_HMC_CFG_SRF_ENTRY_EXIT_BLOCK     (PRI_HMC_CFG_SRF_ENTRY_EXIT_BLOCK),
          .PRI_HMC_CFG_SB_DDR4_MR3              (PRI_HMC_CFG_SB_DDR4_MR3),
          .PRI_HMC_CFG_SB_DDR4_MR4              (PRI_HMC_CFG_SB_DDR4_MR4),
          .PRI_HMC_CFG_SB_DDR4_MR5              (PRI_HMC_CFG_SB_DDR4_MR5),
          .PRI_HMC_CFG_DDR4_MPS_ADDR_MIRROR     (PRI_HMC_CFG_DDR4_MPS_ADDR_MIRROR),
          .PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH     (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH),
          .PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH     (PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH),
          .PRI_HMC_CFG_MEM_IF_BANKADDR_WIDTH    (PRI_HMC_CFG_MEM_IF_BANKADDR_WIDTH),
          .PRI_HMC_CFG_MEM_IF_BGADDR_WIDTH      (PRI_HMC_CFG_MEM_IF_BGADDR_WIDTH),
          .PRI_HMC_CFG_LOCAL_IF_CS_WIDTH        (PRI_HMC_CFG_LOCAL_IF_CS_WIDTH),
          .PRI_HMC_CFG_ADDR_ORDER               (PRI_HMC_CFG_ADDR_ORDER),
          .PRI_HMC_CFG_ACT_TO_RDWR              (PRI_HMC_CFG_ACT_TO_RDWR),
          .PRI_HMC_CFG_ACT_TO_PCH               (PRI_HMC_CFG_ACT_TO_PCH),
          .PRI_HMC_CFG_ACT_TO_ACT               (PRI_HMC_CFG_ACT_TO_ACT),
          .PRI_HMC_CFG_ACT_TO_ACT_DIFF_BANK     (PRI_HMC_CFG_ACT_TO_ACT_DIFF_BANK),
          .PRI_HMC_CFG_ACT_TO_ACT_DIFF_BG       (PRI_HMC_CFG_ACT_TO_ACT_DIFF_BG),
          .PRI_HMC_CFG_RD_TO_RD                 (PRI_HMC_CFG_RD_TO_RD),
          .PRI_HMC_CFG_RD_TO_RD_DIFF_CHIP       (PRI_HMC_CFG_RD_TO_RD_DIFF_CHIP),
          .PRI_HMC_CFG_RD_TO_RD_DIFF_BG         (PRI_HMC_CFG_RD_TO_RD_DIFF_BG),
          .PRI_HMC_CFG_RD_TO_WR                 (PRI_HMC_CFG_RD_TO_WR),
          .PRI_HMC_CFG_RD_TO_WR_DIFF_CHIP       (PRI_HMC_CFG_RD_TO_WR_DIFF_CHIP),
          .PRI_HMC_CFG_RD_TO_WR_DIFF_BG         (PRI_HMC_CFG_RD_TO_WR_DIFF_BG),
          .PRI_HMC_CFG_RD_TO_PCH                (PRI_HMC_CFG_RD_TO_PCH),
          .PRI_HMC_CFG_RD_AP_TO_VALID           (PRI_HMC_CFG_RD_AP_TO_VALID),
          .PRI_HMC_CFG_WR_TO_WR                 (PRI_HMC_CFG_WR_TO_WR),
          .PRI_HMC_CFG_WR_TO_WR_DIFF_CHIP       (PRI_HMC_CFG_WR_TO_WR_DIFF_CHIP),
          .PRI_HMC_CFG_WR_TO_WR_DIFF_BG         (PRI_HMC_CFG_WR_TO_WR_DIFF_BG),
          .PRI_HMC_CFG_WR_TO_RD                 (PRI_HMC_CFG_WR_TO_RD),
          .PRI_HMC_CFG_WR_TO_RD_DIFF_CHIP       (PRI_HMC_CFG_WR_TO_RD_DIFF_CHIP),
          .PRI_HMC_CFG_WR_TO_RD_DIFF_BG         (PRI_HMC_CFG_WR_TO_RD_DIFF_BG),
          .PRI_HMC_CFG_WR_TO_PCH                (PRI_HMC_CFG_WR_TO_PCH),
          .PRI_HMC_CFG_WR_AP_TO_VALID           (PRI_HMC_CFG_WR_AP_TO_VALID),
          .PRI_HMC_CFG_PCH_TO_VALID             (PRI_HMC_CFG_PCH_TO_VALID),
          .PRI_HMC_CFG_PCH_ALL_TO_VALID         (PRI_HMC_CFG_PCH_ALL_TO_VALID),
          .PRI_HMC_CFG_ARF_TO_VALID             (PRI_HMC_CFG_ARF_TO_VALID),
          .PRI_HMC_CFG_PDN_TO_VALID             (PRI_HMC_CFG_PDN_TO_VALID),
          .PRI_HMC_CFG_SRF_TO_VALID             (PRI_HMC_CFG_SRF_TO_VALID),
          .PRI_HMC_CFG_SRF_TO_ZQ_CAL            (PRI_HMC_CFG_SRF_TO_ZQ_CAL),
          .PRI_HMC_CFG_ARF_PERIOD               (PRI_HMC_CFG_ARF_PERIOD),
          .PRI_HMC_CFG_PDN_PERIOD               (PRI_HMC_CFG_PDN_PERIOD),
          .PRI_HMC_CFG_ZQCL_TO_VALID            (PRI_HMC_CFG_ZQCL_TO_VALID),
          .PRI_HMC_CFG_ZQCS_TO_VALID            (PRI_HMC_CFG_ZQCS_TO_VALID),
          .PRI_HMC_CFG_MRS_TO_VALID             (PRI_HMC_CFG_MRS_TO_VALID),
          .PRI_HMC_CFG_MPS_TO_VALID             (PRI_HMC_CFG_MPS_TO_VALID),
          .PRI_HMC_CFG_MRR_TO_VALID             (PRI_HMC_CFG_MRR_TO_VALID),
          .PRI_HMC_CFG_MPR_TO_VALID             (PRI_HMC_CFG_MPR_TO_VALID),
          .PRI_HMC_CFG_MPS_EXIT_CS_TO_CKE       (PRI_HMC_CFG_MPS_EXIT_CS_TO_CKE),
          .PRI_HMC_CFG_MPS_EXIT_CKE_TO_CS       (PRI_HMC_CFG_MPS_EXIT_CKE_TO_CS),
          .PRI_HMC_CFG_RLD3_MULTIBANK_REF_DELAY (PRI_HMC_CFG_RLD3_MULTIBANK_REF_DELAY),
          .PRI_HMC_CFG_MMR_CMD_TO_VALID         (PRI_HMC_CFG_MMR_CMD_TO_VALID),
          .PRI_HMC_CFG_4_ACT_TO_ACT             (PRI_HMC_CFG_4_ACT_TO_ACT),
          .PRI_HMC_CFG_16_ACT_TO_ACT            (PRI_HMC_CFG_16_ACT_TO_ACT),

          .SEC_HMC_CFG_ENABLE_ECC               (SEC_HMC_CFG_ENABLE_ECC),
          .SEC_HMC_CFG_REORDER_DATA             (SEC_HMC_CFG_REORDER_DATA),
          .SEC_HMC_CFG_REORDER_READ             (SEC_HMC_CFG_REORDER_READ),
          .SEC_HMC_CFG_REORDER_RDATA            (SEC_HMC_CFG_REORDER_RDATA),
          .SEC_HMC_CFG_STARVE_LIMIT             (SEC_HMC_CFG_STARVE_LIMIT),
          .SEC_HMC_CFG_DQS_TRACKING_EN          (SEC_HMC_CFG_DQS_TRACKING_EN),
          .SEC_HMC_CFG_ARBITER_TYPE             (SEC_HMC_CFG_ARBITER_TYPE),
          .SEC_HMC_CFG_OPEN_PAGE_EN             (SEC_HMC_CFG_OPEN_PAGE_EN),
          .SEC_HMC_CFG_GEAR_DOWN_EN             (SEC_HMC_CFG_GEAR_DOWN_EN),
          .SEC_HMC_CFG_RLD3_MULTIBANK_MODE      (SEC_HMC_CFG_RLD3_MULTIBANK_MODE),
          .SEC_HMC_CFG_PING_PONG_MODE           (SEC_HMC_CFG_PING_PONG_MODE),
          .SEC_HMC_CFG_SLOT_ROTATE_EN           (SEC_HMC_CFG_SLOT_ROTATE_EN),
          .SEC_HMC_CFG_SLOT_OFFSET              (SEC_HMC_CFG_SLOT_OFFSET),
          .SEC_HMC_CFG_COL_CMD_SLOT             (SEC_HMC_CFG_COL_CMD_SLOT),
          .SEC_HMC_CFG_ROW_CMD_SLOT             (SEC_HMC_CFG_ROW_CMD_SLOT),
          .SEC_HMC_CFG_ENABLE_RC                (SEC_HMC_CFG_ENABLE_RC),
          .SEC_HMC_CFG_CS_TO_CHIP_MAPPING       (SEC_HMC_CFG_CS_TO_CHIP_MAPPING),
          .SEC_HMC_CFG_RB_RESERVED_ENTRY        (SEC_HMC_CFG_RB_RESERVED_ENTRY),
          .SEC_HMC_CFG_WB_RESERVED_ENTRY        (SEC_HMC_CFG_WB_RESERVED_ENTRY),
          .SEC_HMC_CFG_TCL                      (SEC_HMC_CFG_TCL),
          .SEC_HMC_CFG_POWER_SAVING_EXIT_CYC    (SEC_HMC_CFG_POWER_SAVING_EXIT_CYC),
          .SEC_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC(SEC_HMC_CFG_MEM_CLK_DISABLE_ENTRY_CYC),
          .SEC_HMC_CFG_WRITE_ODT_CHIP           (SEC_HMC_CFG_WRITE_ODT_CHIP),
          .SEC_HMC_CFG_READ_ODT_CHIP            (SEC_HMC_CFG_READ_ODT_CHIP),
          .SEC_HMC_CFG_WR_ODT_ON                (SEC_HMC_CFG_WR_ODT_ON),
          .SEC_HMC_CFG_RD_ODT_ON                (SEC_HMC_CFG_RD_ODT_ON),
          .SEC_HMC_CFG_WR_ODT_PERIOD            (SEC_HMC_CFG_WR_ODT_PERIOD),
          .SEC_HMC_CFG_RD_ODT_PERIOD            (SEC_HMC_CFG_RD_ODT_PERIOD),
          .SEC_HMC_CFG_RLD3_REFRESH_SEQ0        (SEC_HMC_CFG_RLD3_REFRESH_SEQ0),
          .SEC_HMC_CFG_RLD3_REFRESH_SEQ1        (SEC_HMC_CFG_RLD3_REFRESH_SEQ1),
          .SEC_HMC_CFG_RLD3_REFRESH_SEQ2        (SEC_HMC_CFG_RLD3_REFRESH_SEQ2),
          .SEC_HMC_CFG_RLD3_REFRESH_SEQ3        (SEC_HMC_CFG_RLD3_REFRESH_SEQ3),
          .SEC_HMC_CFG_SRF_ZQCAL_DISABLE        (SEC_HMC_CFG_SRF_ZQCAL_DISABLE),
          .SEC_HMC_CFG_MPS_ZQCAL_DISABLE        (SEC_HMC_CFG_MPS_ZQCAL_DISABLE),
          .SEC_HMC_CFG_MPS_DQSTRK_DISABLE       (SEC_HMC_CFG_MPS_DQSTRK_DISABLE),
          .SEC_HMC_CFG_SHORT_DQSTRK_CTRL_EN     (SEC_HMC_CFG_SHORT_DQSTRK_CTRL_EN),
          .SEC_HMC_CFG_PERIOD_DQSTRK_CTRL_EN    (SEC_HMC_CFG_PERIOD_DQSTRK_CTRL_EN),
          .SEC_HMC_CFG_PERIOD_DQSTRK_INTERVAL   (SEC_HMC_CFG_PERIOD_DQSTRK_INTERVAL),
          .SEC_HMC_CFG_DQSTRK_TO_VALID_LAST     (SEC_HMC_CFG_DQSTRK_TO_VALID_LAST),
          .SEC_HMC_CFG_DQSTRK_TO_VALID          (SEC_HMC_CFG_DQSTRK_TO_VALID),
          .SEC_HMC_CFG_RFSH_WARN_THRESHOLD      (SEC_HMC_CFG_RFSH_WARN_THRESHOLD),
          .SEC_HMC_CFG_SB_CG_DISABLE            (SEC_HMC_CFG_SB_CG_DISABLE),
          .SEC_HMC_CFG_USER_RFSH_EN             (SEC_HMC_CFG_USER_RFSH_EN),
          .SEC_HMC_CFG_SRF_AUTOEXIT_EN          (SEC_HMC_CFG_SRF_AUTOEXIT_EN),
          .SEC_HMC_CFG_SRF_ENTRY_EXIT_BLOCK     (SEC_HMC_CFG_SRF_ENTRY_EXIT_BLOCK),
          .SEC_HMC_CFG_SB_DDR4_MR3              (SEC_HMC_CFG_SB_DDR4_MR3),
          .SEC_HMC_CFG_SB_DDR4_MR4              (SEC_HMC_CFG_SB_DDR4_MR4),
          .SEC_HMC_CFG_SB_DDR4_MR5              (SEC_HMC_CFG_SB_DDR4_MR5),
          .SEC_HMC_CFG_DDR4_MPS_ADDR_MIRROR     (SEC_HMC_CFG_DDR4_MPS_ADDR_MIRROR),
          .SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH     (SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH),
          .SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH     (SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH),
          .SEC_HMC_CFG_MEM_IF_BANKADDR_WIDTH    (SEC_HMC_CFG_MEM_IF_BANKADDR_WIDTH),
          .SEC_HMC_CFG_MEM_IF_BGADDR_WIDTH      (SEC_HMC_CFG_MEM_IF_BGADDR_WIDTH),
          .SEC_HMC_CFG_LOCAL_IF_CS_WIDTH        (SEC_HMC_CFG_LOCAL_IF_CS_WIDTH),
          .SEC_HMC_CFG_ADDR_ORDER               (SEC_HMC_CFG_ADDR_ORDER),
          .SEC_HMC_CFG_ACT_TO_RDWR              (SEC_HMC_CFG_ACT_TO_RDWR),
          .SEC_HMC_CFG_ACT_TO_PCH               (SEC_HMC_CFG_ACT_TO_PCH),
          .SEC_HMC_CFG_ACT_TO_ACT               (SEC_HMC_CFG_ACT_TO_ACT),
          .SEC_HMC_CFG_ACT_TO_ACT_DIFF_BANK     (SEC_HMC_CFG_ACT_TO_ACT_DIFF_BANK),
          .SEC_HMC_CFG_ACT_TO_ACT_DIFF_BG       (SEC_HMC_CFG_ACT_TO_ACT_DIFF_BG),
          .SEC_HMC_CFG_RD_TO_RD                 (SEC_HMC_CFG_RD_TO_RD),
          .SEC_HMC_CFG_RD_TO_RD_DIFF_CHIP       (SEC_HMC_CFG_RD_TO_RD_DIFF_CHIP),
          .SEC_HMC_CFG_RD_TO_RD_DIFF_BG         (SEC_HMC_CFG_RD_TO_RD_DIFF_BG),
          .SEC_HMC_CFG_RD_TO_WR                 (SEC_HMC_CFG_RD_TO_WR),
          .SEC_HMC_CFG_RD_TO_WR_DIFF_CHIP       (SEC_HMC_CFG_RD_TO_WR_DIFF_CHIP),
          .SEC_HMC_CFG_RD_TO_WR_DIFF_BG         (SEC_HMC_CFG_RD_TO_WR_DIFF_BG),
          .SEC_HMC_CFG_RD_TO_PCH                (SEC_HMC_CFG_RD_TO_PCH),
          .SEC_HMC_CFG_RD_AP_TO_VALID           (SEC_HMC_CFG_RD_AP_TO_VALID),
          .SEC_HMC_CFG_WR_TO_WR                 (SEC_HMC_CFG_WR_TO_WR),
          .SEC_HMC_CFG_WR_TO_WR_DIFF_CHIP       (SEC_HMC_CFG_WR_TO_WR_DIFF_CHIP),
          .SEC_HMC_CFG_WR_TO_WR_DIFF_BG         (SEC_HMC_CFG_WR_TO_WR_DIFF_BG),
          .SEC_HMC_CFG_WR_TO_RD                 (SEC_HMC_CFG_WR_TO_RD),
          .SEC_HMC_CFG_WR_TO_RD_DIFF_CHIP       (SEC_HMC_CFG_WR_TO_RD_DIFF_CHIP),
          .SEC_HMC_CFG_WR_TO_RD_DIFF_BG         (SEC_HMC_CFG_WR_TO_RD_DIFF_BG),
          .SEC_HMC_CFG_WR_TO_PCH                (SEC_HMC_CFG_WR_TO_PCH),
          .SEC_HMC_CFG_WR_AP_TO_VALID           (SEC_HMC_CFG_WR_AP_TO_VALID),
          .SEC_HMC_CFG_PCH_TO_VALID             (SEC_HMC_CFG_PCH_TO_VALID),
          .SEC_HMC_CFG_PCH_ALL_TO_VALID         (SEC_HMC_CFG_PCH_ALL_TO_VALID),
          .SEC_HMC_CFG_ARF_TO_VALID             (SEC_HMC_CFG_ARF_TO_VALID),
          .SEC_HMC_CFG_PDN_TO_VALID             (SEC_HMC_CFG_PDN_TO_VALID),
          .SEC_HMC_CFG_SRF_TO_VALID             (SEC_HMC_CFG_SRF_TO_VALID),
          .SEC_HMC_CFG_SRF_TO_ZQ_CAL            (SEC_HMC_CFG_SRF_TO_ZQ_CAL),
          .SEC_HMC_CFG_ARF_PERIOD               (SEC_HMC_CFG_ARF_PERIOD),
          .SEC_HMC_CFG_PDN_PERIOD               (SEC_HMC_CFG_PDN_PERIOD),
          .SEC_HMC_CFG_ZQCL_TO_VALID            (SEC_HMC_CFG_ZQCL_TO_VALID),
          .SEC_HMC_CFG_ZQCS_TO_VALID            (SEC_HMC_CFG_ZQCS_TO_VALID),
          .SEC_HMC_CFG_MRS_TO_VALID             (SEC_HMC_CFG_MRS_TO_VALID),
          .SEC_HMC_CFG_MPS_TO_VALID             (SEC_HMC_CFG_MPS_TO_VALID),
          .SEC_HMC_CFG_MRR_TO_VALID             (SEC_HMC_CFG_MRR_TO_VALID),
          .SEC_HMC_CFG_MPR_TO_VALID             (SEC_HMC_CFG_MPR_TO_VALID),
          .SEC_HMC_CFG_MPS_EXIT_CS_TO_CKE       (SEC_HMC_CFG_MPS_EXIT_CS_TO_CKE),
          .SEC_HMC_CFG_MPS_EXIT_CKE_TO_CS       (SEC_HMC_CFG_MPS_EXIT_CKE_TO_CS),
          .SEC_HMC_CFG_RLD3_MULTIBANK_REF_DELAY (SEC_HMC_CFG_RLD3_MULTIBANK_REF_DELAY),
          .SEC_HMC_CFG_MMR_CMD_TO_VALID         (SEC_HMC_CFG_MMR_CMD_TO_VALID),
          .SEC_HMC_CFG_4_ACT_TO_ACT             (SEC_HMC_CFG_4_ACT_TO_ACT),
          .SEC_HMC_CFG_16_ACT_TO_ACT            (SEC_HMC_CFG_16_ACT_TO_ACT),
          .PINS_PER_LANE                        (PINS_PER_LANE),
          .LANES_PER_TILE                       (LANES_PER_TILE),
          .PINS_IN_RTL_TILES                    (PINS_IN_RTL_TILES),
          .LANES_IN_RTL_TILES                   (LANES_IN_RTL_TILES),
          .NUM_OF_RTL_TILES                     (NUM_OF_RTL_TILES),
          .AC_PIN_MAP_SCHEME                    (AC_PIN_MAP_SCHEME),
          .PRI_AC_TILE_INDEX                    (PRI_AC_TILE_INDEX),
          .SEC_AC_TILE_INDEX                    (SEC_AC_TILE_INDEX),
          .PRI_HMC_DBC_SHADOW_LANE_INDEX        (PRI_HMC_DBC_SHADOW_LANE_INDEX),
          .LANES_USAGE                          (LANES_USAGE),
          .PINS_USAGE                           (PINS_USAGE),
          .PINS_RATE                            (PINS_RATE),
          .PINS_WDB                             (PINS_WDB),
          .PINS_DB_IN_BYPASS                    (PINS_DB_IN_BYPASS),
          .PINS_DB_OUT_BYPASS                   (PINS_DB_OUT_BYPASS),
          .PINS_DB_OE_BYPASS                    (PINS_DB_OE_BYPASS),
          .PINS_INVERT_WR                       (PINS_INVERT_WR),
          .PINS_INVERT_OE                       (PINS_INVERT_OE),
          .PINS_AC_HMC_DATA_OVERRIDE_ENA        (PINS_AC_HMC_DATA_OVERRIDE_ENA),
          .PINS_DATA_IN_MODE                    (PINS_DATA_IN_MODE),
          .PINS_OCT_MODE                        (PINS_OCT_MODE),
          .PINS_GPIO_MODE                       (PINS_GPIO_MODE),
          .CENTER_TIDS                          (CENTER_TIDS),
          .HMC_TIDS                             (HMC_TIDS),
          .LANE_TIDS                            (LANE_TIDS),
          .PREAMBLE_MODE                        (PREAMBLE_MODE),
          .DBI_WR_ENABLE                        (DBI_WR_ENABLE),
          .DBI_RD_ENABLE                        (DBI_RD_ENABLE),
          .CRC_EN                               (CRC_EN),
          .SWAP_DQS_A_B                         (SWAP_DQS_A_B),
          .DQS_PACK_MODE                        (DQS_PACK_MODE),
          .OCT_SIZE                             (OCT_SIZE),
          .DBC_WB_RESERVED_ENTRY                (DBC_WB_RESERVED_ENTRY),
          .DLL_MODE                             (DLL_MODE),
          .DLL_CODEWORD                         (DLL_CODEWORD),
          .PORT_MEM_DQS_WIDTH                   (PORT_MEM_DQS_WIDTH),
          .PORT_MEM_DQ_WIDTH                    (PORT_MEM_DQ_WIDTH),
          .PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH  (PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH),
          .PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH (PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH),
          .PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH  (PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH),
          .PORT_MEM_A_PINLOC                    (PORT_MEM_A_PINLOC),
          .PORT_MEM_BA_PINLOC                   (PORT_MEM_BA_PINLOC),
          .PORT_MEM_BG_PINLOC                   (PORT_MEM_BG_PINLOC),
          .PORT_MEM_CS_N_PINLOC                 (PORT_MEM_CS_N_PINLOC),
          .PORT_MEM_ACT_N_PINLOC                (PORT_MEM_ACT_N_PINLOC),
          .PORT_MEM_DQ_PINLOC                   (PORT_MEM_DQ_PINLOC),
          .PORT_MEM_DM_PINLOC                   (PORT_MEM_DM_PINLOC),
          .PORT_MEM_DBI_N_PINLOC                (PORT_MEM_DBI_N_PINLOC),
          .PORT_MEM_RAS_N_PINLOC                (PORT_MEM_RAS_N_PINLOC),
          .PORT_MEM_CAS_N_PINLOC                (PORT_MEM_CAS_N_PINLOC),
          .PORT_MEM_WE_N_PINLOC                 (PORT_MEM_WE_N_PINLOC),
          .PORT_MEM_REF_N_PINLOC                (PORT_MEM_REF_N_PINLOC),
          .PORT_MEM_WPS_N_PINLOC                (PORT_MEM_WPS_N_PINLOC),
          .PORT_MEM_RPS_N_PINLOC                (PORT_MEM_RPS_N_PINLOC),
          .PORT_MEM_BWS_N_PINLOC                (PORT_MEM_BWS_N_PINLOC),
          .PORT_MEM_DQA_PINLOC                  (PORT_MEM_DQA_PINLOC),
          .PORT_MEM_DQB_PINLOC                  (PORT_MEM_DQB_PINLOC),
          .PORT_MEM_Q_PINLOC                    (PORT_MEM_Q_PINLOC),
          .PORT_MEM_D_PINLOC                    (PORT_MEM_D_PINLOC),
          .PORT_MEM_RWA_N_PINLOC                (PORT_MEM_RWA_N_PINLOC),
          .PORT_MEM_RWB_N_PINLOC                (PORT_MEM_RWB_N_PINLOC),
          .PORT_MEM_QKA_PINLOC                  (PORT_MEM_QKA_PINLOC),
          .PORT_MEM_QKB_PINLOC                  (PORT_MEM_QKB_PINLOC),
          .PORT_MEM_LDA_N_PINLOC                (PORT_MEM_LDA_N_PINLOC),
          .PORT_MEM_LDB_N_PINLOC                (PORT_MEM_LDB_N_PINLOC),
          .PORT_MEM_CK_PINLOC                   (PORT_MEM_CK_PINLOC),
          .PORT_MEM_DINVA_PINLOC                (PORT_MEM_DINVA_PINLOC),
          .PORT_MEM_DINVB_PINLOC                (PORT_MEM_DINVB_PINLOC),
          .PORT_MEM_AINV_PINLOC                 (PORT_MEM_AINV_PINLOC),
          .PORT_MEM_DM_WIDTH                    (PORT_MEM_DM_WIDTH),
          .PORT_MEM_A_WIDTH                     (PORT_MEM_A_WIDTH),
          .PORT_MEM_BA_WIDTH                    (PORT_MEM_BA_WIDTH),
          .PORT_MEM_BG_WIDTH                    (PORT_MEM_BG_WIDTH),
          .PORT_MEM_CS_N_WIDTH                  (PORT_MEM_CS_N_WIDTH),
          .PORT_MEM_ACT_N_WIDTH                 (PORT_MEM_ACT_N_WIDTH),
          .PORT_MEM_DBI_N_WIDTH                 (PORT_MEM_DBI_N_WIDTH),
          .PORT_MEM_RAS_N_WIDTH                 (PORT_MEM_RAS_N_WIDTH),
          .PORT_MEM_CAS_N_WIDTH                 (PORT_MEM_CAS_N_WIDTH),
          .PORT_MEM_WE_N_WIDTH                  (PORT_MEM_WE_N_WIDTH),
          .PORT_MEM_REF_N_WIDTH                 (PORT_MEM_REF_N_WIDTH),
          .PORT_MEM_WPS_N_WIDTH                 (PORT_MEM_WPS_N_WIDTH),
          .PORT_MEM_RPS_N_WIDTH                 (PORT_MEM_RPS_N_WIDTH),
          .PORT_MEM_BWS_N_WIDTH                 (PORT_MEM_BWS_N_WIDTH),
          .PORT_MEM_DQA_WIDTH                   (PORT_MEM_DQA_WIDTH),
          .PORT_MEM_DQB_WIDTH                   (PORT_MEM_DQB_WIDTH),
          .PORT_MEM_Q_WIDTH                     (PORT_MEM_Q_WIDTH),
          .PORT_MEM_D_WIDTH                     (PORT_MEM_D_WIDTH),
          .PORT_MEM_RWA_N_WIDTH                 (PORT_MEM_RWA_N_WIDTH),
          .PORT_MEM_RWB_N_WIDTH                 (PORT_MEM_RWB_N_WIDTH),
          .PORT_MEM_QKA_WIDTH                   (PORT_MEM_QKA_WIDTH),
          .PORT_MEM_QKB_WIDTH                   (PORT_MEM_QKB_WIDTH),
          .PORT_MEM_LDA_N_WIDTH                 (PORT_MEM_LDA_N_WIDTH),
          .PORT_MEM_LDB_N_WIDTH                 (PORT_MEM_LDB_N_WIDTH),
          .PORT_MEM_CK_WIDTH                    (PORT_MEM_CK_WIDTH),
          .PORT_MEM_DINVA_WIDTH                 (PORT_MEM_DINVA_WIDTH),
          .PORT_MEM_DINVB_WIDTH                 (PORT_MEM_DINVB_WIDTH),
          .PORT_MEM_AINV_WIDTH                  (PORT_MEM_AINV_WIDTH),
          .DIAG_USE_ABSTRACT_PHY                (DIAG_USE_ABSTRACT_PHY),
          .DIAG_ABSTRACT_PHY_WLAT               (DIAG_ABSTRACT_PHY_WLAT),
          .DIAG_ABSTRACT_PHY_RLAT               (DIAG_ABSTRACT_PHY_RLAT),
          .ABPHY_WRITE_PROTOCOL                 (ABPHY_WRITE_PROTOCOL)
       ) io_tiles_abphy_inst (
          .*
       );
     end
     else begin : nonabphy_setoutputs
         assign phy_reset_n_abphy = 'd0;
         assign phy_fb_clk_to_pll_abphy = 'd0;
         assign core_clks_from_cpa_pri_abphy = 'd0;
         assign core_clks_locked_cpa_pri_abphy = 'd0;
         assign core_clks_from_cpa_sec_abphy = 'd0;
         assign core_clks_locked_cpa_sec_abphy = 'd0;
         assign ctl2core_avl_cmd_ready_0_abphy = 'd0;
         assign ctl2core_avl_cmd_ready_1_abphy = 'd0;
         assign ctl2core_avl_rdata_id_0_abphy = 'd0;
         assign ctl2core_avl_rdata_id_1_abphy = 'd0;
         assign l2core_rd_data_vld_avl0_abphy = 'd0;
         assign l2core_wr_data_rdy_ast_abphy = 'd0;
         assign l2core_wb_pointer_for_ecc_abphy = 'd0;
         assign l2core_data_abphy = 'd0;
         assign l2core_rdata_valid_abphy = 'd0;
         assign l2core_afi_rlat_abphy = 'd0;
         assign l2core_afi_wlat_abphy = 'd0;
         assign t2c_afi_abphy = 'd0;
         assign ctl2core_sideband_0_abphy = 'd0;
         assign ctl2core_sideband_1_abphy = 'd0;
         assign ctl2core_mmr_0_abphy = 'd0;
         assign ctl2core_mmr_1_abphy = 'd0;
         assign l2b_data_abphy = 'd0;
         assign l2b_oe_abphy = 'd0;
         assign l2b_dtc_abphy = 'd0;
         assign pa_dprio_block_select_abphy = 'd0;
         assign pa_dprio_readdata_abphy = 'd0;
         assign runAbstractPhySim = 'd0;
     end
   endgenerate

   altera_emif_arch_nf_abphy_mux #(
      .DIAG_USE_ABSTRACT_PHY                (DIAG_USE_ABSTRACT_PHY),
      .PINS_PER_LANE                        (PINS_PER_LANE),
      .LANES_PER_TILE                       (LANES_PER_TILE),
      .PINS_IN_RTL_TILES                    (PINS_IN_RTL_TILES),
      .NUM_OF_RTL_TILES                     (NUM_OF_RTL_TILES),
      .PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH  (PORT_DFT_NF_PA_DPRIO_READDATA_WIDTH),
      .PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH  (PORT_DFT_NF_PA_DPRIO_REG_ADDR_WIDTH),
      .PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH (PORT_DFT_NF_PA_DPRIO_WRITEDATA_WIDTH),
      .LANES_IN_RTL_TILES                   (LANES_IN_RTL_TILES)
   ) altera_emif_arch_nf_abphy_mux_inst (
      .*
   );

   integer i, j;

endmodule
