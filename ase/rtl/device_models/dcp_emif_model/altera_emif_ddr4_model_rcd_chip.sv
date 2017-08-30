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
// Basic simulation model of DDR4 Registering Clock Driver used by RDIMM and LRDIMM
//
///////////////////////////////////////////////////////////////////////////////
module altera_emif_ddr4_model_rcd_chip # (
   parameter ADDRESS_MIRRORING   = 1,
   parameter PORT_MEM_CS_N_WIDTH = 1
) (
   input             [1:0]   DCKE,
   input             [1:0]   DODT,
   input             [3:0]   DCS_n,
   input             [2:0]   DC,

   input            [17:0]   DA,
   input             [1:0]   DBA,
   input             [1:0]   DBG,
   input                     DACT_n,

   input                     CK_t,
   input                     CK_c,

   input                     DRST_n,

   input                     DPAR,

   input                     ERROR_IN_n,

   output                    BODT,
   output                    BCKE,
   output            [3:0]   BCOM,
   output                    BCK_t,
   output                    BCK_c,
   output                    BVrefCA,

   output   logic    [1:0]   QACKE,
   output   logic    [1:0]   QBCKE,
   output   logic    [1:0]   QAODT,
   output   logic    [1:0]   QBODT,
   output   logic    [3:0]   QACS_n,
   output   logic    [3:0]   QBCS_n,
   output   logic    [1:0]   QAC,
   output   logic    [1:0]   QBC,

   output   logic   [17:0]   QAA,
   output   logic   [17:0]   QBA,
   output   logic    [1:0]   QABA,
   output   logic    [1:0]   QABG,
   output   logic    [1:0]   QBBA,
   output   logic    [1:0]   QBBG,
   output   logic            QAACT_n,
   output   logic            QBACT_n,

   output            [3:0]   Y_c,
   output            [3:0]   Y_t,

   output   logic            QRST_n,

   output   logic            QAPAR,
   output   logic            QBPAR,

   output   logic            ALERT_n,

   inout                     SDA,
   input             [2:0]   SA,
   input                     SCL,
   input                     BFUNC,
   input                     VDDSPD,

   input                     VDD,
   input                     VSS,
   input                     AVDD,
   input                     PVDD,
   input                     PVSS
);

   timeunit 1ps;
   timeprecision 1ps;

   typedef enum
   {
      RC00,
      RC01,
      RC02,
      RC03,
      RC04,
      RC05,
      RC06,
      RC07,
      RC08,
      RC09,
      RC0A,
      RC0B,
      RC0C,
      RC0D,
      RC0E,
      RC0F,
      RC1X,
      RC2X,
      RC3X,
      RC4X,
      RC5X,
      RC6X,
      RC7X,
      RC8X,
      RC9X,
      RCAX,
      RCBX,
      RC_NONE
   } rcd_enum_t;

   typedef enum
   {
      OUTPUT_INVERSION_ENABLED,
      OUTPUT_INVERSION_DISABLED
   } rcd_oinv_t;

   typedef enum
   {
      WEAK_DRIVE_DISABLED,
      WEAK_DRIVE_ENABLED
   } rcd_wdrv_t;

   typedef enum
   {
      OUTPUT_ENABLED,
      OUTPUT_DISABLED
   } rcd_oe_t;

   typedef enum
   {
      ENABLED,
      DISABLED
   } rcd_en_t;

   typedef enum
   {
      FREQUENCY_BAND_OPERATION,
      FREQUENCY_BAND_TEST_MODE
   } rcd_freqband_t;

   typedef enum
   {
      DRIVE_LIGHT,
      DRIVE_MODERATE,
      DRIVE_STRONG,
      DRIVE_VERYSTRONG
   } rcd_drive_t;

   typedef enum
   {
      CMD_SOFT_RESET,
      CMD_DB_RESET,
      CMD_SET_DRAM_RESET,
      CMD_CLR_DRAM_RESET,
      CMD_CW_READ_OP,
      CMD_CW_WRITE_OP,
      CMD_CLR_PARITY_ERROR,
      CMD_SOFT_RCD_RESET,
      CMD_NOP
   } rcd_cmd_cw_t;

   typedef enum
   {
      QXC_C_EN_111,
      QXC_C_EN_011,
      QXC_C_EN_001,
      QXC_C_EN_000
   } rcd_qxc_t;

   typedef enum
   {
      CKE_POWERDOWN_IBT_ON,
      CKE_POWERDOWN_IBT_OFF
   } rcd_cke_pd_mode_t;

   typedef enum
   {
      RDIMM_SPEED_UP_TO_1600,
      RDIMM_SPEED_1601_TO_1867,
      RDIMM_SPEED_1868_TO_2134,
      RDIMM_SPEED_2135_TO_2400,
      RDIMM_SPEED_2401_TO_2667,
      RDIMM_SPEED_2668_TO_3200,
      RDIMM_SPEED_RESERVED,
      RDIMM_SPEED_PLL_BYPASS
   } rcd_rdimm_speed_t;

   typedef enum
   {
      RDIMM_SPEED_FINE_1241_TO_1260,
      RDIMM_SPEED_FINE_1261_TO_1280,
      RDIMM_SPEED_FINE_1281_TO_1300,
      RDIMM_SPEED_FINE_1301_TO_1320,
      RDIMM_SPEED_FINE_1321_TO_1340,
      RDIMM_SPEED_FINE_1341_TO_1360,
      RDIMM_SPEED_FINE_1361_TO_1380,
      RDIMM_SPEED_FINE_1381_TO_1400,
      RDIMM_SPEED_FINE_1401_TO_1420,
      RDIMM_SPEED_FINE_1421_TO_1440,
      RDIMM_SPEED_FINE_1441_TO_1460,
      RDIMM_SPEED_FINE_1461_TO_1480,
      RDIMM_SPEED_FINE_1481_TO_1500,
      RDIMM_SPEED_FINE_1501_TO_1520,
      RDIMM_SPEED_FINE_1521_TO_1540,
      RDIMM_SPEED_FINE_1541_TO_1560,
      RDIMM_SPEED_FINE_1561_TO_1580,
      RDIMM_SPEED_FINE_1581_TO_1600,
      RDIMM_SPEED_FINE_1601_TO_1620,
      RDIMM_SPEED_FINE_1621_TO_1640,
      RDIMM_SPEED_FINE_1641_TO_1660,
      RDIMM_SPEED_FINE_1661_TO_1680,
      RDIMM_SPEED_FINE_1681_TO_1700,
      RDIMM_SPEED_FINE_1701_TO_1720,
      RDIMM_SPEED_FINE_1721_TO_1740,
      RDIMM_SPEED_FINE_1741_TO_1760,
      RDIMM_SPEED_FINE_1761_TO_1780,
      RDIMM_SPEED_FINE_1781_TO_1800,
      RDIMM_SPEED_FINE_1801_TO_1820,
      RDIMM_SPEED_FINE_1821_TO_1840,
      RDIMM_SPEED_FINE_1841_TO_1860,
      RDIMM_SPEED_FINE_1861_TO_1880,
      RDIMM_SPEED_FINE_1881_TO_1900,
      RDIMM_SPEED_FINE_1901_TO_1920,
      RDIMM_SPEED_FINE_1921_TO_1940,
      RDIMM_SPEED_FINE_1941_TO_1960,
      RDIMM_SPEED_FINE_1961_TO_1980,
      RDIMM_SPEED_FINE_1981_TO_2000,
      RDIMM_SPEED_FINE_2001_TO_2020,
      RDIMM_SPEED_FINE_2021_TO_2040,
      RDIMM_SPEED_FINE_2041_TO_2060,
      RDIMM_SPEED_FINE_2061_TO_2080,
      RDIMM_SPEED_FINE_2081_TO_2100,
      RDIMM_SPEED_FINE_2101_TO_2120,
      RDIMM_SPEED_FINE_2121_TO_2140,
      RDIMM_SPEED_FINE_2141_TO_2160,
      RDIMM_SPEED_FINE_2161_TO_2180,
      RDIMM_SPEED_FINE_2181_TO_2200,
      RDIMM_SPEED_FINE_2201_TO_2220,
      RDIMM_SPEED_FINE_2221_TO_2240,
      RDIMM_SPEED_FINE_2241_TO_2260,
      RDIMM_SPEED_FINE_2261_TO_2280,
      RDIMM_SPEED_FINE_2281_TO_2300,
      RDIMM_SPEED_FINE_2301_TO_2320,
      RDIMM_SPEED_FINE_2321_TO_2340,
      RDIMM_SPEED_FINE_2341_TO_2360,
      RDIMM_SPEED_FINE_2361_TO_2380,
      RDIMM_SPEED_FINE_2381_TO_2400,
      RDIMM_SPEED_FINE_2401_TO_2420,
      RDIMM_SPEED_FINE_2421_TO_2440,
      RDIMM_SPEED_FINE_2441_TO_2460,
      RDIMM_SPEED_FINE_2461_TO_2480,
      RDIMM_SPEED_FINE_2481_TO_2500,
      RDIMM_SPEED_FINE_2501_TO_2520,
      RDIMM_SPEED_FINE_2521_TO_2540,
      RDIMM_SPEED_FINE_2541_TO_2560,
      RDIMM_SPEED_FINE_2561_TO_2580,
      RDIMM_SPEED_FINE_2581_TO_2600,
      RDIMM_SPEED_FINE_2601_TO_2620,
      RDIMM_SPEED_FINE_2621_TO_2640,
      RDIMM_SPEED_FINE_2641_TO_2660,
      RDIMM_SPEED_FINE_2661_TO_2680,
      RDIMM_SPEED_FINE_2681_TO_2700,
      RDIMM_SPEED_FINE_2701_TO_2720,
      RDIMM_SPEED_FINE_2721_TO_2740,
      RDIMM_SPEED_FINE_2741_TO_2760,
      RDIMM_SPEED_FINE_2761_TO_2780,
      RDIMM_SPEED_FINE_2781_TO_2800,
      RDIMM_SPEED_FINE_2801_TO_2820,
      RDIMM_SPEED_FINE_2821_TO_2840,
      RDIMM_SPEED_FINE_2841_TO_2860,
      RDIMM_SPEED_FINE_2861_TO_2880,
      RDIMM_SPEED_FINE_2881_TO_2900,
      RDIMM_SPEED_FINE_2901_TO_2920,
      RDIMM_SPEED_FINE_2921_TO_2940,
      RDIMM_SPEED_FINE_2941_TO_2960,
      RDIMM_SPEED_FINE_2961_TO_2980,
      RDIMM_SPEED_FINE_2981_TO_3000,
      RDIMM_SPEED_FINE_3001_TO_3020,
      RDIMM_SPEED_FINE_3021_TO_3040,
      RDIMM_SPEED_FINE_3041_TO_3060,
      RDIMM_SPEED_FINE_3061_TO_3080,
      RDIMM_SPEED_FINE_3081_TO_3100,
      RDIMM_SPEED_FINE_3101_TO_3120,
      RDIMM_SPEED_FINE_3121_TO_3140,
      RDIMM_SPEED_FINE_3141_TO_3160,
      RDIMM_SPEED_FINE_3161_TO_3180,
      RDIMM_SPEED_FINE_3181_TO_3200
   } rcd_rdimm_fine_speed_t;

   typedef enum
   {
      CONTEXT_1,
      CONTEXT_2
   } rcd_context_t;

   typedef enum
   {
      VDD_1P2V,
      VDD_RESERVED_LOWER
   } rcd_vdd_t;

   typedef enum
   {
      QVREFCA_VDD_HALF_BVREFCA_VDD_HALF,
      QVREFCA_VREF_INT_BVREFCA_VDD_HALF,
      QVREFCA_VDD_HALF_BVREFCA_VREF_INT,
      QVREFCA_EXTERNAL_BVREFCA_EXTERNAL
   } rcd_vrefca_src_t;

   typedef enum
   {
      RX_VREFIN_SRC_INTERNAL,
      RX_VREFIN_SRC_EXTERNAL
   } rcd_vrefin_src_t;

   typedef enum
   {
      NORMAL_MODE,
      CLOCK_TO_CA_TRAINING_MODE,
      DCS0N_LOOPBACK_MODE,
      DCS1N_LOOPBACK_MODE,
      DCKE0_LOOPBACK_MODE,
      DCKE1_LOOPBACK_MODE,
      DODT0_LOOPBACK_MODE,
      DODT1_LOOPBACK_MODE
   } rcd_training_mode;

   typedef enum
   {
      DIRECT_DUALCS_MODE,
      DIRECT_QUADCS_MODE,
      ENCODED_QUADCS_MODE
   } rcd_cs_mode_t;

   typedef enum
   {
      LRDIMM,
      RDIMM
   } rcd_dimm_t;

   typedef enum
   {
      ALERT_N_STICKY,
      ALERT_N_PULSE
   } rcd_alert_assert_t;

   typedef enum
   {
      ALERT_N_REENABLE_OFF,
      ALERT_N_REENABLE_ON
   } rcd_alert_renable_t;

   typedef enum
   {
      LATENCY_1NCK,
      LATENCY_2NCK,
      LATENCY_3NCK,
      LATENCY_4NCK,
      LATENCY_0NCK
   } rcd_latency_adder_t;

   typedef enum
   {
      VREFCA_INT_50P00,
      VREFCA_INT_50P83,
      VREFCA_INT_51P67,
      VREFCA_INT_52P60,
      VREFCA_INT_53P33,
      VREFCA_INT_54P17,
      VREFCA_INT_55P00,
      VREFCA_INT_55P83,
      VREFCA_INT_56P67,
      VREFCA_INT_57P50,
      VREFCA_INT_58P33,
      VREFCA_INT_59P17,
      VREFCA_INT_60P00,
      VREFCA_INT_60P83,
      VREFCA_INT_61P67,
      VREFCA_INT_62P50,
      VREFCA_INT_63P33,
      VREFCA_INT_64P17,
      VREFCA_INT_65P00,
      VREFCA_INT_65P83,
      VREFCA_INT_66P67,
      VREFCA_INT_33P33,
      VREFCA_INT_34P17,
      VREFCA_INT_35P00,
      VREFCA_INT_35P83,
      VREFCA_INT_36P67,
      VREFCA_INT_37P50,
      VREFCA_INT_38P33,
      VREFCA_INT_39P17,
      VREFCA_INT_40P00,
      VREFCA_INT_40P83,
      VREFCA_INT_41P67,
      VREFCA_INT_42P50,
      VREFCA_INT_43P33,
      VREFCA_INT_44P17,
      VREFCA_INT_45P00,
      VREFCA_INT_45P83,
      VREFCA_INT_46P67,
      VREFCA_INT_47P50,
      VREFCA_INT_48P33,
      VREFCA_INT_49P17
   } rcd_vrefca_int_t;

   typedef enum
   {
      ODT_ADDITION_OFF,
      ODT_ADDITION_1CYC,
      ODT_ADDITION_2CYC
   } rcd_odt_addition_t;

   typedef enum
   {
      IBT_100OHM,
      IBT_150OHM,
      IBT_300OHM,
      IBT_OFF
   } rcd_ibt_t;

   typedef enum
   {
      ODT_WR_TIMING_0CYC,
      ODT_WR_TIMING_1CYC
   } rcd_odt_wr_timing_t;

   typedef enum
   {
      ODT_RD_TIMING_0CYC,
      ODT_RD_TIMING_1CYC,
      ODT_RD_TIMING_2CYC,
      ODT_RD_TIMING_3CYC,
      ODT_RD_TIMING_4CYC,
      ODT_RD_TIMING_5CYC,
      ODT_RD_TIMING_6CYC,
      ODT_RD_TIMING_7CYC,
      ODT_RD_TIMING_8CYC
   } rcd_odt_rd_timing_t;

   typedef enum
   {
      QODT_CTRL_DODT01,
      QODT_CTRL_DODT0,
      QODT_CTRL_OFF,
      QODT_CTRL_INTERNAL
   } rcd_qodt_ctrl_t;

   typedef enum
   {
      QODT_ON,
      QODT_OFF
   } rcd_qodt_t;


   typedef struct
   {
      rcd_enum_t              current_rc;

      rcd_en_t                rc00_output_inversion;
      rcd_en_t                rc00_weak_drive;
      rcd_en_t                rc00_a_outputs;
      rcd_en_t                rc00_b_outputs;

      rcd_en_t                rc01_y0_clock;
      rcd_en_t                rc01_y1_clock;
      rcd_en_t                rc01_y2_clock;
      rcd_en_t                rc01_y3_clock;

      rcd_en_t                rc02_da17_ibt;
      rcd_en_t                rc02_dpar_ibt;
      rcd_en_t                rc02_transparent_mode;
      rcd_freqband_t          rc02_freqband;

      rcd_drive_t             rc03_qac_drive;
      rcd_drive_t             rc03_qcs_drive;

      rcd_drive_t             rc04_qodt_drive;
      rcd_drive_t             rc04_qcke_drive;

      rcd_drive_t             rc05_clka_drive;
      rcd_drive_t             rc05_clkb_drive;

      rcd_qxc_t               rc08_qxc_outputs;
      rcd_en_t                rc08_qpar_outputs;
      rcd_en_t                rc08_a17;

      rcd_en_t                rc09_dcs1_ibt;
      rcd_en_t                rc09_dcs1;
      rcd_cke_pd_mode_t       rc09_cke_pd_mode;
      rcd_en_t                rc09_cke_pd;

      rcd_rdimm_speed_t       rc0a_rdimm_speed;
      rcd_context_t           rc0a_context;

      rcd_vdd_t               rc0b_vdd;
      rcd_vrefca_src_t        rc0b_vrefca_src;
      rcd_vrefin_src_t        rc0b_vrefin_src;

      rcd_training_mode       rc0c_training_mode;

      rcd_cs_mode_t           rc0d_cs_mode;
      rcd_dimm_t              rc0d_dimm;
      rcd_en_t                rc0d_mrs_mirr;

      rcd_en_t                rc0e_parity;
      rcd_alert_assert_t      rc0e_alert_assert;
      rcd_alert_renable_t     rc0e_alert_reenable;

      rcd_latency_adder_t     rc0f_latency_adder;

      rcd_vrefca_int_t        rc1x_vrefca_int;


      rcd_rdimm_fine_speed_t  rc3x_rdimm_fine_speed;


      rcd_odt_addition_t      rc5x_wr_odt_addition;
      rcd_odt_addition_t      rc5x_rd_odt_addition;

      rcd_ibt_t               rc7x_ca_ibt;
      rcd_ibt_t               rc7x_dcs_ibt;
      rcd_ibt_t               rc7x_dcke_ibt;
      rcd_ibt_t               rc7x_dodt_ibt;

      rcd_odt_wr_timing_t     rc8x_odt_wr_timing;
      rcd_odt_rd_timing_t     rc8x_odt_rd_timing;
      rcd_en_t                rc8x_bodt_outputs;
      rcd_qodt_ctrl_t         rc8x_qodt_ctrl;

      rcd_qodt_t              rc9x_wr_qodt0_r0;
      rcd_qodt_t              rc9x_wr_qodt1_r0;
      rcd_qodt_t              rc9x_wr_qodt0_r1;
      rcd_qodt_t              rc9x_wr_qodt1_r1;
      rcd_qodt_t              rc9x_wr_qodt0_r2;
      rcd_qodt_t              rc9x_wr_qodt1_r2;
      rcd_qodt_t              rc9x_wr_qodt0_r3;
      rcd_qodt_t              rc9x_wr_qodt1_r3;

      rcd_qodt_t              rcax_rd_qodt0_r0;
      rcd_qodt_t              rcax_rd_qodt1_r0;
      rcd_qodt_t              rcax_rd_qodt0_r1;
      rcd_qodt_t              rcax_rd_qodt1_r1;
      rcd_qodt_t              rcax_rd_qodt0_r2;
      rcd_qodt_t              rcax_rd_qodt1_r2;
      rcd_qodt_t              rcax_rd_qodt0_r3;
      rcd_qodt_t              rcax_rd_qodt1_r3;

      rcd_en_t                rcbx_dc0_ibt;
      rcd_en_t                rcbx_dc1_ibt;
      rcd_en_t                rcbx_dc2_ibt;
      rcd_en_t                rcbx_ddr4db01_mrs_snoop;
      rcd_en_t                rcbx_ddr4rcd01_mrs_snoop;
      rcd_en_t                rcbx_dcke1_ibt;
      rcd_en_t                rcbx_dcke1;

   } rcd_t;


   rcd_t rcd;

   logic mem_ck_diff;


   assign Y_t = {4{CK_t}};
   assign Y_c = {4{CK_c}};

   assign QRST_n = DRST_n;

   wire [3:0] active_ranks;
   wire effective_DBG1;
   assign active_ranks = {4'b0, ~DCS_n[PORT_MEM_CS_N_WIDTH-1:0]};
   assign effective_DBG1 = (ADDRESS_MIRRORING && (active_ranks & 4'b1010)) ? DBG[0] : DBG[1];

   logic   [3:0][17:0] QAA_pre_reg;
   logic   [3:0][17:0] QBA_pre_reg;
   logic   [3:0][1:0]  QABA_pre_reg;
   logic   [3:0][1:0]  QABG_pre_reg;
   logic   [3:0][1:0]  QBBA_pre_reg;
   logic   [3:0][1:0]  QBBG_pre_reg;
   logic   [3:0]       QAACT_n_pre_reg;
   logic   [3:0]       QBACT_n_pre_reg;
   logic   [3:0][3:0]  QACS_n_pre_reg;
   logic   [3:0][3:0]  QBCS_n_pre_reg;
   logic   [3:0][1:0]  QAC_pre_reg;
   logic   [3:0][1:0]  QBC_pre_reg;
   logic   [3:0][1:0]  QAODT_pre_reg;
   logic   [3:0][1:0]  QBODT_pre_reg;
   logic   [3:0][1:0]  DCKE_reg;
   logic   [3:0]       QAPAR_pre_reg;
   logic   [3:0]       QBPAR_pre_reg;

   always_ff @(posedge CK_t)
   begin
      QAA_pre_reg[0]     <= DA;
      QBA_pre_reg[0]     <= ((ADDRESS_MIRRORING) ? ({~DA[17], DA[16:14], ~DA[11], DA[12], ~DA[13], DA[10], ~DA[9], ~DA[7], ~DA[8], ~DA[5], ~DA[6], ~DA[3], ~DA[4], DA[2:0]}) : ({~DA[17], DA[16:14], ~DA[13], DA[12], ~DA[11], DA[10], ~DA[9:3], DA[2:0]}));
      QABA_pre_reg[0]    <= DBA;
      QBBA_pre_reg[0]    <= ((ADDRESS_MIRRORING) ? ({~DBA[0], ~DBA[1]}) : (~DBA));
      QABG_pre_reg[0]    <= DBG;
      QBBG_pre_reg[0]    <= ((ADDRESS_MIRRORING) ? ({~DBG[0], ~DBG[1]}) : (~DBG));
      QAACT_n_pre_reg[0] <= DACT_n;
      QBACT_n_pre_reg[0] <= DACT_n;
      QACS_n_pre_reg[0]  <= ((DACT_n == 1'b1) && (DA[16:14] == 3'b0) && (effective_DBG1 == 1'b1)) ? 4'b1111 : DCS_n;
      QBCS_n_pre_reg[0]  <= ((DACT_n == 1'b1) && (DA[16:14] == 3'b0) && (effective_DBG1 == 1'b0)) ? 4'b1111 : DCS_n;
      QAC_pre_reg[0]     <= DC;
      QBC_pre_reg[0]     <= DC;
      QAODT_pre_reg[0]   <= DODT;
      QBODT_pre_reg[0]   <= DODT;
      DCKE_reg[0]        <= DCKE;
      QAPAR_pre_reg[0]   <= DPAR;
      QBPAR_pre_reg[0]   <= ~DPAR;

      QAA_pre_reg[3:1]     <= QAA_pre_reg[2:0];
      QBA_pre_reg[3:1]     <= QBA_pre_reg[2:0];
      QABA_pre_reg[3:1]    <= QABA_pre_reg[2:0];
      QBBA_pre_reg[3:1]    <= QBBA_pre_reg[2:0];
      QABG_pre_reg[3:1]    <= QABG_pre_reg[2:0];
      QBBG_pre_reg[3:1]    <= QBBG_pre_reg[2:0];
      QAACT_n_pre_reg[3:1] <= QAACT_n_pre_reg[2:0];
      QBACT_n_pre_reg[3:1] <= QBACT_n_pre_reg[2:0];
      QACS_n_pre_reg[3:1]  <= QACS_n_pre_reg[2:0];
      QBCS_n_pre_reg[3:1]  <= QBCS_n_pre_reg[2:0];
      QAC_pre_reg[3:1]     <= QAC_pre_reg[2:0];
      QBC_pre_reg[3:1]     <= QBC_pre_reg[2:0];
      QAODT_pre_reg[3:1]   <= QAODT_pre_reg[2:0];
      QBODT_pre_reg[3:1]   <= QBODT_pre_reg[2:0];
      DCKE_reg[3:1]        <= DCKE_reg[2:0];
      QAPAR_pre_reg[3:1]   <= QAPAR_pre_reg[2:0];
      QBPAR_pre_reg[3:1]   <= QBPAR_pre_reg[2:0];
   end

   always_comb
   begin
      if (rcd.rc0f_latency_adder == LATENCY_1NCK) begin
         QAA     = QAA_pre_reg[2];
         QBA     = QBA_pre_reg[2];
         QABA    = QABA_pre_reg[2];
         QBBA    = QBBA_pre_reg[2];
         QABG    = QABG_pre_reg[2];
         QBBG    = QBBG_pre_reg[2];
         QAACT_n = QAACT_n_pre_reg[2];
         QBACT_n = QBACT_n_pre_reg[2];
         QACS_n  = QACS_n_pre_reg[2];
         QBCS_n  = QBCS_n_pre_reg[2];
         QAC     = QAC_pre_reg[2];
         QBC     = QBC_pre_reg[2];
         QAODT   = QAODT_pre_reg[2];
         QBODT   = QBODT_pre_reg[2];

         QAPAR   = QAPAR_pre_reg[1];
         QBPAR   = QBPAR_pre_reg[1];

         QACKE   = DCKE_reg[0] || DCKE_reg[1] || DCKE_reg[2];
         QBCKE   = DCKE_reg[0] || DCKE_reg[1] || DCKE_reg[2];

      end else if (rcd.rc0f_latency_adder == LATENCY_2NCK) begin
         QAA     = QAA_pre_reg[3];
         QBA     = QBA_pre_reg[3];
         QABA    = QABA_pre_reg[3];
         QBBA    = QBBA_pre_reg[3];
         QABG    = QABG_pre_reg[3];
         QBBG    = QBBG_pre_reg[3];
         QAACT_n = QAACT_n_pre_reg[3];
         QBACT_n = QBACT_n_pre_reg[3];
         QACS_n  = QACS_n_pre_reg[3];
         QBCS_n  = QBCS_n_pre_reg[3];
         QAC     = QAC_pre_reg[3];
         QBC     = QBC_pre_reg[3];
         QAODT   = QAODT_pre_reg[3];
         QBODT   = QBODT_pre_reg[3];

         QAPAR   = QAPAR_pre_reg[2];
         QBPAR   = QBPAR_pre_reg[2];

         QACKE   = DCKE_reg[0] || DCKE_reg[1] || DCKE_reg[2] || DCKE_reg[3];
         QBCKE   = DCKE_reg[0] || DCKE_reg[1] || DCKE_reg[2] || DCKE_reg[3];
      end else begin
         $error("Model does not support RC0F Latency Adder Mode other than 1CK or 2CK");
      end
   end

   localparam PARITY_TO_ALERT_N_LATENCY = 3;
   localparam PARITY_PULSE_WIDTH = 48;
   localparam ALERT_N_PIPELINE_SIZE = PARITY_PULSE_WIDTH + PARITY_TO_ALERT_N_LATENCY + 1;

   bit expected_parity;
   bit parity_cmp_en;
   bit [ALERT_N_PIPELINE_SIZE-1:0] parity_alert_n_pipeline;

   always @(posedge mem_ck_diff)
   begin
      if (rcd.rc0e_parity == ENABLED)
      begin
         if (DCS_n[PORT_MEM_CS_N_WIDTH-1:0] != {PORT_MEM_CS_N_WIDTH{1'b1}}) begin
            expected_parity <= ^{DA, DBA, DBG, DACT_n, DC};
            parity_cmp_en   <= 1'b1;
         end else begin
            expected_parity <= 1'b0;
            parity_cmp_en   <= 1'b0;
         end

         if (parity_cmp_en) begin
            if (DPAR != expected_parity) begin
               parity_alert_n_pipeline[ALERT_N_PIPELINE_SIZE-1:PARITY_TO_ALERT_N_LATENCY] <= '0;
               parity_alert_n_pipeline[PARITY_TO_ALERT_N_LATENCY-1:0] <= parity_alert_n_pipeline[PARITY_TO_ALERT_N_LATENCY:1];
            end else begin
               parity_alert_n_pipeline  <= {1'b1, parity_alert_n_pipeline[ALERT_N_PIPELINE_SIZE-1:1]};
            end
         end else begin
            parity_alert_n_pipeline  <= {1'b1, parity_alert_n_pipeline[ALERT_N_PIPELINE_SIZE-1:1]};
         end
      end
   end

   assign ALERT_n = ERROR_IN_n && parity_alert_n_pipeline[0];

   function string rc_string (rcd_enum_t rcd);
      case (rcd)
         RC00: rc_string = "Global Features Control Word";
         RC01: rc_string = "Clock Driver Enable Control Word";
         RC02: rc_string = "Timing and IBT Control Word";
         RC03: rc_string = "CA and CS Signals Driver Characteristics Control Word";
         RC04: rc_string = "ODT and CKE Signals Driver Characteristics Control Word";
         RC05: rc_string = "Clock Driver Characteristics Control Word";
         RC06: rc_string = "Command Space Control Word";
         RC07: rc_string = "RFU";
         RC08: rc_string = "Input/Output Configuration Control Word";
         RC09: rc_string = "Power Saving Settings Control Word";
         RC0A: rc_string = "RDIMM Operating Speed";
         RC0B: rc_string = "Operating Voltage VDD and VREF Source Control Word";
         RC0C: rc_string = "Training Control Word";
         RC0D: rc_string = "DIMM Configuration Control Word";
         RC0E: rc_string = "Parity Control Word";
         RC0F: rc_string = "Command Latency Adder Control Word";

         RC1X: rc_string = "Internal Vref Control Word";
         RC2X: rc_string = "I2C Bus Control Word";
         RC3X: rc_string = "Fine Granularity RDIMM Operating Speed";
         RC4X: rc_string = "CW Source Selection Control Word";
         RC5X: rc_string = "CW Destination Selection & Write/Read Additional QxODT[1:0] Signal High Control word";
         RC6X: rc_string = "CW Data Control Word";
         RC7X: rc_string = "IBT Control Word";
         RC8X: rc_string = "ODT Control Word";
         RC9X: rc_string = "QxODT[1:0] Write Pattern Control Word";
         RCAX: rc_string = "QxODT[1:0] Read Pattern Control Word";
         RCBX: rc_string = "IBT and MRS Snoop Control Word";
         default: rc_string = "Error - Unknown RCD Value";

      endcase
   endfunction

   task rcd_reset();
      rcd.rc00_output_inversion     = ENABLED;
      rcd.rc00_weak_drive           = DISABLED;
      rcd.rc00_a_outputs            = ENABLED;
      rcd.rc00_b_outputs            = ENABLED;

      rcd.rc01_y0_clock             = ENABLED;
      rcd.rc01_y1_clock             = ENABLED;
      rcd.rc01_y2_clock             = ENABLED;
      rcd.rc01_y3_clock             = ENABLED;

      rcd.rc02_da17_ibt             = ENABLED;
      rcd.rc02_dpar_ibt             = ENABLED;
      rcd.rc02_transparent_mode     = DISABLED;
      rcd.rc02_freqband             = FREQUENCY_BAND_OPERATION;

      rcd.rc03_qac_drive            = DRIVE_LIGHT;
      rcd.rc03_qcs_drive            = DRIVE_LIGHT;

      rcd.rc04_qodt_drive           = DRIVE_LIGHT;
      rcd.rc04_qcke_drive           = DRIVE_LIGHT;

      rcd.rc05_clka_drive           = DRIVE_LIGHT;
      rcd.rc05_clkb_drive           = DRIVE_LIGHT;

      rcd.rc08_qxc_outputs          = QXC_C_EN_111;
      rcd.rc08_qpar_outputs         = ENABLED;
      rcd.rc08_a17                  = ENABLED;

      rcd.rc09_dcs1_ibt             = ENABLED;
      rcd.rc09_dcs1                 = ENABLED;
      rcd.rc09_cke_pd_mode          = CKE_POWERDOWN_IBT_ON;
      rcd.rc09_cke_pd               = DISABLED;

      rcd.rc0a_rdimm_speed          = RDIMM_SPEED_UP_TO_1600;
      rcd.rc0a_context              = CONTEXT_1;

      rcd.rc0b_vdd                  = VDD_1P2V;
      rcd.rc0b_vrefca_src           = QVREFCA_VDD_HALF_BVREFCA_VDD_HALF;
      rcd.rc0b_vrefin_src           = RX_VREFIN_SRC_INTERNAL;

      rcd.rc0c_training_mode        = NORMAL_MODE;

      rcd.rc0d_cs_mode              = DIRECT_DUALCS_MODE;
      rcd.rc0d_dimm                 = LRDIMM;
      rcd.rc0d_mrs_mirr             = DISABLED;

      rcd.rc0e_parity               = DISABLED;
      rcd.rc0e_alert_assert         = ALERT_N_STICKY;
      rcd.rc0e_alert_reenable       = ALERT_N_REENABLE_OFF;

      rcd.rc0f_latency_adder        = LATENCY_1NCK;

      rcd.rc1x_vrefca_int           = VREFCA_INT_50P00;

      rcd.rc3x_rdimm_fine_speed     = RDIMM_SPEED_FINE_1241_TO_1260;

      rcd.rc5x_wr_odt_addition      = ODT_ADDITION_OFF;
      rcd.rc5x_rd_odt_addition      = ODT_ADDITION_OFF;

      rcd.rc7x_ca_ibt               = IBT_100OHM;
      rcd.rc7x_dcs_ibt              = IBT_100OHM;
      rcd.rc7x_dcke_ibt             = IBT_100OHM;
      rcd.rc7x_dodt_ibt             = IBT_100OHM;

      rcd.rc8x_odt_wr_timing        = ODT_WR_TIMING_0CYC;
      rcd.rc8x_odt_rd_timing        = ODT_RD_TIMING_0CYC;
      rcd.rc8x_bodt_outputs         = ENABLED;
      rcd.rc8x_qodt_ctrl            = QODT_CTRL_DODT01;

      rcd.rc9x_wr_qodt0_r0          = QODT_OFF;
      rcd.rc9x_wr_qodt1_r0          = QODT_OFF;
      rcd.rc9x_wr_qodt0_r1          = QODT_OFF;
      rcd.rc9x_wr_qodt1_r1          = QODT_OFF;
      rcd.rc9x_wr_qodt0_r2          = QODT_OFF;
      rcd.rc9x_wr_qodt1_r2          = QODT_OFF;
      rcd.rc9x_wr_qodt0_r3          = QODT_OFF;
      rcd.rc9x_wr_qodt1_r3          = QODT_OFF;

      rcd.rcax_rd_qodt0_r0          = QODT_OFF;
      rcd.rcax_rd_qodt1_r0          = QODT_OFF;
      rcd.rcax_rd_qodt0_r1          = QODT_OFF;
      rcd.rcax_rd_qodt1_r1          = QODT_OFF;
      rcd.rcax_rd_qodt0_r2          = QODT_OFF;
      rcd.rcax_rd_qodt1_r2          = QODT_OFF;
      rcd.rcax_rd_qodt0_r3          = QODT_OFF;
      rcd.rcax_rd_qodt1_r3          = QODT_OFF;

      rcd.rcbx_dc0_ibt              = ENABLED;
      rcd.rcbx_dc1_ibt              = ENABLED;
      rcd.rcbx_dc2_ibt              = ENABLED;
      rcd.rcbx_ddr4db01_mrs_snoop   = ENABLED;
      rcd.rcbx_ddr4rcd01_mrs_snoop  = ENABLED;
      rcd.rcbx_dcke1_ibt            = ENABLED;
      rcd.rcbx_dcke1                = ENABLED;


   endtask


   always @(CK_t or CK_c)
   begin
      case ({CK_t, CK_c})
         2'b00: mem_ck_diff = mem_ck_diff;
         2'b01: mem_ck_diff = 1'b0;
         2'b10: mem_ck_diff = 1'b1;
         2'b11: mem_ck_diff = mem_ck_diff;
         default: mem_ck_diff = 1'bx;
      endcase
   end


   always @(posedge mem_ck_diff)
   begin
      if ( ({DBG[0], DBA[1:0]} == 3'b111) && ({DCS_n[0], DACT_n, DA[16:14]} == {1'b0, 1'b1, 3'b000}) )
      begin
         if (    ((rcd.rc0d_cs_mode == ENCODED_QUADCS_MODE) && (DC[0] == 1'b0))
              || (rcd.rc0d_cs_mode != ENCODED_QUADCS_MODE) )
         begin
            $display("   DA[12]=%x  DA[11: 8]=%x%x%x%x  DA[ 7: 4]=%x%x%x%x  DA[ 3: 0]=%x%x%x%x",
                         DA[12], DA[11], DA[10], DA[9], DA[8], DA[7],
                         DA[6], DA[5], DA[4], DA[3], DA[2], DA[1], DA[0]);
            casex (DA[12:4])
               9'b0_0000_0000: begin rcd.current_rc <= RC00; update_rc(RC00, DA[7:0]); end
               9'b0_0000_0001: begin rcd.current_rc <= RC01; update_rc(RC01, DA[7:0]); end
               9'b0_0000_0010: begin rcd.current_rc <= RC02; update_rc(RC02, DA[7:0]); end
               9'b0_0000_0011: begin rcd.current_rc <= RC03; update_rc(RC03, DA[7:0]); end
               9'b0_0000_0100: begin rcd.current_rc <= RC04; update_rc(RC04, DA[7:0]); end
               9'b0_0000_0101: begin rcd.current_rc <= RC05; update_rc(RC05, DA[7:0]); end
               9'b0_0000_0110: begin rcd.current_rc <= RC06; update_rc(RC06, DA[7:0]); end
               9'b0_0000_0111: begin rcd.current_rc <= RC07; update_rc(RC07, DA[7:0]); end
               9'b0_0000_1000: begin rcd.current_rc <= RC08; update_rc(RC08, DA[7:0]); end
               9'b0_0000_1001: begin rcd.current_rc <= RC09; update_rc(RC09, DA[7:0]); end
               9'b0_0000_1010: begin rcd.current_rc <= RC0A; update_rc(RC0A, DA[7:0]); end
               9'b0_0000_1011: begin rcd.current_rc <= RC0B; update_rc(RC0B, DA[7:0]); end
               9'b0_0000_1100: begin rcd.current_rc <= RC0C; update_rc(RC0C, DA[7:0]); end
               9'b0_0000_1101: begin rcd.current_rc <= RC0D; update_rc(RC0D, DA[7:0]); end
               9'b0_0000_1110: begin rcd.current_rc <= RC0E; update_rc(RC0E, DA[7:0]); end
               9'b0_0000_1111: begin rcd.current_rc <= RC0F; update_rc(RC0F, DA[7:0]); end
               9'b0_0001_xxxx: begin rcd.current_rc <= RC1X; update_rc(RC1X, DA[7:0]); end
               9'b0_0010_xxxx: begin rcd.current_rc <= RC2X; update_rc(RC2X, DA[7:0]); end
               9'b0_0011_xxxx: begin rcd.current_rc <= RC3X; update_rc(RC3X, DA[7:0]); end
               9'b0_0100_xxxx: begin rcd.current_rc <= RC4X; update_rc(RC4X, DA[7:0]); end
               9'b0_0101_xxxx: begin rcd.current_rc <= RC5X; update_rc(RC5X, DA[7:0]); end
               9'b0_0110_xxxx: begin rcd.current_rc <= RC6X; update_rc(RC6X, DA[7:0]); end
               9'b0_0111_xxxx: begin rcd.current_rc <= RC7X; update_rc(RC7X, DA[7:0]); end
               9'b0_1000_xxxx: begin rcd.current_rc <= RC8X; update_rc(RC8X, DA[7:0]); end
               9'b0_1001_xxxx: begin rcd.current_rc <= RC9X; update_rc(RC9X, DA[7:0]); end
               9'b0_1010_xxxx: begin rcd.current_rc <= RCAX; update_rc(RCAX, DA[7:0]); end
               9'b0_1011_xxxx: begin rcd.current_rc <= RCBX; update_rc(RCBX, DA[7:0]); end
            endcase
         end
      end
   end


   task print_rc(input rcd_enum_t rc);
      $display("   RCD Write to %s: %s", rc.name(), rc_string(rc));
      case (rc)
         RC00:
         begin
            $display("    Output Inversion: %s", rcd.rc00_output_inversion.name());
            $display("    Weak Drive:       %s", rcd.rc00_weak_drive.name());
            $display("    A-Side Outputs:   %s", rcd.rc00_a_outputs.name());
            $display("    B-Side Outputs:   %s", rcd.rc00_b_outputs.name());
         end

         RC01:
         begin
            $display("    Y0_t/Y0_c Clocks: %s", rcd.rc01_y0_clock.name());
            $display("    Y1_t/Y1_c Clocks: %s", rcd.rc01_y1_clock.name());
            $display("    Y2_t/Y2_c Clocks: %s", rcd.rc01_y2_clock.name());
            $display("    Y3_t/Y3_c Clocks: %s", rcd.rc01_y3_clock.name());
         end

         RC02:
         begin
            $display("    DA17 Input Bus Termination: %s", rcd.rc02_da17_ibt.name());
            $display("    DPAR Input Bus Termination: %s", rcd.rc02_dpar_ibt.name());
            $display("    Transparent Mode:           %s", rcd.rc02_transparent_mode.name());
            $display("    Frequency Band Select:      %s", rcd.rc02_freqband.name());
         end

         RC03:
         begin
            $display("    A/C Output Drive: %s", rcd.rc03_qac_drive.name());
            $display("    CSn Output Drive: %s", rcd.rc03_qcs_drive.name());
         end

         RC04:
         begin
            $display("    ODT Output Drive: %s", rcd.rc04_qodt_drive.name());
            $display("    CKE Output Drive: %s", rcd.rc04_qcke_drive.name());
         end

         RC05:
         begin
            $display("    CLKA Output Drive: %s", rcd.rc05_clka_drive.name());
            $display("    CLKB Output Drive: %s", rcd.rc05_clkb_drive.name());
         end

         RC06:
         begin
         end

         RC07:
         begin
         end

         RC08:
         begin
            $display("    QxC[2:0] Output Enable: %s", rcd.rc08_qxc_outputs.name());
            $display("    QPAR Output Enable:     %s", rcd.rc08_qpar_outputs.name());
            $display("    DA17/QxA17 I/O Enable:  %s", rcd.rc08_a17.name());
         end

         RC09:
         begin
            $display("    DCS1 Input Bus Termination: %s", rcd.rc09_dcs1_ibt.name());
            $display("    DCS1/QxCS1 I/O Enable:      %s", rcd.rc09_dcs1.name());
            $display("    CKE Power-Down Mode:        %s", rcd.rc09_cke_pd_mode.name());
            $display("    CKE Power-Down Mode Enable: %s", rcd.rc09_cke_pd.name());
         end

         RC0A:
         begin
            $display("    RDIMM Operating Speed: %s", rcd.rc0a_rdimm_speed.name());
            $display("    Context for operation: %s", rcd.rc0a_context.name());
         end

         RC0B:
         begin
            $display("    Register VDD Operating Voltage: %s", rcd.rc0b_vdd.name());
            $display("    QVrefCA and BVrefCA Sources:    %s", rcd.rc0b_vrefca_src.name());
            $display("    Input Receiver Vref Source:     %s", rcd.rc0b_vrefin_src.name());
         end

         RC0C:
         begin
            $display("    Training Mode Selection: %s", rcd.rc0c_training_mode.name());
         end

         RC0D:
         begin
            $display("    Chip Select Mode:          %s", rcd.rc0d_cs_mode.name());
            $display("    DIMM Type:                 %s", rcd.rc0d_dimm.name());
            $display("    Address Mirroring for MRS: %s", rcd.rc0d_mrs_mirr.name());
         end

         RC0E:
         begin
            $display("    Parity Checking:   %s", rcd.rc0e_parity.name());
            $display("    ALERT_n Assertion: %s", rcd.rc0e_alert_assert.name());
            $display("    ALERT_n Re-Enable: %s", rcd.rc0e_alert_reenable.name());
         end

         RC0F:
         begin
            $display("    DRAM Command Latency Addition: %s", rcd.rc0f_latency_adder.name());
         end

         RC1X:
         begin
            $display("    Internal VrefCA Control Word: %s", rcd.rc1x_vrefca_int.name());
         end

         RC2X:
         begin
            $display("    Fine Granularity RDIMM Speed: %s", rcd.rc3x_rdimm_fine_speed.name());
         end

         RC3X:
         begin
         end

         RC4X:
         begin
         end

         RC5X:
         begin
         end

         RC6X:
         begin
         end

         RC7X:
         begin
            $display("      CA Input Bus Termination: %s", rcd.rc7x_ca_ibt.name());
            $display("    DCSn Input Bus Termination: %s", rcd.rc7x_dcs_ibt.name());
            $display("    DCKE Input Bus Termination: %s", rcd.rc7x_dcke_ibt.name());
            $display("    DODT Input Bus Termination: %s", rcd.rc7x_dodt_ibt.name());
         end

         RC8X:
         begin
            $display("    QxODT[1:0] Write Timing: %s", rcd.rc8x_odt_wr_timing.name());
            $display("    QxODT[1:0] Read  Timing: %s", rcd.rc8x_odt_rd_timing.name());
            $display("    BODT Output Driver:      %s", rcd.rc8x_bodt_outputs.name());
            $display("    ODT In/Out/IBT Control:  %s", rcd.rc8x_qodt_ctrl.name());
         end

         RC9X:
         begin
            $display("    QxODT[0] for Rank 0 Write: %s", rcd.rc9x_wr_qodt0_r0.name());
            $display("    QxODT[1] for Rank 0 Write: %s", rcd.rc9x_wr_qodt1_r0.name());
            $display("    QxODT[0] for Rank 1 Write: %s", rcd.rc9x_wr_qodt0_r1.name());
            $display("    QxODT[1] for Rank 1 Write: %s", rcd.rc9x_wr_qodt1_r1.name());
            $display("    QxODT[0] for Rank 2 Write: %s", rcd.rc9x_wr_qodt0_r2.name());
            $display("    QxODT[1] for Rank 2 Write: %s", rcd.rc9x_wr_qodt1_r2.name());
            $display("    QxODT[0] for Rank 3 Write: %s", rcd.rc9x_wr_qodt0_r3.name());
            $display("    QxODT[1] for Rank 3 Write: %s", rcd.rc9x_wr_qodt1_r3.name());
         end

         RCAX:
         begin
            $display("    QxODT[0] for Rank 0 Read:  %s", rcd.rcax_rd_qodt0_r0.name());
            $display("    QxODT[1] for Rank 0 Read:  %s", rcd.rcax_rd_qodt1_r0.name());
            $display("    QxODT[0] for Rank 1 Read:  %s", rcd.rcax_rd_qodt0_r1.name());
            $display("    QxODT[1] for Rank 1 Read:  %s", rcd.rcax_rd_qodt1_r1.name());
            $display("    QxODT[0] for Rank 2 Read:  %s", rcd.rcax_rd_qodt0_r2.name());
            $display("    QxODT[1] for Rank 2 Read:  %s", rcd.rcax_rd_qodt1_r2.name());
            $display("    QxODT[0] for Rank 3 Read:  %s", rcd.rcax_rd_qodt0_r3.name());
            $display("    QxODT[1] for Rank 3 Read:  %s", rcd.rcax_rd_qodt1_r3.name());
         end

         RCBX:
         begin
            $display("    DC0 Input Bus Termination:   %s", rcd.rcbx_dc0_ibt.name());
            $display("    DC1 Input Bus Termination:   %s", rcd.rcbx_dc1_ibt.name());
            $display("    DC2 Input Bus Termination:   %s", rcd.rcbx_dc2_ibt.name());
            $display("    DDR4 DB01 MRS Snooping:      %s", rcd.rcbx_ddr4db01_mrs_snoop.name());
            $display("    DDR4 RCD01 MRS Snooping:     %s", rcd.rcbx_ddr4rcd01_mrs_snoop.name());
            $display("    DCKE1 Input Bus Termination: %s", rcd.rcbx_dcke1_ibt.name());
            $display("    DKCE1/QxCKE1 Input/Output:   %s", rcd.rcbx_dcke1.name());
         end

         default:
         begin
            $display("    RC Command is unrecognized or not implemented");
         end
      endcase
   endtask

   task update_rc(input rcd_enum_t rc, input [7:0] da);
      case (rc)
         RC00:
         begin
            rcd.rc00_output_inversion  = (da[0] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc00_weak_drive        = (da[1] == 1'b0) ? DISABLED : ENABLED;
            rcd.rc00_a_outputs         = (da[2] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc00_b_outputs         = (da[3] == 1'b0) ? ENABLED : DISABLED;
         end

         RC01:
         begin
            rcd.rc01_y0_clock = (da[0] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc01_y1_clock = (da[1] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc01_y2_clock = (da[2] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc01_y3_clock = (da[3] == 1'b0) ? ENABLED : DISABLED;
         end

         RC02:
         begin
            rcd.rc02_da17_ibt         = (da[0] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc02_dpar_ibt         = (da[1] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc02_transparent_mode = (da[2] == 1'b0) ? DISABLED : ENABLED;
            rcd.rc02_freqband         = (da[3] == 1'b0) ? FREQUENCY_BAND_OPERATION : FREQUENCY_BAND_TEST_MODE;
         end

         RC03:
         begin
            case (da[1:0])
               2'b00: rcd.rc03_qac_drive = DRIVE_LIGHT;
               2'b01: rcd.rc03_qac_drive = DRIVE_MODERATE;
               2'b10: rcd.rc03_qac_drive = DRIVE_STRONG;
               2'b11: rcd.rc03_qac_drive = DRIVE_VERYSTRONG;
            endcase
            case (da[3:2])
               2'b00: rcd.rc03_qcs_drive = DRIVE_LIGHT;
               2'b01: rcd.rc03_qcs_drive = DRIVE_MODERATE;
               2'b10: rcd.rc03_qcs_drive = DRIVE_STRONG;
               2'b11: rcd.rc03_qcs_drive = DRIVE_VERYSTRONG;
            endcase
         end

         RC04:
         begin
            case (da[1:0])
               2'b00: rcd.rc04_qodt_drive = DRIVE_LIGHT;
               2'b01: rcd.rc04_qodt_drive = DRIVE_MODERATE;
               2'b10: rcd.rc04_qodt_drive = DRIVE_STRONG;
               2'b11: rcd.rc04_qodt_drive = DRIVE_VERYSTRONG;
            endcase
            case (da[3:2])
               2'b00: rcd.rc04_qcke_drive = DRIVE_LIGHT;
               2'b01: rcd.rc04_qcke_drive = DRIVE_MODERATE;
               2'b10: rcd.rc04_qcke_drive = DRIVE_STRONG;
               2'b11: rcd.rc04_qcke_drive = DRIVE_VERYSTRONG;
            endcase
         end

         RC05:
         begin
            case (da[1:0])
               2'b00: rcd.rc05_clka_drive = DRIVE_LIGHT;
               2'b01: rcd.rc05_clka_drive = DRIVE_MODERATE;
               2'b10: rcd.rc05_clka_drive = DRIVE_STRONG;
               2'b11: rcd.rc05_clka_drive = DRIVE_VERYSTRONG;
            endcase
            case (da[3:2])
               2'b00: rcd.rc05_clkb_drive = DRIVE_LIGHT;
               2'b01: rcd.rc05_clkb_drive = DRIVE_MODERATE;
               2'b10: rcd.rc05_clkb_drive = DRIVE_STRONG;
               2'b11: rcd.rc05_clkb_drive = DRIVE_VERYSTRONG;
            endcase
         end

         RC08:
         begin
            case (da[1:0])
               2'b00: rcd.rc08_qxc_outputs = QXC_C_EN_111;
               2'b01: rcd.rc08_qxc_outputs = QXC_C_EN_011;
               2'b10: rcd.rc08_qxc_outputs = QXC_C_EN_001;
               2'b11: rcd.rc08_qxc_outputs = QXC_C_EN_000;
            endcase

            rcd.rc08_qpar_outputs = (da[2] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc08_a17          = (da[3] == 1'b0) ? ENABLED : DISABLED;
         end

         RC09:
         begin
            rcd.rc09_dcs1_ibt    = (da[0] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc09_dcs1        = (da[1] == 1'b0) ? ENABLED : DISABLED;
            rcd.rc09_cke_pd_mode = (da[2] == 1'b0) ? CKE_POWERDOWN_IBT_ON : CKE_POWERDOWN_IBT_OFF;
            rcd.rc09_cke_pd      = (da[3] == 1'b0) ? DISABLED : ENABLED;
         end

         RC0A:
         begin
            case (da[2:0])
               3'b000: rcd.rc0a_rdimm_speed = RDIMM_SPEED_UP_TO_1600;
               3'b001: rcd.rc0a_rdimm_speed = RDIMM_SPEED_1601_TO_1867;
               3'b010: rcd.rc0a_rdimm_speed = RDIMM_SPEED_1868_TO_2134;
               3'b011: rcd.rc0a_rdimm_speed = RDIMM_SPEED_2135_TO_2400;
               3'b100: rcd.rc0a_rdimm_speed = RDIMM_SPEED_2401_TO_2667;
               3'b101: rcd.rc0a_rdimm_speed = RDIMM_SPEED_2668_TO_3200;
               3'b110: rcd.rc0a_rdimm_speed = RDIMM_SPEED_RESERVED;
               3'b111: rcd.rc0a_rdimm_speed = RDIMM_SPEED_PLL_BYPASS;
            endcase
            rcd.rc0a_context = (da[3] == 1'b0) ? CONTEXT_1 : CONTEXT_2;
         end

         RC0B:
         begin
            rcd.rc0b_vdd        = (da[0] == 1'b0) ? VDD_1P2V : VDD_RESERVED_LOWER;
            case (da[2:1])
               2'b00: rcd.rc0b_vrefca_src = QVREFCA_VDD_HALF_BVREFCA_VDD_HALF;
               2'b01: rcd.rc0b_vrefca_src = QVREFCA_VREF_INT_BVREFCA_VDD_HALF;
               2'b10: rcd.rc0b_vrefca_src = QVREFCA_VDD_HALF_BVREFCA_VREF_INT;
               2'b11: rcd.rc0b_vrefca_src = QVREFCA_EXTERNAL_BVREFCA_EXTERNAL;
            endcase
            rcd.rc0b_vrefin_src = (da[3] == 1'b0) ? RX_VREFIN_SRC_INTERNAL : RX_VREFIN_SRC_EXTERNAL;
         end

         RC0C:
         begin
            case (da[2:0])
               3'b000: rcd.rc0c_training_mode = NORMAL_MODE;
               3'b001: rcd.rc0c_training_mode = CLOCK_TO_CA_TRAINING_MODE;
               3'b010: rcd.rc0c_training_mode = DCS0N_LOOPBACK_MODE;
               3'b011: rcd.rc0c_training_mode = DCS1N_LOOPBACK_MODE;
               3'b100: rcd.rc0c_training_mode = DCKE0_LOOPBACK_MODE;
               3'b101: rcd.rc0c_training_mode = DCKE1_LOOPBACK_MODE;
               3'b110: rcd.rc0c_training_mode = DODT0_LOOPBACK_MODE;
               3'b111: rcd.rc0c_training_mode = DODT1_LOOPBACK_MODE;
            endcase
         end

         RC0D:
         begin
            case (da[1:0])
               2'b00: rcd.rc0d_cs_mode = DIRECT_DUALCS_MODE;
               2'b01: rcd.rc0d_cs_mode = DIRECT_QUADCS_MODE;
               2'b11: rcd.rc0d_cs_mode = ENCODED_QUADCS_MODE;
               default: $error("Invalid setting for RC0D CS Mode");
            endcase
            rcd.rc0d_dimm     = (da[2] == 1'b0) ? LRDIMM : RDIMM;
            rcd.rc0d_mrs_mirr = (da[3] == 1'b0) ? DISABLED : ENABLED;
         end

         RC0E:
         begin
            rcd.rc0e_parity         = (da[0] == 1'b0) ? DISABLED : ENABLED;
            rcd.rc0e_alert_assert   = (da[2] == 1'b0) ? ALERT_N_STICKY : ALERT_N_PULSE;
            rcd.rc0e_alert_reenable = (da[3] == 1'b0) ? ALERT_N_REENABLE_OFF : ALERT_N_REENABLE_ON;
         end

         RC0F:
         begin
            case (da[2:0])
               3'b000: rcd.rc0f_latency_adder = LATENCY_1NCK;
               3'b001: rcd.rc0f_latency_adder = LATENCY_2NCK;
               3'b010: rcd.rc0f_latency_adder = LATENCY_3NCK;
               3'b011: rcd.rc0f_latency_adder = LATENCY_4NCK;
               3'b100: rcd.rc0f_latency_adder = LATENCY_0NCK;
               default: $error("Reserved setting for RC0F Latency Adder Mode");
            endcase
         end

         RC1X:
         begin
            case (da[7:0])
               8'b0000_0000: rcd.rc1x_vrefca_int = VREFCA_INT_50P00;
               8'b0000_0001: rcd.rc1x_vrefca_int = VREFCA_INT_50P83;
               8'b0000_0010: rcd.rc1x_vrefca_int = VREFCA_INT_51P67;
               8'b0000_0011: rcd.rc1x_vrefca_int = VREFCA_INT_52P60;
               8'b0000_0100: rcd.rc1x_vrefca_int = VREFCA_INT_53P33;
               8'b0000_0101: rcd.rc1x_vrefca_int = VREFCA_INT_54P17;
               8'b0000_0110: rcd.rc1x_vrefca_int = VREFCA_INT_55P00;
               8'b0000_0111: rcd.rc1x_vrefca_int = VREFCA_INT_55P83;
               8'b0000_1000: rcd.rc1x_vrefca_int = VREFCA_INT_56P67;
               8'b0000_1001: rcd.rc1x_vrefca_int = VREFCA_INT_57P50;
               8'b0000_1010: rcd.rc1x_vrefca_int = VREFCA_INT_58P33;
               8'b0000_1011: rcd.rc1x_vrefca_int = VREFCA_INT_59P17;
               8'b0000_1100: rcd.rc1x_vrefca_int = VREFCA_INT_60P00;
               8'b0000_1101: rcd.rc1x_vrefca_int = VREFCA_INT_60P83;
               8'b0000_1110: rcd.rc1x_vrefca_int = VREFCA_INT_61P67;
               8'b0000_1111: rcd.rc1x_vrefca_int = VREFCA_INT_62P50;
               8'b0001_0000: rcd.rc1x_vrefca_int = VREFCA_INT_63P33;
               8'b0001_0001: rcd.rc1x_vrefca_int = VREFCA_INT_64P17;
               8'b0001_0010: rcd.rc1x_vrefca_int = VREFCA_INT_65P00;
               8'b0001_0011: rcd.rc1x_vrefca_int = VREFCA_INT_65P83;
               8'b0001_0100: rcd.rc1x_vrefca_int = VREFCA_INT_66P67;

               8'b0010_1100: rcd.rc1x_vrefca_int = VREFCA_INT_33P33;
               8'b0010_1101: rcd.rc1x_vrefca_int = VREFCA_INT_34P17;
               8'b0010_1110: rcd.rc1x_vrefca_int = VREFCA_INT_35P00;
               8'b0010_1111: rcd.rc1x_vrefca_int = VREFCA_INT_35P83;
               8'b0011_0000: rcd.rc1x_vrefca_int = VREFCA_INT_36P67;
               8'b0011_0001: rcd.rc1x_vrefca_int = VREFCA_INT_37P50;
               8'b0011_0010: rcd.rc1x_vrefca_int = VREFCA_INT_38P33;
               8'b0011_0011: rcd.rc1x_vrefca_int = VREFCA_INT_39P17;
               8'b0011_0100: rcd.rc1x_vrefca_int = VREFCA_INT_40P00;
               8'b0011_0101: rcd.rc1x_vrefca_int = VREFCA_INT_40P83;
               8'b0011_0110: rcd.rc1x_vrefca_int = VREFCA_INT_41P67;
               8'b0011_0111: rcd.rc1x_vrefca_int = VREFCA_INT_42P50;
               8'b0011_1000: rcd.rc1x_vrefca_int = VREFCA_INT_43P33;
               8'b0011_1001: rcd.rc1x_vrefca_int = VREFCA_INT_44P17;
               8'b0011_1010: rcd.rc1x_vrefca_int = VREFCA_INT_45P00;
               8'b0011_1011: rcd.rc1x_vrefca_int = VREFCA_INT_45P83;
               8'b0011_1100: rcd.rc1x_vrefca_int = VREFCA_INT_46P67;
               8'b0011_1101: rcd.rc1x_vrefca_int = VREFCA_INT_47P50;
               8'b0011_1110: rcd.rc1x_vrefca_int = VREFCA_INT_48P33;
               8'b0011_1111: rcd.rc1x_vrefca_int = VREFCA_INT_49P17;

               default: $error("Reserved setting for RC1X Internal VrefCA");

            endcase
         end

         RC3X:
         begin
            case (da[6:0])
               7'b000_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1241_TO_1260;
               7'b000_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1261_TO_1280;
               7'b000_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1281_TO_1300;
               7'b000_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1301_TO_1320;
               7'b000_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1321_TO_1340;
               7'b000_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1341_TO_1360;
               7'b000_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1361_TO_1380;
               7'b000_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1381_TO_1400;
               7'b000_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1401_TO_1420;
               7'b000_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1421_TO_1440;
               7'b000_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1441_TO_1460;
               7'b000_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1461_TO_1480;
               7'b000_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1481_TO_1500;
               7'b000_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1501_TO_1520;
               7'b000_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1521_TO_1540;
               7'b000_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1541_TO_1560;
               7'b001_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1561_TO_1580;
               7'b001_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1581_TO_1600;
               7'b001_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1601_TO_1620;
               7'b001_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1621_TO_1640;
               7'b001_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1641_TO_1660;
               7'b001_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1661_TO_1680;
               7'b001_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1681_TO_1700;
               7'b001_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1701_TO_1720;
               7'b001_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1721_TO_1740;
               7'b001_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1741_TO_1760;
               7'b001_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1761_TO_1780;
               7'b001_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1781_TO_1800;
               7'b001_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1801_TO_1820;
               7'b001_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1821_TO_1840;
               7'b001_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1841_TO_1860;
               7'b001_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1861_TO_1880;
               7'b010_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1881_TO_1900;
               7'b010_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1901_TO_1920;
               7'b010_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1921_TO_1940;
               7'b010_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1941_TO_1960;
               7'b010_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1961_TO_1980;
               7'b010_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_1981_TO_2000;
               7'b010_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2001_TO_2020;
               7'b010_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2021_TO_2040;
               7'b010_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2041_TO_2060;
               7'b010_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2061_TO_2080;
               7'b010_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2081_TO_2100;
               7'b010_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2101_TO_2120;
               7'b010_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2121_TO_2140;
               7'b010_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2141_TO_2160;
               7'b010_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2161_TO_2180;
               7'b010_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2181_TO_2200;
               7'b011_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2201_TO_2220;
               7'b011_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2221_TO_2240;
               7'b011_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2241_TO_2260;
               7'b011_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2261_TO_2280;
               7'b011_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2281_TO_2300;
               7'b011_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2301_TO_2320;
               7'b011_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2321_TO_2340;
               7'b011_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2341_TO_2360;
               7'b011_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2361_TO_2380;
               7'b011_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2381_TO_2400;
               7'b011_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2401_TO_2420;
               7'b011_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2421_TO_2440;
               7'b011_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2441_TO_2460;
               7'b011_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2461_TO_2480;
               7'b011_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2481_TO_2500;
               7'b011_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2501_TO_2520;
               7'b100_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2521_TO_2540;
               7'b100_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2541_TO_2560;
               7'b100_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2561_TO_2580;
               7'b100_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2581_TO_2600;
               7'b100_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2601_TO_2620;
               7'b100_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2621_TO_2640;
               7'b100_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2641_TO_2660;
               7'b100_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2661_TO_2680;
               7'b100_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2681_TO_2700;
               7'b100_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2701_TO_2720;
               7'b100_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2721_TO_2740;
               7'b100_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2741_TO_2760;
               7'b100_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2761_TO_2780;
               7'b100_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2781_TO_2800;
               7'b100_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2801_TO_2820;
               7'b100_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2821_TO_2840;
               7'b101_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2841_TO_2860;
               7'b101_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2861_TO_2880;
               7'b101_0010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2881_TO_2900;
               7'b101_0011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2901_TO_2920;
               7'b101_0100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2921_TO_2940;
               7'b101_0101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2941_TO_2960;
               7'b101_0110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2961_TO_2980;
               7'b101_0111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_2981_TO_3000;
               7'b101_1000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3001_TO_3020;
               7'b101_1001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3021_TO_3040;
               7'b101_1010: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3041_TO_3060;
               7'b101_1011: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3061_TO_3080;
               7'b101_1100: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3081_TO_3100;
               7'b101_1101: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3101_TO_3120;
               7'b101_1110: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3121_TO_3140;
               7'b101_1111: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3141_TO_3160;
               7'b110_0000: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3161_TO_3180;
               7'b110_0001: rcd.rc3x_rdimm_fine_speed = RDIMM_SPEED_FINE_3181_TO_3200;
               default: $error("Reserved setting for RC3X RDIMM Fine Speed");
            endcase
         end

         RC5X:
         begin
            case (da[1:0])
               2'b00: rcd.rc5x_wr_odt_addition = ODT_ADDITION_OFF;
               2'b01: rcd.rc5x_wr_odt_addition = ODT_ADDITION_1CYC;
               2'b10: rcd.rc5x_wr_odt_addition = ODT_ADDITION_2CYC;
               default: $error("Reserved setting for RC5X Write ODT Addition");
            endcase
            case (da[3:2])
               2'b00: rcd.rc5x_rd_odt_addition = ODT_ADDITION_OFF;
               2'b01: rcd.rc5x_rd_odt_addition = ODT_ADDITION_1CYC;
               2'b10: rcd.rc5x_rd_odt_addition = ODT_ADDITION_2CYC;
               default: $error("Reserved setting for RC5X Read ODT Addition");
            endcase
         end

         RC7X:
         begin
            case (da[1:0])
               2'b00: rcd.rc7x_ca_ibt = IBT_100OHM;
               2'b01: rcd.rc7x_ca_ibt = IBT_150OHM;
               2'b10: rcd.rc7x_ca_ibt = IBT_300OHM;
               2'b11: rcd.rc7x_ca_ibt = IBT_OFF;
            endcase
            case (da[3:2])
               2'b00: rcd.rc7x_dcs_ibt = IBT_100OHM;
               2'b01: rcd.rc7x_dcs_ibt = IBT_150OHM;
               2'b10: rcd.rc7x_dcs_ibt = IBT_300OHM;
               2'b11: rcd.rc7x_dcs_ibt = IBT_OFF;
            endcase
            case (da[5:4])
               2'b00: rcd.rc7x_dcke_ibt = IBT_100OHM;
               2'b01: rcd.rc7x_dcke_ibt = IBT_150OHM;
               2'b10: rcd.rc7x_dcke_ibt = IBT_300OHM;
               2'b11: rcd.rc7x_dcke_ibt = IBT_OFF;
            endcase
            case (da[7:6])
               2'b00: rcd.rc7x_dodt_ibt = IBT_100OHM;
               2'b01: rcd.rc7x_dodt_ibt = IBT_150OHM;
               2'b10: rcd.rc7x_dodt_ibt = IBT_300OHM;
               2'b11: rcd.rc7x_dodt_ibt = IBT_OFF;
            endcase
         end

         RC8X:
         begin
            rcd.rc8x_odt_wr_timing = (da[0] == 1'b0) ? ODT_WR_TIMING_0CYC : ODT_WR_TIMING_1CYC;
            case (da[4:1])
               4'b0000: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_0CYC;
               4'b0001: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_1CYC;
               4'b0010: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_2CYC;
               4'b0011: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_3CYC;
               4'b0100: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_4CYC;
               4'b0101: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_5CYC;
               4'b0110: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_6CYC;
               4'b0111: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_7CYC;
               4'b1000: rcd.rc8x_odt_rd_timing = ODT_RD_TIMING_8CYC;
               default: $error("Reserved setting for RC8X QODT Read Timing");
            endcase
            rcd.rc8x_bodt_outputs = (da[5] == 1'b0) ? ENABLED : DISABLED;
            case (da[7:6])
               2'b00: rcd.rc8x_qodt_ctrl = QODT_CTRL_DODT01;
               2'b01: rcd.rc8x_qodt_ctrl = QODT_CTRL_DODT0;
               2'b10: rcd.rc8x_qodt_ctrl = QODT_CTRL_OFF;
               2'b11: rcd.rc8x_qodt_ctrl = QODT_CTRL_INTERNAL;
            endcase
         end

         RC9X:
         begin
            rcd.rc9x_wr_qodt0_r0 = (da[0] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt1_r0 = (da[1] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt0_r1 = (da[2] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt1_r1 = (da[3] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt0_r2 = (da[4] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt1_r2 = (da[5] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt0_r3 = (da[6] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rc9x_wr_qodt1_r3 = (da[7] == 1'b0) ? QODT_OFF : QODT_ON;
         end

         RCAX:
         begin
            rcd.rcax_rd_qodt0_r0 = (da[0] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt1_r0 = (da[1] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt0_r1 = (da[2] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt1_r1 = (da[3] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt0_r2 = (da[4] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt1_r2 = (da[5] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt0_r3 = (da[6] == 1'b0) ? QODT_OFF : QODT_ON;
            rcd.rcax_rd_qodt1_r3 = (da[7] == 1'b0) ? QODT_OFF : QODT_ON;
         end

         RCBX:
         begin
            rcd.rcbx_dc0_ibt              = (da[0] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_dc1_ibt              = (da[1] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_dc2_ibt              = (da[2] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_ddr4db01_mrs_snoop   = (da[3] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_ddr4rcd01_mrs_snoop  = (da[4] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_dcke1_ibt            = (da[5] == 1'b0) ? ENABLED : DISABLED;
            rcd.rcbx_dcke1                = (da[6] == 1'b0) ? ENABLED : DISABLED;
         end

      endcase

      print_rc(rc);
   endtask

   initial begin
      parity_alert_n_pipeline = '1;
      expected_parity = 1'b0;
      parity_cmp_en = 1'b0;
   end

endmodule

