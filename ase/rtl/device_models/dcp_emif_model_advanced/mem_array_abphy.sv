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




`define _abphy_get_pin_count(_loc) ( _loc[ 9 : 0 ] )
`define _abphy_get_pin_index(_loc, _port_i) ( _loc[ (_port_i + 1) * 10 +: 10 ] )

`define _abphy_get_tile(_loc, _port_i) (  `_abphy_get_pin_index(_loc, _port_i) / (PINS_PER_LANE * LANES_PER_TILE) )
`define _abphy_get_lane(_loc, _port_i) ( (`_abphy_get_pin_index(_loc, _port_i) / PINS_PER_LANE) % LANES_PER_TILE )
`define _abphy_get_pin(_loc, _port_i)  (  `_abphy_get_pin_index(_loc, _port_i) % PINS_PER_LANE )

`define _abphy_sel_hmc_val(_tile_i, _pri, _sec)             ( PHY_PING_PONG_EN ? (_tile_i <= SEC_AC_TILE_INDEX ? _sec : _pri) : _pri )


`define _abphy_connect_to_abphy(_loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port) \
   for (port_i = 0; port_i < _mem_port_width; ++port_i) begin : aa \
      for (phase_i = 0; phase_i < _num_of_phases; ++phase_i) begin : bb \
        assign _abphy_port[phase_i][port_i]  =  _tile_data_port[ (`_abphy_get_pin_index(_loc, port_i)*8)+phase_i]; \
      end \
   end


`define _abphy_connect_to_abphy_oe(_loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port ) \
   for (port_i = 0; port_i < _mem_port_width; ++port_i) begin : a \
      for (phase_i = 0; phase_i < _num_of_phases; ++phase_i) begin : b \
        assign _abphy_port[phase_i][port_i]  =  _tile_data_port[ (`_abphy_get_pin_index(_loc, port_i)*4)+phase_i]; \
      end \
   end

`define _abphy_connect_to_abphy_oe2(_loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port ) \
   for (port_i = 0; port_i < _mem_port_width; ++port_i) begin : c \
      for (phase_i = 0; phase_i < _num_of_phases; ++phase_i) begin : d \
        assign _abphy_port[phase_i][port_i]  =  _tile_data_port[ (`_abphy_get_pin_index(_loc, port_i)*4)+phase_i]; \
      end \
   end

`define _abphy_connect_from_abphy(_loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port) \
   for (port_i = 0; port_i < _mem_port_width; ++port_i) begin : e \
      for (phase_i = 0; phase_i < _num_of_phases; ++phase_i) begin : f \
        assign _tile_data_port[ (`_abphy_get_pin_index(_loc, port_i)*8)+phase_i] = _abphy_port[phase_i][port_i]; \
      end \
   end
`define _abphy_connect_from_abphy2(_loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port) \
   for (port_i = 0; port_i < _mem_port_width; ++port_i) begin : g \
      for (phase_i = 0; phase_i < _num_of_phases; ++phase_i) begin : h \
        assign _tile_data_port[ (`_abphy_get_pin_index(_loc, port_i)*8)+phase_i] = _abphy_port[phase_i][port_i]; \
      end \
   end

`define _abphy_connect_to_abphy_debug( _loc, _mem_port_width, _num_of_phases, _abphy_port, _tile_data_port) \
  initial begin \
   for (i = 0; i < _mem_port_width; ++i) begin  \
      for (j = 0; j < _num_of_phases; ++j) begin  \
        //$display( "abphy %d/%d connected to tile=%-d,pin=%-d",i,j,`_abphy_get_tile(_loc,i),`_abphy_get_pin_index(_loc,i)%48); \
      end \
   end \
  end

`define _get_pin_ddr_raw(_tile_i, _lane_i, _pin_i)       ( PINS_RATE[_tile_i * LANES_PER_TILE * PINS_PER_LANE + _lane_i * PINS_PER_LANE + _pin_i] )
`define _get_pin_ddr_str(_tile_i, _lane_i, _pin_i)       ( `_get_pin_ddr_raw(_tile_i, _lane_i, _pin_i) == PIN_RATE_DDR ? "mode_ddr" : "mode_sdr" )
`define _get_pin_ddr_str_wrap(_loc, _port_i)             (`_get_pin_ddr_str(`_abphy_get_tile(_loc,_port_i), `_abphy_get_lane(_loc,_port_i), `_abphy_get_pin(_loc,_port_i)))

module mem_array_abphy # (
  parameter NUM_OF_RTL_TILES        = 1,
  parameter LANES_PER_TILE          = 4,
  parameter PINS_PER_LANE           = 12,
  parameter PINS_RATE               = 1'b0,
  parameter MEM_DATA_MASK_EN        = 1,
  parameter USER_CLK_RATIO          = 1,
  parameter PHY_HMC_CLK_RATIO       = 1,
  parameter NUM_OF_HMC_PORTS        = 0,
  parameter PORT_MEM_A_PINLOC       = 0,
  parameter PORT_MEM_BA_PINLOC      = 0,
  parameter PORT_MEM_BG_PINLOC      = 0,
  parameter PORT_MEM_CS_N_PINLOC    = 0,
  parameter PORT_MEM_ACT_N_PINLOC   = 0,
  parameter PORT_MEM_DQ_PINLOC      = 0,
  parameter PORT_MEM_DM_PINLOC      = 0,
  parameter PORT_MEM_DBI_N_PINLOC   = 0,
  parameter PORT_MEM_RAS_N_PINLOC   = 0,
  parameter PORT_MEM_CAS_N_PINLOC   = 0,
  parameter PORT_MEM_WE_N_PINLOC    = 0,
  parameter PORT_MEM_REF_N_PINLOC   = 0,
  parameter PORT_MEM_WPS_N_PINLOC   = 0,
  parameter PORT_MEM_RPS_N_PINLOC   = 0,
  parameter PORT_MEM_BWS_N_PINLOC   = 0,
  parameter PORT_MEM_DQA_PINLOC     = 0,
  parameter PORT_MEM_DQB_PINLOC     = 0,
  parameter PORT_MEM_Q_PINLOC       = 0,
  parameter PORT_MEM_D_PINLOC       = 0,
  parameter PORT_MEM_RWA_N_PINLOC   = 0,
  parameter PORT_MEM_RWB_N_PINLOC   = 0,
  parameter PORT_MEM_QKA_PINLOC     = 0,
  parameter PORT_MEM_QKB_PINLOC     = 0,
  parameter PORT_MEM_LDA_N_PINLOC   = 0,
  parameter PORT_MEM_LDB_N_PINLOC   = 0,
  parameter PORT_MEM_CK_PINLOC      = 0,
  parameter PORT_MEM_DINVA_PINLOC   = 0,
  parameter PORT_MEM_DINVB_PINLOC   = 0,
  parameter PORT_MEM_AINV_PINLOC    = 0,
  parameter PORT_MEM_DQ_WIDTH       = 0,
  parameter PORT_MEM_A_WIDTH        = 0,
  parameter PORT_MEM_BA_WIDTH       = 0,
  parameter PORT_MEM_BG_WIDTH       = 0,
  parameter PORT_MEM_CS_N_WIDTH     = 0,
  parameter PORT_MEM_ACT_N_WIDTH    = 0,
  parameter PORT_MEM_DBI_N_WIDTH    = 0,
  parameter PORT_MEM_RAS_N_WIDTH    = 0,
  parameter PORT_MEM_CAS_N_WIDTH    = 0,
  parameter PORT_MEM_WE_N_WIDTH     = 0,
  parameter PORT_MEM_DM_WIDTH       = 0,
  parameter PORT_MEM_REF_N_WIDTH    = 0,
  parameter PORT_MEM_WPS_N_WIDTH    = 0,
  parameter PORT_MEM_RPS_N_WIDTH    = 0,
  parameter PORT_MEM_BWS_N_WIDTH    = 0,
  parameter PORT_MEM_DQA_WIDTH      = 0,
  parameter PORT_MEM_DQB_WIDTH      = 0,
  parameter PORT_MEM_Q_WIDTH        = 0,
  parameter PORT_MEM_D_WIDTH        = 0,
  parameter PORT_MEM_RWA_N_WIDTH    = 0,
  parameter PORT_MEM_RWB_N_WIDTH    = 0,
  parameter PORT_MEM_QKA_WIDTH      = 0,
  parameter PORT_MEM_QKB_WIDTH      = 0,
  parameter PORT_MEM_LDA_N_WIDTH    = 0,
  parameter PORT_MEM_LDB_N_WIDTH    = 0,
  parameter PORT_MEM_CK_WIDTH       = 0,
  parameter PORT_MEM_DINVA_WIDTH    = 0,
  parameter PORT_MEM_DINVB_WIDTH    = 0,
  parameter PORT_MEM_AINV_WIDTH     = 0,
  parameter PHY_PING_PONG_EN        = 0,
  parameter PROTOCOL_ENUM           = "",
  parameter DBI_WR_ENABLE           = "",
  parameter DBI_RD_ENABLE           = "",
  parameter PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH  = "",
  parameter PRI_HMC_CFG_MEM_IF_ROWADDR_WIDTH  = "",
  parameter SEC_HMC_CFG_MEM_IF_COLADDR_WIDTH  = "",
  parameter SEC_HMC_CFG_MEM_IF_ROWADDR_WIDTH  = "",
  parameter MEM_BURST_LENGTH        = 8,
  parameter ABPHY_WRITE_PROTOCOL    = 1

) (
  input                                                                        phy_clk,
  input                                                                        reset_n,
  input      [12*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]                          select_ac_hmc,
  input      [96*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]                          ac_hmc,
  input      [96*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]                          dq_data_to_mem,
  input      [48*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]                          dq_oe,
  input                                                                        afi_cal_success,
  input                                                                        mem_rd_req,
  input      [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                   afi_wlat,
  input      [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0][5:0]                   afi_rlat,
  output reg [96*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]                          dq_data_from_mem,
  output reg [3:0]                                                             rdata_valid_local [NUM_OF_RTL_TILES-1:0][LANES_PER_TILE-1:0],
  input                                                                        runAbstractPhySim
);

   timeunit 1ns;
   timeprecision 1ps;

   typedef enum bit [0:0] {
      PIN_RATE_DDR       = 1'b0,
      PIN_RATE_SDR       = 1'b1
   } PIN_RATE;

  parameter [4:0]
    pCMD_ACTIVE             =  (PROTOCOL_ENUM == "PROTOCOL_DDR4") ?   5'b00xxx :
                              ((PROTOCOL_ENUM == "PROTOCOL_DDR3") ? 5'b00110 :
                              ((PROTOCOL_ENUM == "PROTOCOL_LPDDR3") ? 5'b001x0 :
                              5'b00011 )),

    pCMD_WRITE              =  (PROTOCOL_ENUM == "PROTOCOL_DDR4") ?   5'b01100 :
                              ((PROTOCOL_ENUM == "PROTOCOL_DDR3"|| PROTOCOL_ENUM == "PROTOCOL_LPDDR3") ? 5'b01000 :
                              ((PROTOCOL_ENUM == "PROTOCOL_RLD3" ) ? 5'b00100 :
                              ((PROTOCOL_ENUM == "PROTOCOL_QDR2") ? 5'b01000 :
                              ((PROTOCOL_ENUM == "PROTOCOL_QDR4") ? 5'b00000 :
                               5'd0 )))),

    pCMD_READ               =  (PROTOCOL_ENUM == "PROTOCOL_DDR4") ?   5'b01101 :
                              ((PROTOCOL_ENUM == "PROTOCOL_DDR3" || PROTOCOL_ENUM == "PROTOCOL_LPDDR3") ? 5'b01010 :
                              ((PROTOCOL_ENUM == "PROTOCOL_RLD3") ? 5'b01100 :
                              ((PROTOCOL_ENUM == "PROTOCOL_QDR2") ? 5'b10000 :
                              ((PROTOCOL_ENUM == "PROTOCOL_QDR4") ? 5'b01000 :
                               5'd0 )))),
    pCMD_WRITE_N_READ       = PROTOCOL_ENUM == "PROTOCOL_QDR2" ? 5'b00000 : 5'b10101;


  parameter
    pNO_RD_DATA               = 'd0,
    pCURRENT_RD_DATA          = 'd1,
    pDELAY_1_RD_DATA          = 'd2,
    pDELAY_2_RD_DATA          = 'd3;

  parameter
    pWS_IDLE                = 'd0,
    pWS_WR_N_CHECK_DATA     = 'd1,
    pWS_BUILD_DATA          = 'd2;

  parameter
    pRS_IDLE                = 'd0,
    pRS_READING             = 'd1,
    pRS_READ_UPPER          = 'd2,
    pRS_READ_BOTH           = 'd3;

  localparam pLOCAL_DM_WIDTH     = PROTOCOL_ENUM == "PROTOCOL_DDR4" ? PORT_MEM_DBI_N_WIDTH :
                                   PROTOCOL_ENUM == "PROTOCOL_QDR2" ? PORT_MEM_BWS_N_WIDTH :
                                      PORT_MEM_DM_WIDTH;
  localparam pLOCAL_D_WIDTH      = PROTOCOL_ENUM == "PROTOCOL_QDR2" ? PORT_MEM_D_WIDTH  :
                                      PROTOCOL_ENUM == "PROTOCOL_QDR4" ? (PORT_MEM_DQA_WIDTH+PORT_MEM_DQB_WIDTH) :
                                      PORT_MEM_DQ_WIDTH;
  localparam pLOCAL_Q_WIDTH      = PROTOCOL_ENUM == "PROTOCOL_QDR2" ? PORT_MEM_Q_WIDTH  :
                                      PROTOCOL_ENUM == "PROTOCOL_QDR4" ? (PORT_MEM_DQA_WIDTH+PORT_MEM_DQB_WIDTH) :
                                      PORT_MEM_DQ_WIDTH;

  localparam LOCAL_ADDR_WIDTH    = PROTOCOL_ENUM == "PROTOCOL_LPDDR3" ? 16 : PORT_MEM_A_WIDTH;


  localparam pWRITE_PATH_LAT                = 7;
  localparam pWRITE_PATH_LAT_INDEX          = pWRITE_PATH_LAT-1;
  localparam pNUM_OF_INTF                   = PROTOCOL_ENUM == "PROTOCOL_QDR4" ? 2 : PHY_PING_PONG_EN+1;
  localparam pNUM_OF_CHANS                  = PROTOCOL_ENUM == "PROTOCOL_QDR4" ? 2 : PORT_MEM_CS_N_WIDTH;
  localparam PORT_MEM_DQ_WIDTH_PER_INTF     = pLOCAL_D_WIDTH/pNUM_OF_INTF;
  localparam PORT_MEM_DBI_N_WIDTH_PER_INTF  = (pLOCAL_DM_WIDTH/pNUM_OF_INTF)>=1 ? pLOCAL_DM_WIDTH/pNUM_OF_INTF : 1;


  localparam pDM_MASK_LEN        = PROTOCOL_ENUM == "PROTOCOL_RLD3" ? 9 : PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF;
  localparam pBANK_WIDTH         = 4;
  localparam pCMD_WIDTH          = 5;
  localparam pAC_PIPE_ADDR_WIDTH = (2*LOCAL_ADDR_WIDTH) + pBANK_WIDTH;
  localparam pAC_PIPE_WIDTH      = pAC_PIPE_ADDR_WIDTH  + 2;
  localparam pWR_MEM_PIPE_WIDTH  = pAC_PIPE_ADDR_WIDTH  + 1  +(PORT_MEM_DQ_WIDTH_PER_INTF*8)+(PORT_MEM_DBI_N_WIDTH_PER_INTF*8);
  localparam pADDR_PIN_RATE      = `_get_pin_ddr_str_wrap(PORT_MEM_A_PINLOC, (PORT_MEM_A_WIDTH/2));
  localparam pADDR_NUM_OF_PHASES = (`_get_pin_ddr_str_wrap(PORT_MEM_A_PINLOC, (PORT_MEM_A_WIDTH/2))=="mode_ddr"? 8:4);


  reg [pCMD_WIDTH-1:0]                            cmd_d                 [pNUM_OF_CHANS-1:0][3:0];
  reg [PORT_MEM_A_WIDTH-1:0]                      addr_d                [7:0];
  reg [pBANK_WIDTH-1:0]                           bank_d                [3:0];
  reg [pLOCAL_D_WIDTH-1:0]                        data_d                [7:0];
  reg [pLOCAL_DM_WIDTH-1:0]                       data_dm_d             [7:0];
  reg [pLOCAL_DM_WIDTH-1:0]                       rd_dbi_d              [7:0];
  reg [PORT_MEM_DINVA_WIDTH-1:0]                  rd_dinva_d            [7:0];
  reg [PORT_MEM_DINVB_WIDTH-1:0]                  rd_dinvb_d            [7:0];
  reg [pLOCAL_D_WIDTH-1:0]                        oe_d                  [3:0];
  reg [pLOCAL_D_WIDTH-1:0]                        oe_a_d                [3:0];
  reg [pLOCAL_D_WIDTH-1:0]                        oe_b_d                [3:0];
  reg [PORT_MEM_BA_WIDTH-1:0]                     bank_a_d              [3:0];
  reg [PORT_MEM_BG_WIDTH-1:0]                     bank_g_d              [3:0];
  reg [PORT_MEM_CS_N_WIDTH-1:0]                   cs_n_d                [3:0];
  reg [PORT_MEM_ACT_N_WIDTH-1:0]                  act_n_d               [3:0];
  reg [PORT_MEM_RAS_N_WIDTH-1:0]                  ras_n_d               [3:0];
  reg [PORT_MEM_CAS_N_WIDTH-1:0]                  cas_n_d               [3:0];
  reg [PORT_MEM_WE_N_WIDTH-1:0]                   we_n_d                [3:0];
  reg [PORT_MEM_REF_N_WIDTH-1:0]                  ref_n_d               [3:0];
  reg [PORT_MEM_WPS_N_WIDTH-1:0]                  wps_n_d               [3:0];
  reg [PORT_MEM_RPS_N_WIDTH-1:0]                  rps_n_d               [3:0];
  reg [PORT_MEM_DQA_WIDTH-1:0]                    dqa_d                 [7:0];
  reg [PORT_MEM_DQB_WIDTH-1:0]                    dqb_d                 [7:0];
  reg [PORT_MEM_RWA_N_WIDTH-1:0]                  rwa_n_d               [7:0];
  reg [PORT_MEM_RWB_N_WIDTH-1:0]                  rwb_n_d               [7:0];
  reg [PORT_MEM_LDA_N_WIDTH-1:0]                  lda_n_d               [7:0];
  reg [PORT_MEM_LDB_N_WIDTH-1:0]                  ldb_n_d               [7:0];
  reg [PORT_MEM_CK_WIDTH-1:0]                     ck_d                  [7:0];
  reg [PORT_MEM_DINVA_WIDTH-1:0]                  dinva_d               [7:0];
  reg [PORT_MEM_DINVB_WIDTH-1:0]                  dinvb_d               [7:0];
  reg [PORT_MEM_AINV_WIDTH-1:0]                   ainv_d                [7:0];

  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            data_muxd_d           [pNUM_OF_INTF-1:0][7:0];
  reg [PORT_MEM_DBI_N_WIDTH_PER_INTF-1:0]         data_dm_muxd_d        [pNUM_OF_INTF-1:0][7:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            oe_muxd_d             [pNUM_OF_INTF-1:0][3:0];
  reg [LOCAL_ADDR_WIDTH-1:0]                      col_addr_d            [pADDR_NUM_OF_PHASES-1:0];


  reg [LOCAL_ADDR_WIDTH-1:0]                      row_addr              [15:0][pNUM_OF_CHANS-1:0];
  reg [LOCAL_ADDR_WIDTH-1:0]                      row_addr_d            [pNUM_OF_CHANS-1:0];
  reg [LOCAL_ADDR_WIDTH-1:0]                      lpddr3_row_addr_d     [3:0];


  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        mem                   [*];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        mem_temp;
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            mem_temp_array        [7:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            mem_temp_array_mask;

  reg                                             wr_WRITE_en           [pNUM_OF_CHANS-1:0][3:0];
  reg                                             wr_READ_en            [pNUM_OF_CHANS-1:0][3:0];
  reg                                             rd_en;
  reg [(2*LOCAL_ADDR_WIDTH)+pBANK_WIDTH-1:0]      wr_ac_data            [pNUM_OF_CHANS-1:0][3:0];
  reg [LOCAL_ADDR_WIDTH-1:0]                      wr_ac_data_col_addr   [pNUM_OF_CHANS-1:0][3:0];
  reg [LOCAL_ADDR_WIDTH-1:0]                      wr_ac_data_row_addr   [pNUM_OF_CHANS-1:0][3:0];
  reg [pBANK_WIDTH-1:0]                           wr_ac_data_bank       [pNUM_OF_CHANS-1:0][3:0];

  reg [pAC_PIPE_WIDTH-1:0]                        ac_cmd_pipeline [63:0][pNUM_OF_CHANS-1:0][3:0];
  reg                                             mem_wr_d              [pNUM_OF_INTF-1:0];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        mem_data_d            [pNUM_OF_INTF-1:0];
  reg [(PORT_MEM_DBI_N_WIDTH_PER_INTF*8)-1:0]     mem_data_dm_d         [pNUM_OF_INTF-1:0];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        rd_mem_data_d         [pNUM_OF_CHANS-1:0][3:0];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        rd_mem_data           [pNUM_OF_CHANS-1:0][3:0];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        rd_mem_data_r         [pNUM_OF_CHANS-1:0][3:0];
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        rd_mem_data_shifted_d [pNUM_OF_CHANS-1:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            rd_mem_data_shifted_debug_d [pNUM_OF_CHANS-1:0][7:0];
  reg [1:0]                                       rd_mem_shifted_en_d   [pNUM_OF_CHANS-1:0][3:0][7:0];
  reg [3:0]                                       rd_mem_shifted_element_d [pNUM_OF_CHANS-1:0][3:0][7:0];
  reg [3:0]                                       mem_rd_amt_sent_d     [pNUM_OF_CHANS-1:0][3:0];
  reg [3:0]                                       mem_rd_amt_sent       [pNUM_OF_CHANS-1:0][3:0];
  reg [1:0]                                       mem_rd_delay_d        [pNUM_OF_CHANS-1:0][3:0];
  reg [1:0]                                       mem_rd_delay          [pNUM_OF_CHANS-1:0][3:0];
  reg [3:0]                                       mem_rd_amt_prev_to_send_d [pNUM_OF_CHANS-1:0][3:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            mem_data              [pNUM_OF_INTF-1:0][7:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            mem_data_r1           [pNUM_OF_INTF-1:0][7:0];
  reg [PORT_MEM_DBI_N_WIDTH_PER_INTF-1:0]         mem_data_dm           [pNUM_OF_INTF-1:0][7:0];
  reg [PORT_MEM_DBI_N_WIDTH_PER_INTF-1:0]         mem_data_dm_r1        [pNUM_OF_INTF-1:0][7:0];
  reg                                             active_detected;
  reg                                             row_addr_d_en         [pNUM_OF_CHANS-1:0];
  reg [3:0]                                       active_bank;
  reg [pWR_MEM_PIPE_WIDTH-1:0]                    WR_mem_pipeline  [15:0][pNUM_OF_CHANS-1:0][3:0];
  reg [1:0]                                       mem_rd_state_d        [pNUM_OF_CHANS-1:0][3:0];
  reg [1:0]                                       mem_rd_state          [pNUM_OF_CHANS-1:0][3:0];
  reg [pAC_PIPE_ADDR_WIDTH-1:0]                   RD_mem_addr_d         [pNUM_OF_CHANS-1:0][3:0];
  reg                                             RD_req_d              [pNUM_OF_CHANS-1:0][3:0];
  reg                                             WR_req_d              [pNUM_OF_CHANS-1:0][3:0];
  reg [pAC_PIPE_ADDR_WIDTH-1:0]                   WR_mem_addr_d         [pNUM_OF_CHANS-1:0][3:0];
  reg                                             mem_wr_pipe           [pNUM_OF_CHANS-1:0][3:0];
  reg [pAC_PIPE_ADDR_WIDTH-1:0]                   mem_addr_pipe         [pNUM_OF_CHANS-1:0][3:0];
  reg [pAC_PIPE_ADDR_WIDTH-1:0]                   temp_addr;
  reg [(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:0]        mem_data_pipe         [pNUM_OF_CHANS-1:0][3:0];
  reg [(PORT_MEM_DBI_N_WIDTH_PER_INTF*8)-1:0]     mem_data_dm_pipe      [pNUM_OF_CHANS-1:0][3:0];
  reg [pLOCAL_Q_WIDTH-1:0]                        rd_data_d             [7:0];
  reg [PORT_MEM_DQA_WIDTH-1:0]                    rd_data_a_d           [7:0];
  reg [PORT_MEM_DQB_WIDTH-1:0]                    rd_data_b_d           [7:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            rd_data_debug_d       [pNUM_OF_CHANS-1:0][3:0][7:0];
  reg [5:0]                                       afi_rlat_sel          [pNUM_OF_CHANS-1:0];
  reg [5:0]                                       afi_wlat_sel          [pNUM_OF_CHANS-1:0];
  reg [5:0]                                       afi_wlat_sel_tmp;
  reg [5:0]                                       afi_rlat_sel_tmp;
  reg [5:0]                                       afi_wlat_sel_phase    [pNUM_OF_CHANS-1:0][3:0];
  reg [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]            mem_data_ph_d         [pNUM_OF_CHANS-1:0][3:0][7:0];
  reg [PORT_MEM_DBI_N_WIDTH_PER_INTF-1:0]         mem_data_dm_ph_d      [pNUM_OF_CHANS-1:0][3:0][7:0];
  reg [3:0]                                       rdata_valid_local_per_chan [pNUM_OF_CHANS-1:0];

  reg                                             first_wr;


  wire [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]           wr_data_d              [7:0][pNUM_OF_CHANS-1:0][3:0];
  wire [PORT_MEM_DQ_WIDTH_PER_INTF-1:0]           mem_data_pipe_array    [pNUM_OF_CHANS-1:0][3:0][7:0];
  wire [PORT_MEM_DBI_N_WIDTH_PER_INTF-1:0]        mem_data_dm_pipe_array [pNUM_OF_CHANS-1:0][3:0][7:0];
  wire [96*NUM_OF_RTL_TILES*LANES_PER_TILE-1:0]   ac_data_to_mem;
  wire [31:0]                                     banks_per_write;

  integer i,j,k,ii,jj,aa,bb,phase_num,cs_num,beat_num;
  integer interface_num;
  integer high_addr;

  assign ac_data_to_mem = ( NUM_OF_HMC_PORTS==0 ) ? dq_data_to_mem : ac_hmc;

  assign banks_per_write = PROTOCOL_ENUM != "PROTOCOL_RLD3" || ABPHY_WRITE_PROTOCOL==0 ? 1 : ABPHY_WRITE_PROTOCOL*2;

  generate
      genvar port_i;
      genvar phase_i;
      genvar cs_i;


      if (`_abphy_get_pin_count(PORT_MEM_A_PINLOC) != 0) begin : mem_a
        `_abphy_connect_to_abphy(PORT_MEM_A_PINLOC, PORT_MEM_A_WIDTH, pADDR_NUM_OF_PHASES, addr_d, ac_data_to_mem )
      end

      if (`_abphy_get_pin_count(PORT_MEM_BA_PINLOC) != 0) begin : mem_ba
        `_abphy_connect_to_abphy(PORT_MEM_BA_PINLOC, PORT_MEM_BA_WIDTH, 4, bank_a_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_BG_PINLOC) != 0) begin : mem_bg
         `_abphy_connect_to_abphy(PORT_MEM_BG_PINLOC, PORT_MEM_BG_WIDTH, 4, bank_g_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_CS_N_PINLOC) != 0) begin : mem_cs_n
        `_abphy_connect_to_abphy(PORT_MEM_CS_N_PINLOC, PORT_MEM_CS_N_WIDTH, 4, cs_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_ACT_N_PINLOC) != 0) begin : mem_act_n
        `_abphy_connect_to_abphy(PORT_MEM_ACT_N_PINLOC, PORT_MEM_ACT_N_WIDTH, 4, act_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_RAS_N_PINLOC) != 0) begin : mem_ras
        `_abphy_connect_to_abphy(PORT_MEM_RAS_N_PINLOC, PORT_MEM_RAS_N_WIDTH, 4, ras_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_CAS_N_PINLOC) != 0) begin : mem_cas
        `_abphy_connect_to_abphy(PORT_MEM_CAS_N_PINLOC, PORT_MEM_CAS_N_WIDTH, 4, cas_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_WE_N_PINLOC) != 0) begin : mem_we_n
        `_abphy_connect_to_abphy(PORT_MEM_WE_N_PINLOC, PORT_MEM_WE_N_WIDTH, 4, we_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_REF_N_PINLOC) != 0) begin : mem_ref_n
        `_abphy_connect_to_abphy(PORT_MEM_REF_N_PINLOC, PORT_MEM_REF_N_WIDTH, 4, ref_n_d, ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_WPS_N_PINLOC) != 0) begin : mem_wps_n
        `_abphy_connect_to_abphy(PORT_MEM_WPS_N_PINLOC,PORT_MEM_WPS_N_WIDTH , 4, wps_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_RPS_N_PINLOC) != 0) begin : mem_rps_n
        `_abphy_connect_to_abphy(PORT_MEM_RPS_N_PINLOC,PORT_MEM_RPS_N_WIDTH , 4, rps_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_DINVA_PINLOC) != 0) begin : mem_dinva
        `_abphy_connect_to_abphy(PORT_MEM_DINVA_PINLOC,PORT_MEM_DINVA_WIDTH , 8, dinva_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_DINVB_PINLOC) != 0) begin : mem_dinvb
        `_abphy_connect_to_abphy(PORT_MEM_DINVB_PINLOC,PORT_MEM_DINVB_WIDTH , 8, dinvb_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_AINV_PINLOC) != 0) begin : mem_ainv
        `_abphy_connect_to_abphy(PORT_MEM_AINV_PINLOC,PORT_MEM_AINV_WIDTH , 8, ainv_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_DQA_PINLOC) != 0) begin : mem_dqa
        `_abphy_connect_to_abphy(PORT_MEM_DQA_PINLOC,PORT_MEM_DQA_WIDTH , 8, dqa_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_DQB_PINLOC) != 0) begin : mem_dqb
        `_abphy_connect_to_abphy(PORT_MEM_DQB_PINLOC,PORT_MEM_DQB_WIDTH , 8, dqb_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_RWA_N_PINLOC) != 0) begin : mem_rwa_n
        `_abphy_connect_to_abphy(PORT_MEM_RWA_N_PINLOC,PORT_MEM_RWA_N_WIDTH , 8, rwa_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_RWB_N_PINLOC) != 0) begin : mem_rwb_n
        `_abphy_connect_to_abphy(PORT_MEM_RWB_N_PINLOC,PORT_MEM_RWB_N_WIDTH , 8, rwb_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_LDA_N_PINLOC) != 0) begin : mem_lda_n
        `_abphy_connect_to_abphy(PORT_MEM_LDA_N_PINLOC,PORT_MEM_LDA_N_WIDTH , 8, lda_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_LDB_N_PINLOC) != 0) begin : mem_ldb_n
        `_abphy_connect_to_abphy(PORT_MEM_LDB_N_PINLOC,PORT_MEM_LDB_N_WIDTH , 8, ldb_n_d , ac_data_to_mem)
      end

      if (`_abphy_get_pin_count(PORT_MEM_CK_PINLOC) != 0) begin : mem_ck
        `_abphy_connect_to_abphy(PORT_MEM_CK_PINLOC,PORT_MEM_CK_WIDTH , 8, ck_d , ac_data_to_mem)
      end


      if ( PROTOCOL_ENUM == "PROTOCOL_QDR2" ) begin : mem_d_qdr2
        `_abphy_connect_to_abphy   (PORT_MEM_D_PINLOC,  pLOCAL_D_WIDTH , 8, data_d,    dq_data_to_mem)
        `_abphy_connect_to_abphy_oe(PORT_MEM_D_PINLOC,  pLOCAL_D_WIDTH,  4, oe_d,      dq_oe)
        `_abphy_connect_from_abphy (PORT_MEM_Q_PINLOC,  pLOCAL_Q_WIDTH,  8, rd_data_d, dq_data_from_mem)
      end
      else if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin : mem_d_qdr4
        `_abphy_connect_to_abphy_oe(PORT_MEM_DQA_PINLOC,PORT_MEM_DQA_WIDTH,  4, oe_a_d,      dq_oe)
        `_abphy_connect_to_abphy_oe2(PORT_MEM_DQB_PINLOC,PORT_MEM_DQB_WIDTH,  4, oe_b_d,      dq_oe)
        for (port_i = 0; port_i < pLOCAL_D_WIDTH; ++port_i) begin : cc
           for (phase_i = 0; phase_i < 4; ++phase_i) begin : dd
             if ( port_i<PORT_MEM_DQA_WIDTH ) begin : ee
               assign oe_d[phase_i][port_i]  =  oe_a_d[phase_i][port_i];
             end
             else begin
               assign oe_d[phase_i][port_i]  =  oe_b_d[phase_i][port_i-PORT_MEM_DQA_WIDTH];
             end
           end
        end
        for (port_i = 0; port_i < pLOCAL_Q_WIDTH; ++port_i) begin : ff
           for (phase_i = 0; phase_i < 8; ++phase_i) begin : gg
             if ( port_i<PORT_MEM_DQA_WIDTH ) begin : hh
               assign rd_data_a_d[phase_i][port_i]                     =  rd_data_d[phase_i][port_i];
             end
             else begin
               assign rd_data_b_d[phase_i][port_i-PORT_MEM_DQA_WIDTH]  =  rd_data_d[phase_i][port_i];
             end
           end
        end
        `_abphy_connect_from_abphy (PORT_MEM_DQA_PINLOC,PORT_MEM_DQA_WIDTH , 8, rd_data_a_d, dq_data_from_mem)
        `_abphy_connect_from_abphy2 (PORT_MEM_DQB_PINLOC,PORT_MEM_DQB_WIDTH , 8, rd_data_b_d, dq_data_from_mem)
        if (`_abphy_get_pin_count(PORT_MEM_DINVA_PINLOC) != 0) begin : rd_dinva
          `_abphy_connect_from_abphy(PORT_MEM_DINVA_PINLOC,PORT_MEM_DINVA_WIDTH , 8, rd_dinva_d , dq_data_from_mem)
        end

        if (`_abphy_get_pin_count(PORT_MEM_DINVB_PINLOC) != 0) begin : rd_dinvb
          `_abphy_connect_from_abphy(PORT_MEM_DINVB_PINLOC,PORT_MEM_DINVB_WIDTH , 8, rd_dinvb_d , dq_data_from_mem)
        end
      end
      else begin : mem_d_all_others
        if (`_abphy_get_pin_count(PORT_MEM_DQ_PINLOC) != 0) begin : mem_dq
          `_abphy_connect_to_abphy   (PORT_MEM_DQ_PINLOC, pLOCAL_D_WIDTH, 8, data_d,    dq_data_to_mem)
          `_abphy_connect_to_abphy_oe(PORT_MEM_DQ_PINLOC, pLOCAL_D_WIDTH, 4, oe_d,      dq_oe)
          `_abphy_connect_from_abphy (PORT_MEM_DQ_PINLOC, pLOCAL_Q_WIDTH, 8, rd_data_d, dq_data_from_mem)
        end
      end

        if ( PROTOCOL_ENUM == "PROTOCOL_DDR4" ) begin : mem_dm_ddr4
          if (`_abphy_get_pin_count(PORT_MEM_DBI_N_PINLOC) != 0) begin : mem_dbi_n_ddr4
            `_abphy_connect_to_abphy(PORT_MEM_DBI_N_PINLOC, PORT_MEM_DBI_N_WIDTH, 8, data_dm_d, dq_data_to_mem)
            `_abphy_connect_from_abphy (PORT_MEM_DBI_N_PINLOC, PORT_MEM_DBI_N_WIDTH, 8, rd_dbi_d, dq_data_from_mem)
          end
        end
        else if ( PROTOCOL_ENUM == "PROTOCOL_QDR2" ) begin : mem_dm_qdr
          if (`_abphy_get_pin_count(PORT_MEM_BWS_N_PINLOC) != 0) begin : mem_bws_n
            `_abphy_connect_to_abphy(PORT_MEM_BWS_N_PINLOC,PORT_MEM_BWS_N_WIDTH , 4, data_dm_d , dq_data_to_mem)
          end
        end
        else begin : mem_dm
          if (`_abphy_get_pin_count(PORT_MEM_DM_PINLOC) != 0) begin : mem_dm_n_ddr3
            `_abphy_connect_to_abphy(PORT_MEM_DM_PINLOC, PORT_MEM_DM_WIDTH, 8, data_dm_d, dq_data_to_mem)
          end
        end


  endgenerate
  initial begin
    for ( i=0; i<8; i++ ) begin
      rd_dbi_d[i]={pLOCAL_DM_WIDTH{1'b1}};
      rd_dinva_d[i]={PORT_MEM_DINVA_WIDTH{1'b0}};
      rd_dinvb_d[i]={PORT_MEM_DINVB_WIDTH{1'b0}};
    end
  end

  always @ ( * ) begin
    for (j = 0; j < pNUM_OF_CHANS; j++) begin
      if ( PROTOCOL_ENUM == "PROTOCOL_DDR4" ) begin
        if ( PHY_PING_PONG_EN==1 ) begin
          if ( j>=(pNUM_OF_CHANS/2) ) begin
            if ( PHY_HMC_CLK_RATIO==4 ) begin
              afi_wlat_sel[j] = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))] - 2;
            end
            else begin
              afi_wlat_sel[j] = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))] - 1;
            end
            afi_rlat_sel[j]   = afi_rlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, (PORT_MEM_DQ_WIDTH/2))] - 4;
          end
          else begin
            if ( PHY_HMC_CLK_RATIO==4 ) begin
              afi_wlat_sel[j] = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)];
            end
            else begin
              afi_wlat_sel[j] = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] + 1;
            end
            afi_rlat_sel[j]   = afi_rlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] - 2;
          end
        end
        else begin
          if ( PHY_HMC_CLK_RATIO==4 ) begin
            afi_wlat_sel[j]   = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)];
          end
          else begin
            afi_wlat_sel[j]   = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] + 1;
          end
          afi_rlat_sel[j]     = afi_rlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] - 2;
        end
      end
      else if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
        afi_wlat_sel_tmp      = j==0 ? afi_wlat[`_abphy_get_tile(PORT_MEM_DQA_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQA_PINLOC, 0)] :
                                       afi_wlat[`_abphy_get_tile(PORT_MEM_DQB_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQB_PINLOC, 0)];
        afi_rlat_sel_tmp      = j==0 ? afi_rlat[`_abphy_get_tile(PORT_MEM_DQA_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQA_PINLOC, 0)] :
                                       afi_rlat[`_abphy_get_tile(PORT_MEM_DQB_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQB_PINLOC, 0)];
        if ( PHY_HMC_CLK_RATIO==2 && MEM_BURST_LENGTH==8 )

          afi_wlat_sel[j]     = afi_wlat_sel_tmp + 1;
        else if ( MEM_BURST_LENGTH==2 )
          afi_wlat_sel[j]     = afi_wlat_sel_tmp - 1;
        else
          afi_wlat_sel[j]     = afi_wlat_sel_tmp;

        if ( NUM_OF_HMC_PORTS==0 ) begin
          afi_wlat_sel[j]     = afi_wlat_sel[j]-1;
        end
        afi_rlat_sel[j]       = afi_rlat_sel_tmp - 2;
      end
      else begin
        if ( PHY_HMC_CLK_RATIO==2 && MEM_BURST_LENGTH==8 )
          afi_wlat_sel[j]     = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] + 1;
        else if ( MEM_BURST_LENGTH==2 )
          afi_wlat_sel[j]     = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] - 1;
        else
          afi_wlat_sel[j]     = afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)];

        if ( NUM_OF_HMC_PORTS==0 ) begin
          afi_wlat_sel[j]     = afi_wlat_sel[j]-1;
        end
        afi_rlat_sel[j]       = afi_rlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)] - 2;
      end
    end
  end

  integer mod_phase,rounded_num_of_cycles,max_num_of_cycles;
  always @ ( negedge  phy_clk ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      max_num_of_cycles                       = ((6%(2*PHY_HMC_CLK_RATIO)+MEM_BURST_LENGTH)+(2*PHY_HMC_CLK_RATIO)-1)/(2*PHY_HMC_CLK_RATIO);
      for ( k=0; k<4; k++ ) begin
        mod_phase                             = (k*2)%(2*PHY_HMC_CLK_RATIO);
        rounded_num_of_cycles                 = ((mod_phase+MEM_BURST_LENGTH)+(2*PHY_HMC_CLK_RATIO)-1)/(2*PHY_HMC_CLK_RATIO);


        if ( rounded_num_of_cycles<max_num_of_cycles )
          afi_wlat_sel_phase[j][k]            = afi_wlat_sel[j]-1;
        else
          afi_wlat_sel_phase[j][k]            = afi_wlat_sel[j];
      end
    end
  end

  always @ ( * ) begin
    for ( i=0; i<pNUM_OF_INTF; i++ ) begin
      for ( j=0; j<8; j++ ) begin
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
          for ( k=0; k<PORT_MEM_DINVA_WIDTH; k++ ) begin
            if ( i==0 ) begin
              data_muxd_d[i][j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH]   =
                 (dinva_d[j][k]==1'b1 && `_abphy_get_pin_count(PORT_MEM_DINVA_PINLOC)!= 0) ?
                    ~dqa_d[j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH] :
                       dqa_d[j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH];
            end
            else begin
              data_muxd_d[i][j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH]   =
                 (dinvb_d[j][k]==1'b1 && `_abphy_get_pin_count(PORT_MEM_DINVB_PINLOC)!= 0) ?
                    ~dqb_d[j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH] :
                       dqb_d[j][(k*(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH))+:PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DINVA_WIDTH];
            end
          end
        end
        else begin
          if ( PROTOCOL_ENUM == "PROTOCOL_DDR4" && MEM_DATA_MASK_EN=='d0 && `_abphy_get_pin_count(PORT_MEM_DBI_N_PINLOC) != 0 && DBI_WR_ENABLE=="true" ) begin
            for ( k=0; k<PORT_MEM_DBI_N_WIDTH_PER_INTF; k++ ) begin
              if ( ~data_dm_d[j][k] )
                data_muxd_d[i][j][((PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)*k)+:(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)]=
                    ~data_d[j][((PORT_MEM_DQ_WIDTH_PER_INTF*i)+((PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)*k))+:(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)];
              else
                data_muxd_d[i][j][((PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)*k)+:(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)]=
                     data_d[j][((PORT_MEM_DQ_WIDTH_PER_INTF*i)+((PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)*k))+:(PORT_MEM_DQ_WIDTH_PER_INTF/PORT_MEM_DBI_N_WIDTH_PER_INTF)];
            end
          end
          else begin
            data_muxd_d[i][j]   = data_d[j][PORT_MEM_DQ_WIDTH_PER_INTF*i+:PORT_MEM_DQ_WIDTH_PER_INTF];
          end
        end

        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
          data_dm_muxd_d[i][j]  = {PORT_MEM_DBI_N_WIDTH_PER_INTF{1'b0}};
        end
        else if ( PROTOCOL_ENUM == "PROTOCOL_DDR3" || PROTOCOL_ENUM == "PROTOCOL_RLD3" || PROTOCOL_ENUM == "PROTOCOL_QDR2" || PROTOCOL_ENUM == "PROTOCOL_LPDDR3") begin
          data_dm_muxd_d[i][j]  = ~data_dm_d[j][PORT_MEM_DBI_N_WIDTH_PER_INTF*i+:PORT_MEM_DBI_N_WIDTH_PER_INTF];
        end
        else begin
          data_dm_muxd_d[i][j]  = data_dm_d[j][PORT_MEM_DBI_N_WIDTH_PER_INTF*i+:PORT_MEM_DBI_N_WIDTH_PER_INTF];
        end
      end
      for ( j=0; j<4; j++ ) begin
        oe_muxd_d[i][j]         = oe_d[j][PORT_MEM_DQ_WIDTH_PER_INTF*i+:PORT_MEM_DQ_WIDTH_PER_INTF];
      end
    end
  end

  always @ ( negedge phy_clk ) begin
    for ( i=0; i<4; i++ ) begin
      bank_d[i]=0;
      if ( PROTOCOL_ENUM == "PROTOCOL_LPDDR3" ) begin
        bank_d[i]            = addr_d[(2*i)][9:7];
      end
      else if ( PROTOCOL_ENUM != "PROTOCOL_QDR2" && PROTOCOL_ENUM != "PROTOCOL_QDR4" ) begin
        for ( j=0; j<`_abphy_get_pin_count(PORT_MEM_BA_PINLOC); j++ ) begin
          bank_d[i][j]       = bank_a_d[i][j];
        end
        for ( j=`_abphy_get_pin_count(PORT_MEM_BA_PINLOC); j<`_abphy_get_pin_count(PORT_MEM_BA_PINLOC)+`_abphy_get_pin_count(PORT_MEM_BG_PINLOC); j++ ) begin
          bank_d[i][j]     = bank_g_d[i][j-`_abphy_get_pin_count(PORT_MEM_BA_PINLOC)];
        end
      end
    end
  end

  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      for ( i=0; i<4; i++ ) begin
        cmd_d[j][i]              = 5'b11111;
        if ( i<PHY_HMC_CLK_RATIO && afi_cal_success==1'b1 && runAbstractPhySim==1'b1 ) begin
          case ( PROTOCOL_ENUM )
            "PROTOCOL_DDR4" : begin
              cmd_d[j][i]        = {cs_n_d[i][j],act_n_d[i],addr_d[i][16],addr_d[i][15],addr_d[i][14]};
            end
            "PROTOCOL_DDR3" : begin
              cmd_d[j][i]        = {cs_n_d[i][j],ras_n_d[i],cas_n_d[i],we_n_d[i],1'b0};
            end
            "PROTOCOL_RLD3" : begin
              cmd_d[j][i]        = {cs_n_d[i][j],we_n_d[i],ref_n_d[i],2'd0};
            end
            "PROTOCOL_LPDDR3" : begin
              cmd_d[j][i]        = {cs_n_d[i][j],addr_d[(2*i)][0],addr_d[(2*i)][1],addr_d[(2*i)][2],1'd0};
            end
            "PROTOCOL_QDR2" : begin
              cmd_d[j][i]        = {wps_n_d[i][j],rps_n_d[i][j],3'b000};
            end
            "PROTOCOL_QDR4" : begin
              if ( j==0 ) begin
                cmd_d[j][i]      = {lda_n_d[{i[1:0],j[0]}],rwa_n_d[{i[1:0],j[0]}],2'd0,ck_d[{i[1:0],j[0]}]};
              end
              else begin
                cmd_d[j][i]      = {ldb_n_d[{i[1:0],j[0]}],rwb_n_d[{i[1:0],j[0]}],2'd0,~ck_d[{i[1:0],j[0]}]};
              end
            end
            default : begin
              //$display("ERROR FAIL : protocol not defined");
            end
          endcase
        end
      end
    end
  end

  generate
    if ( PROTOCOL_ENUM=="PROTOCOL_LPDDR3" ) begin : lpddr_row_addr
      always @ ( * ) begin
        for ( i=0; i<4; i++ ) begin
          lpddr3_row_addr_d[i]        = {addr_d[(2*i)+1][9:8],addr_d[(2*i)][6:2],addr_d[(2*i)+1][7:0]};
        end
      end
    end
  endgenerate


  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      active_detected                        = 'd0;
      active_bank                            = 'd0;
      row_addr_d[j]                          = 'd0;
      row_addr_d_en[j]                       = 'd0;
      for ( i=0; i<4; i++ ) begin
        casex ( cmd_d[j][i] )
          pCMD_ACTIVE : begin
            active_detected                  = 1'b1;
            if ( PROTOCOL_ENUM=="PROTOCOL_LPDDR3" ) begin
              active_bank                    = bank_d[i];
              row_addr_d[j]                  = lpddr3_row_addr_d[i];
            end
            else begin
              active_bank                    = bank_d[i];
              row_addr_d[j]                  = addr_d[i];
            end
          end
          pCMD_WRITE : begin
            if ( PROTOCOL_ENUM=="PROTOCOL_LPDDR3" ) begin
              if ( active_detected==1'b1 &&  bank_d[i]==active_bank )
                row_addr_d_en[j]             = 1'b1;
              else
                row_addr_d_en[j]             = 1'b0;
            end
            else begin
              if ( active_detected==1'b1 &&  bank_d[i]==active_bank )
                row_addr_d_en[j]             = 1'b1;
              else
                row_addr_d_en[j]             = 1'b0;
            end
          end
          pCMD_READ : begin
            if ( PROTOCOL_ENUM=="PROTOCOL_LPDDR3" ) begin
              if ( active_detected==1'b1 &&  bank_d[i]==active_bank )
                row_addr_d_en[j]               = 1'b1;
              else
                row_addr_d_en[j]               = 1'b0;
            end
            else begin
              if ( active_detected==1'b1 &&  bank_d[i]==active_bank )
                row_addr_d_en[j]               = 1'b1;
              else
                row_addr_d_en[j]               = 1'b0;
            end
          end
        endcase
      end
    end
  end

  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<16; i++ ) begin
          row_addr[i][j]                     <= 'd0;
        end
      end
    end
    else begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<4; i++ ) begin
          if ( (^cmd_d[j][i]) !==1'bx  && PROTOCOL_ENUM != "PROTOCOL_RLD3" ) begin
            casex ( cmd_d[j][i] )
              pCMD_ACTIVE : begin
                if ( PROTOCOL_ENUM=="PROTOCOL_LPDDR3" ) begin
                  row_addr[bank_d[i]][j]        <= lpddr3_row_addr_d[i];
                  //$display("abphy FIFO IN : CS%-d ACTIVATE(%b):%0t bank=%d row_addr=%x",j,cmd_d[j][i],$time,bank_d[i],lpddr3_row_addr_d[i]);
                end
                else begin
                  row_addr[bank_d[i]][j]        <= addr_d[i];
                  //$display("abphy FIFO IN : CS%-d ACTIVATE(%b):%0t bank=%d full_addr=%x row_addr=%x",j,cmd_d[j][i],$time,bank_d[i],addr_d[i],addr_d[i]);
                end
              end
            endcase
          end
        end
      end
    end
  end

  parameter COL_ADDR_PAD_WIDTH = (PROTOCOL_ENUM == "PROTOCOL_LPDDR3") ? 0 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_12") ? LOCAL_ADDR_WIDTH-12 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_11") ? LOCAL_ADDR_WIDTH-11 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_10") ? LOCAL_ADDR_WIDTH-10 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_9")  ? LOCAL_ADDR_WIDTH-9 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_8")  ? LOCAL_ADDR_WIDTH-8 :
    (PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH=="col_width_7")  ? LOCAL_ADDR_WIDTH-7 :
              LOCAL_ADDR_WIDTH-6;

  always @ ( * ) begin
    for ( i=0; i<pADDR_NUM_OF_PHASES; i++ ) begin
      if ( PROTOCOL_ENUM == "PROTOCOL_RLD3" || PROTOCOL_ENUM == "PROTOCOL_QDR2" ||
                                        PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" && `_abphy_get_pin_count(PORT_MEM_AINV_PINLOC) != 0 ) begin
          col_addr_d[i]               = ainv_d[i] ? ~addr_d[i] : addr_d[i];
        end
        else begin
          col_addr_d[i]               = addr_d[i];
        end
      end
      else if ( PROTOCOL_ENUM == "PROTOCOL_LPDDR3" ) begin
        if ( i<4 ) begin
          col_addr_d[i]               = {addr_d[(2*i)+1][9:1],addr_d[(2*i)][6:5],1'b0};
        end
        else begin
          col_addr_d[i]               = 'd0;
        end
      end
      else begin
        case( PRI_HMC_CFG_MEM_IF_COLADDR_WIDTH )
		        "col_width_12" : begin
            if ( PROTOCOL_ENUM == "PROTOCOL_DDR3" )
              col_addr_d[i]           = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][13],addr_d[i][11],addr_d[i][9:0]};
            else
              col_addr_d[i]           = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][12:11],addr_d[i][9:0]};
          end
		        "col_width_11" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][11],addr_d[i][9:0]};
          end
		        "col_width_10" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][9:0]};
          end
		        "col_width_9" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][9:0]};
          end
		        "col_width_8" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][9:0]};
          end
		        "col_width_7" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][9:0]};
          end
		        "col_width_6" : begin
            col_addr_d[i]             = {{COL_ADDR_PAD_WIDTH{1'b0}},addr_d[i][9:0]};
          end

          default : begin
            //$display("ERROR FAIL : col addr not defined");
          end
        endcase
      end
    end
  end

  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<4; i++ ) begin
          wr_WRITE_en[j][i]                          <= 1'b0;
          wr_READ_en[j][i]                           <= 1'b0;
          wr_ac_data[j][i]                           <= 'd0;
        end
      end
    end
    else begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<4; i++ ) begin
          wr_WRITE_en[j][i]                       <= 1'b0;
          wr_READ_en[j][i]                        <= 1'b0;
          wr_ac_data[j][i]                        <= {bank_d[i],row_addr_d[j],col_addr_d[i]};
          if ( (^cmd_d[j][i]) !==1'bx ) begin
            casex ( cmd_d[j][i] )
              pCMD_WRITE : begin
                wr_WRITE_en[j][i]                 <= 1'b1;
                wr_READ_en[j][i]                  <= 1'b0;
                if ( row_addr_d_en[j] ) begin
                  wr_ac_data[j][i]                <= {bank_d[i],row_addr_d[j],col_addr_d[i]};
                  //$display("abphy FIFO IN : CS%-d phase=%-d WRITE(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr_d[j],col_addr_d[i]);
                end
                else begin
                  if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
                    wr_ac_data[j][i]              <= {bank_d[i],row_addr[bank_d[i]][j],col_addr_d[{i[1:0],j[0]}]};
                    //$display("abphy FIFO IN : CS%-d phase=%-d qdr4WRITE(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x", j,i,cmd_d[j][i],$time,bank_d[i],addr_d[{i[1:0],j[0]}],row_addr[bank_d[i]][j],col_addr_d[{i[1:0],j[0]}]);
                  end
                  else begin
                    wr_ac_data[j][i]              <= {bank_d[i],row_addr[bank_d[i]][j],col_addr_d[i]};
                    if ( PROTOCOL_ENUM == "PROTOCOL_QDR2" && pADDR_PIN_RATE=="mode_ddr" ) begin
                      //$display("abphy FIFO IN : CS%-d phase=%-d qdrWRITE(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x", j,i,cmd_d[j][i],$time,bank_d[i+1],addr_d[i+1],row_addr[bank_d[i]][j],col_addr_d[i+1]);
                    end
                    else begin
                      //$display("abphy FIFO IN : CS%-d phase=%-d WRITE(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr[bank_d[i]][j],col_addr_d[i]);
                    end
                  end
                end
              end
              pCMD_READ : begin
                wr_WRITE_en[j][i]                 <= 1'b0;
                wr_READ_en[j][i]                  <= 1'b1;
                if ( row_addr_d_en[j] ) begin
                  //$display("abphy FIFO IN : CS%-d phase=%-d READ(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr_d[j],col_addr_d[i]);
                  wr_ac_data[j][i]                <= {bank_d[i],row_addr_d[j],col_addr_d[i]};
                end
                else begin
                  if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
                    wr_ac_data[j][i]            <= {bank_d[i],row_addr[bank_d[i]][j],col_addr_d[{i[1:0],j[0]}]};
                    //$display("abphy FIFO IN : CS%-d phase=%-d qdr4READ(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x", j,i,cmd_d[j][i],$time,bank_d[i],addr_d[{i[1:0],j[0]}],row_addr[bank_d[i]][j],col_addr_d[{i[1:0],j[0]}]);
                  end
                  else begin
                    //$display("abphy FIFO IN : CS%-d phase=%-d READ(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr[bank_d[i]][j],col_addr_d[i]);
                    wr_ac_data[j][i]              <= {bank_d[i],row_addr[bank_d[i]][j],col_addr_d[i]};
                  end
                end
              end
              pCMD_WRITE_N_READ :begin
                if ( PROTOCOL_ENUM != "PROTOCOL_DDR4" ) begin
                  wr_WRITE_en[j][i]                 <= 1'b1;
                  wr_READ_en[j][i]                  <= 1'b1;
                  if ( row_addr_d_en[j] ) begin
                    //$display("abphy FIFO IN : CS%-d phase=%-d WRITE/READ(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr_d[j],col_addr_d[i]);
                    wr_ac_data[j][i]                <= {bank_d[i],row_addr_d[j],col_addr_d[i]};
                  end
                  else begin
                    if ( PROTOCOL_ENUM == "PROTOCOL_QDR2" && pADDR_PIN_RATE=="mode_ddr" ) begin
                      //$display("abphy FIFO IN : CS%-d phase=%-d qdrWRITE_read(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i+1],addr_d[i+1],row_addr_d[j],col_addr_d[i+1]);
                      //$display("abphy FIFO IN : CS%-d phase=%-d qdrREAD_write(%b):%0t bank=%d full_addr=%x row_addr=%x col_addr=%x",j,i,cmd_d[j][i],$time,bank_d[i],addr_d[i],row_addr[bank_d[i]][j],col_addr_d[i]);
                    end
                    else begin
                    end
                    wr_ac_data[j][i]                <= {bank_d[i],row_addr[bank_d[i]][j],col_addr_d[i]};
                  end
                end
              end
            endcase
          end
        end
      end
    end
  end


  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<64; i++ ) begin
          for ( k=0; k<4; k++ ) begin
            ac_cmd_pipeline[i][j][k]      <= 'd0;
          end
        end
      end
    end
    else begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( k=0; k<4; k++ ) begin
          ac_cmd_pipeline[0][j][k]      <= {wr_WRITE_en[j][k],wr_READ_en[j][k],wr_ac_data[j][k]};
          for ( i=1; i<64; i++ ) begin
            ac_cmd_pipeline[i][j][k]    <= ac_cmd_pipeline[i-1][j][k];
          end
        end
      end
    end
  end


  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      for ( k=0; k<4; k++ ) begin
        RD_mem_addr_d[j][k]        = ac_cmd_pipeline[afi_rlat_sel[j]][j][k][pAC_PIPE_ADDR_WIDTH-1:0];
        RD_req_d[j][k]             = ac_cmd_pipeline[afi_rlat_sel[j]][j][k][pAC_PIPE_ADDR_WIDTH];
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR2" && k<2 && pADDR_PIN_RATE=="mode_ddr") begin
          WR_mem_addr_d[j][k]      = afi_wlat_sel_phase[j][k]==6'h3f ? wr_ac_data[j][k]  : ac_cmd_pipeline[afi_wlat_sel_phase[j][k]][j][k+1][pAC_PIPE_ADDR_WIDTH-1:0];
        end
        else begin
          WR_mem_addr_d[j][k]      = afi_wlat_sel_phase[j][k]==6'h3f ? wr_ac_data[j][k]  : ac_cmd_pipeline[afi_wlat_sel_phase[j][k]][j][k][pAC_PIPE_ADDR_WIDTH-1:0];
        end
        WR_req_d[j][k]             = afi_wlat_sel_phase[j][k]==6'h3f ? wr_WRITE_en[j][k] : ac_cmd_pipeline[afi_wlat_sel_phase[j][k]][j][k][pAC_PIPE_ADDR_WIDTH+1];
      end
    end
  end

  integer curr_element,reverse_cnt;
  always @ ( negedge phy_clk ) begin
    for ( cs_num=0; cs_num<pNUM_OF_CHANS; cs_num++ ) begin
      interface_num                                         = (pNUM_OF_INTF>1 && cs_num>=(pNUM_OF_CHANS/pNUM_OF_INTF))? 1 : 0;
      for ( phase_num=0; phase_num<4; phase_num++ ) begin
        if ( WR_req_d[cs_num][phase_num] ) begin
          curr_element                                      = ((phase_num+(MEM_BURST_LENGTH/2)-1)*2)%(PHY_HMC_CLK_RATIO*2)+1;
          reverse_cnt                                       = (PHY_HMC_CLK_RATIO*2)-curr_element-1;
          for ( beat_num=MEM_BURST_LENGTH-1; beat_num>=0; beat_num-- ) begin
            if ( reverse_cnt<(2*PHY_HMC_CLK_RATIO) ) begin
              mem_data_ph_d[cs_num][phase_num][beat_num]    = data_muxd_d[interface_num][curr_element];
              mem_data_dm_ph_d[cs_num][phase_num][beat_num] = data_dm_muxd_d[interface_num][curr_element];
            end
            else if ( reverse_cnt<(4*PHY_HMC_CLK_RATIO) ) begin
              mem_data_ph_d[cs_num][phase_num][beat_num]    = mem_data[interface_num][curr_element];
              mem_data_dm_ph_d[cs_num][phase_num][beat_num] = mem_data_dm[interface_num][curr_element];
            end
            else begin
              mem_data_ph_d[cs_num][phase_num][beat_num]    = mem_data_r1[interface_num][curr_element];
              mem_data_dm_ph_d[cs_num][phase_num][beat_num] = mem_data_dm_r1[interface_num][curr_element];
            end
            reverse_cnt++;
            curr_element = curr_element==0 ? (PHY_HMC_CLK_RATIO*2)-1 : (curr_element-1);
          end
        end
      end
    end
  end

  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( i=0; i<pNUM_OF_INTF; i++ ) begin
        for ( beat_num=0; beat_num<8; beat_num++ ) begin
          mem_data[i][beat_num]             <= 'd0;
          mem_data_r1[i][beat_num]          <= 'd0;
          mem_data_dm[i][beat_num]          <= 'd0;
          mem_data_dm_r1[i][beat_num]       <= 'd0;
        end
      end
    end
    else begin
      for ( i=0; i<pNUM_OF_INTF; i++ ) begin
        for ( beat_num=0; beat_num<8; beat_num++ ) begin
          mem_data[i][beat_num]             <= data_muxd_d[i][beat_num];
          mem_data_r1[i][beat_num]          <= mem_data[i][beat_num];
          mem_data_dm[i][beat_num]          <= data_dm_muxd_d[i][beat_num];
          mem_data_dm_r1[i][beat_num]       <= mem_data_dm[i][beat_num];
        end
      end
    end
  end

  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( i=0; i<16; i++ ) begin
          for ( k=0; k<4; k++ ) begin
            WR_mem_pipeline[i][j][k]        <= 'd0;
          end
        end
      end
    end
    else begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        interface_num                   = (pNUM_OF_INTF>1 && j>=(pNUM_OF_CHANS/pNUM_OF_INTF))? 1 : 0;
        for ( k=0; k<4; k++ ) begin
          WR_mem_pipeline[0][j][k]          <= { WR_req_d[j][k], WR_mem_addr_d[j][k],
                                                   mem_data_dm_ph_d[j][k][7],mem_data_dm_ph_d[j][k][6],mem_data_dm_ph_d[j][k][5],mem_data_dm_ph_d[j][k][4],
                                                   mem_data_dm_ph_d[j][k][3],mem_data_dm_ph_d[j][k][2],mem_data_dm_ph_d[j][k][1],mem_data_dm_ph_d[j][k][0],
                                                   mem_data_ph_d[j][k][7],mem_data_ph_d[j][k][6],mem_data_ph_d[j][k][5],mem_data_ph_d[j][k][4],
                                                   mem_data_ph_d[j][k][3],mem_data_ph_d[j][k][2],mem_data_ph_d[j][k][1],mem_data_ph_d[j][k][0] };
          for ( i=1; i<16; i++ ) begin
            WR_mem_pipeline[i][j][k]        <= WR_mem_pipeline[i-1][j][k];
          end
        end
      end
    end
  end

  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      for ( k=0; k<4; k++ ) begin
        mem_wr_pipe[j][k]      =                    WR_mem_pipeline[pWRITE_PATH_LAT_INDEX][j][k][pAC_PIPE_ADDR_WIDTH+(PORT_MEM_DQ_WIDTH_PER_INTF*8)+(PORT_MEM_DBI_N_WIDTH_PER_INTF*8)];
        mem_addr_pipe[j][k]    =                    WR_mem_pipeline[pWRITE_PATH_LAT_INDEX][j][k][(PORT_MEM_DQ_WIDTH_PER_INTF*8)+(PORT_MEM_DBI_N_WIDTH_PER_INTF*8)+:pAC_PIPE_ADDR_WIDTH];
        mem_data_dm_pipe[j][k] = MEM_DATA_MASK_EN ? WR_mem_pipeline[pWRITE_PATH_LAT_INDEX][j][k][(PORT_MEM_DQ_WIDTH_PER_INTF*8)+:(PORT_MEM_DBI_N_WIDTH_PER_INTF*8)] :
                                                                                               {(PORT_MEM_DBI_N_WIDTH_PER_INTF*8){1'b1}};
        mem_data_pipe[j][k]    =                    WR_mem_pipeline[pWRITE_PATH_LAT_INDEX][j][k][0+:(PORT_MEM_DQ_WIDTH_PER_INTF*8)];
      end
    end
  end

  integer phase_to_put_data_too;
  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      for ( k=0; k<4; k++ ) begin
        phase_to_put_data_too                   = PROTOCOL_ENUM == "PROTOCOL_DDR4" || PROTOCOL_ENUM == "PROTOCOL_DDR3" || PROTOCOL_ENUM == "PROTOCOL_LPDDR3"? 0 : k;
        case( mem_rd_state[j][k] )
          pRS_IDLE: begin
            for ( i=0; i<8; i++ ) begin
              rd_mem_shifted_en_d[j][k][i]      = pNO_RD_DATA;
              rd_mem_shifted_element_d[j][k][i] = 'd0;
              mem_rd_amt_prev_to_send_d[j][k]   = 'd0;
            end

            if ( RD_req_d[j][k] ) begin
              mem_rd_state_d[j][k]                  = pRS_READING;
              mem_rd_amt_sent_d[j][k]               = ((PHY_HMC_CLK_RATIO-phase_to_put_data_too)*2)>MEM_BURST_LENGTH ? MEM_BURST_LENGTH : ((PHY_HMC_CLK_RATIO-phase_to_put_data_too)*2);
              mem_rd_delay_d[j][k]                  = 'd0;
              for ( i=0; i<8; i++ ) begin
                if ( i>=(2*phase_to_put_data_too) && i<((2*phase_to_put_data_too)+mem_rd_amt_sent_d[j][k]) ) begin
                  rd_mem_shifted_en_d[j][k][i]      = pCURRENT_RD_DATA;
                  rd_mem_shifted_element_d[j][k][i] = i-(2*phase_to_put_data_too);
                end
              end
            end
            else begin
              mem_rd_state_d[j][k]                  = pRS_IDLE;
              mem_rd_amt_sent_d[j][k]               = 'd0;
              mem_rd_delay_d[j][k]                  = 'd0;
            end
          end

          pRS_READING: begin

            for ( i=0; i<8; i++ ) begin
              rd_mem_shifted_en_d[j][k][i]      = pNO_RD_DATA;
              rd_mem_shifted_element_d[j][k][i] = 'd0;
              mem_rd_amt_prev_to_send_d[j][k]   = 'd0;
            end

            if ( RD_req_d[j][k] ) begin
              mem_rd_state_d[j][k]                  = pRS_READING;
              mem_rd_amt_sent_d[j][k]               = ((PHY_HMC_CLK_RATIO-phase_to_put_data_too)*2)>MEM_BURST_LENGTH ? MEM_BURST_LENGTH : ((PHY_HMC_CLK_RATIO-phase_to_put_data_too)*2);
              mem_rd_delay_d[j][k]                  = 'd0;
              for ( i=0; i<8; i++ ) begin
                if ( i>=(2*phase_to_put_data_too) && i<((2*phase_to_put_data_too)+mem_rd_amt_sent_d[j][k]) ) begin
                  rd_mem_shifted_en_d[j][k][i]      = pCURRENT_RD_DATA;
                  rd_mem_shifted_element_d[j][k][i] = i-(2*phase_to_put_data_too);
                end
              end
            end
            else begin
              if ( mem_rd_amt_sent[j][k]<MEM_BURST_LENGTH ) begin
                mem_rd_state_d[j][k]                = pRS_READING;
                mem_rd_amt_sent_d[j][k]             = (mem_rd_amt_sent[j][k]+(2*PHY_HMC_CLK_RATIO))>MEM_BURST_LENGTH ? MEM_BURST_LENGTH :
                                                                mem_rd_amt_sent[j][k] + (2*PHY_HMC_CLK_RATIO);
                mem_rd_delay_d[j][k]                = mem_rd_delay[j][k]+'d1;
              end
              else begin
                mem_rd_state_d[j][k]                = pRS_IDLE;
                mem_rd_amt_sent_d[j][k]             = 'd0;
                mem_rd_delay_d[j][k]                = 'd0;
              end
            end

            if ( mem_rd_amt_sent[j][k]<MEM_BURST_LENGTH && mem_rd_amt_sent[j][k]>0 ) begin
              if ( (mem_rd_amt_sent[j][k]+(2*PHY_HMC_CLK_RATIO))>MEM_BURST_LENGTH ) begin
                mem_rd_amt_prev_to_send_d[j][k]   = MEM_BURST_LENGTH-mem_rd_amt_sent[j][k];
              end
              else begin
                mem_rd_amt_prev_to_send_d[j][k]   = mem_rd_amt_sent[j][k] + (2*PHY_HMC_CLK_RATIO);
              end
              for ( i=0; i<mem_rd_amt_prev_to_send_d[j][k]; i++ ) begin
                rd_mem_shifted_en_d[j][k][i]      = mem_rd_delay[j][k]>'d0 ? pDELAY_2_RD_DATA : pDELAY_1_RD_DATA;
                rd_mem_shifted_element_d[j][k][i] = mem_rd_amt_sent[j][k]+i;
              end
            end
            else begin
              mem_rd_amt_prev_to_send_d[j][k]     = 'd0;
            end

          end
        endcase
      end
    end
  end



  always @ ( posedge phy_clk or negedge reset_n ) begin
    if ( ~reset_n ) begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( k=0; k<4; k++ ) begin
          mem_rd_state[j][k]               <= pRS_IDLE;
          mem_rd_amt_sent[j][k]            <= 'd0;
          mem_rd_delay[j][k]               <= 'd0;
          rd_mem_data[j][k]                <= 'd0;
          rd_mem_data_r[j][k]              <= 'd0;
        end
      end
    end
    else begin
      for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
        for ( k=0; k<4; k++ ) begin
          mem_rd_state[j][k]               <= mem_rd_state_d[j][k];
          mem_rd_amt_sent[j][k]            <= mem_rd_amt_sent_d[j][k];
          mem_rd_delay[j][k]               <= mem_rd_delay_d[j][k];
          rd_mem_data[j][k]                <= rd_mem_data_d[j][k];
          rd_mem_data_r[j][k]              <= rd_mem_data[j][k];
        end
      end
    end
  end


  generate
    genvar geni,geni2,geni3;
    for (geni = 0; geni < 8; ++geni) begin : aa1
      for ( geni2=0; geni2<pNUM_OF_CHANS; ++geni2 ) begin : cc3
        for (geni3 = 0; geni3 < 4; ++geni3) begin: wr_data_gen
          assign mem_data_pipe_array[geni2][geni3][geni]    = mem_data_pipe[geni2][geni3][PORT_MEM_DQ_WIDTH_PER_INTF-1+(PORT_MEM_DQ_WIDTH_PER_INTF*geni):(PORT_MEM_DQ_WIDTH_PER_INTF*geni)];
          assign mem_data_dm_pipe_array[geni2][geni3][geni] = mem_data_dm_pipe[geni2][geni3][PORT_MEM_DBI_N_WIDTH_PER_INTF-1+(PORT_MEM_DBI_N_WIDTH_PER_INTF*geni):(PORT_MEM_DBI_N_WIDTH_PER_INTF*geni)];
        end
      end
    end
  endgenerate


  always @ ( * ) begin
    for ( j=0; j<8; j++ ) begin
      rd_data_d[j] = 'd0;
    end

    for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
      if ( mem_rd_state_d[i][3]!=pRS_IDLE || mem_rd_state_d[i][2]!=pRS_IDLE || mem_rd_state_d[i][1]!=pRS_IDLE || mem_rd_state_d[i][0]!=pRS_IDLE) begin
        interface_num                   = (pNUM_OF_INTF>1 && i>=(pNUM_OF_CHANS/pNUM_OF_INTF))? 1 : 0;
        for ( j=0; j<8; j++ ) begin
          rd_data_d[j][(PORT_MEM_DQ_WIDTH_PER_INTF*interface_num)+:PORT_MEM_DQ_WIDTH_PER_INTF] =
                 rd_mem_data_shifted_d[i][(PORT_MEM_DQ_WIDTH_PER_INTF*j)+:PORT_MEM_DQ_WIDTH_PER_INTF];
        end
      end
    end
  end


generate
        if ( MEM_DATA_MASK_EN==1 ) begin: mem_wr_data_mask
  integer j;
  initial begin
    @ ( posedge reset_n );
    forever begin
      @ ( posedge phy_clk );
      #1;
      for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
          high_addr=0;
        end
        else begin
          high_addr=i;
        end
        for ( jj=0; jj<4; jj++ ) begin
          if ( mem_wr_pipe[i][jj] ) begin
            mem_temp             = mem[{high_addr[5:0],mem_addr_pipe[i][jj]}];
            mem_temp_array[7]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*8)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*7)];
            mem_temp_array[6]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*7)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*6)];
            mem_temp_array[5]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*6)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*5)];
            mem_temp_array[4]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*5)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*4)];
            mem_temp_array[3]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*4)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*3)];
            mem_temp_array[2]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*3)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*2)];
            mem_temp_array[1]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*2)-1:(PORT_MEM_DQ_WIDTH_PER_INTF*1)];
            mem_temp_array[0]    = mem_temp[(PORT_MEM_DQ_WIDTH_PER_INTF*1)-1:0];
            //$display("abphy : mem_array WR CS%-d addr=%x",i,mem_addr_pipe[i][jj]);
            for ( j=0; j<MEM_BURST_LENGTH; j++ ) begin
              if ( mem_temp_array[j]==={PORT_MEM_DQ_WIDTH_PER_INTF{1'bx}} ) begin
                mem_temp_array[j]       ='d0;
              end
              mem_temp_array_mask       = 'd0;
              for ( k=0; k<PORT_MEM_DBI_N_WIDTH_PER_INTF; k++ ) begin
                if ( PROTOCOL_ENUM == "PROTOCOL_RLD3" ) begin
                  case ( PORT_MEM_DQ_WIDTH_PER_INTF )
                    18 : begin
                      mem_temp_array_mask   = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<(k*pDM_MASK_LEN) );
                    end
                    36 : begin
                      mem_temp_array_mask   = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<(k*pDM_MASK_LEN) );
                      mem_temp_array_mask   = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<((k+2)*pDM_MASK_LEN) );
                    end
                    72 : begin
                      if ( k<2 ) begin
                        mem_temp_array_mask = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<(k*pDM_MASK_LEN) );
                        mem_temp_array_mask = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<((k+2)*pDM_MASK_LEN) );
                      end
                      else begin
                        mem_temp_array_mask = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<(((k-2)*pDM_MASK_LEN)+36) );
                        mem_temp_array_mask = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<((k*pDM_MASK_LEN)+36) );
                      end
                    end
                  endcase
                end
                else begin
                  mem_temp_array_mask   = mem_temp_array_mask | ( {pDM_MASK_LEN{mem_data_dm_pipe_array[i][jj][j][k]}}<<(k*pDM_MASK_LEN) );
                end
              end
              mem_temp_array[j]         = (mem_temp_array[j]&~mem_temp_array_mask)|(mem_data_pipe_array[i][jj][j]&mem_temp_array_mask);
              //$display("abphy : CS%-d phase%-d burst=%-d mask=%x masked_data=%x unmasked_data=%x",i,jj,j,mem_temp_array_mask,mem_temp_array[j],mem_data_pipe_array[i][jj][j]);
            end
            for ( ii=0; ii<banks_per_write; ii++ ) begin
              temp_addr = mem_addr_pipe[i][jj];
              temp_addr[pAC_PIPE_ADDR_WIDTH-1:pAC_PIPE_ADDR_WIDTH-pBANK_WIDTH]=temp_addr[pAC_PIPE_ADDR_WIDTH-1:pAC_PIPE_ADDR_WIDTH-pBANK_WIDTH]+(ii*(16/banks_per_write));
              mem[{high_addr[5:0],temp_addr}]    = {mem_temp_array[7],mem_temp_array[6],mem_temp_array[5],mem_temp_array[4],mem_temp_array[3],mem_temp_array[2],mem_temp_array[1],mem_temp_array[0]};
              //$display("abphy : multi-bank WR CS%-d phase%-d addr=%x data=%x",i,jj,temp_addr,mem[{high_addr[5:0],temp_addr}]);
            end
          end
        end
      end
    end
  end

        end
        else begin: mem_wr_no_data_mask

  integer j;
  initial begin
    @ ( posedge reset_n );
    forever begin
      @ ( posedge phy_clk );
      #1;
      for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
          high_addr=0;
        end
        else begin
          high_addr=i;
        end
        for ( jj=0; jj<4; jj++ ) begin
          if ( mem_wr_pipe[i][jj] ) begin
            if ( PROTOCOL_ENUM == "PROTOCOL_RLD3" ) begin
              for ( ii=0; ii<banks_per_write; ii++ ) begin
                temp_addr = mem_addr_pipe[i][jj];
                temp_addr[pAC_PIPE_ADDR_WIDTH-1:pAC_PIPE_ADDR_WIDTH-pBANK_WIDTH]=temp_addr[pAC_PIPE_ADDR_WIDTH-1:pAC_PIPE_ADDR_WIDTH-pBANK_WIDTH]+(ii*(16/banks_per_write));
                mem[{high_addr[5:0],temp_addr}]    = mem_data_pipe[i][jj];
                //$display("abphy : multi-bank WR CS%-d phase%-d addr=%x data=%x",i,jj,temp_addr,mem[{high_addr[5:0],temp_addr}]);
              end
            end
            else begin
              mem[{high_addr[5:0],mem_addr_pipe[i][jj]}]=mem_data_pipe[i][jj];
            end
          end
        end
      end
    end
  end

        end
endgenerate


  initial begin
    @ ( posedge reset_n );
    forever begin
      @ ( posedge phy_clk );
      #1;
      for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
        if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
          high_addr=0;
        end
        else begin
          high_addr=i;
        end
        for ( k=0; k<4; k++ ) begin
          if ( RD_req_d[i][k] ) begin
            if ( mem.exists({high_addr[5:0],RD_mem_addr_d[i][k]}) ) begin
              rd_mem_data_d[i][k]           = mem[{high_addr[5:0],RD_mem_addr_d[i][k]}];
            end
            else begin
              rd_mem_data_d[i][k]           = 'd0;
              //$display("abphy : WARNING: Attempting to read from uninitialized location @ CS%-d addr=%x at time=%0t",i,RD_mem_addr_d[i][k],$time);
            end
            //$display("abphy : memory_array RD CS%-d phase%-d addr=%x rd_data=%x at time=%0t",i,k,RD_mem_addr_d[i][k],mem[{high_addr[5:0],RD_mem_addr_d[i][k]}],$time);
          end
        end
      end
    end
  end

  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      rd_mem_data_shifted_d[j]         = 'd0;
      for ( k=0; k<4; k++ ) begin
        for ( i=0; i<8; i++ ) begin
          if ( rd_mem_shifted_en_d[j][k][i] == pCURRENT_RD_DATA ) begin
            rd_mem_data_shifted_d[j][(i*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF]=
                rd_mem_data_d[j][k][(rd_mem_shifted_element_d[j][k][i]*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF];
          end
          else if ( rd_mem_shifted_en_d[j][k][i] == pDELAY_1_RD_DATA ) begin
            rd_mem_data_shifted_d[j][(i*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF]=
                rd_mem_data[j][k][(rd_mem_shifted_element_d[j][k][i]*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF];
          end
          else if ( rd_mem_shifted_en_d[j][k][i] == pDELAY_2_RD_DATA ) begin
            rd_mem_data_shifted_d[j][(i*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF]=
                rd_mem_data_r[j][k][(rd_mem_shifted_element_d[j][k][i]*PORT_MEM_DQ_WIDTH_PER_INTF)+:PORT_MEM_DQ_WIDTH_PER_INTF];
          end
        end
      end
    end
  end

  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      rdata_valid_local_per_chan[j]                        = 'd0;
      for ( k=0; k<4; k++ ) begin
        for ( i=0; i<8; i++ ) begin
          if ( rd_mem_shifted_en_d[j][k][i] != pNO_RD_DATA ) begin
            if ( rd_mem_shifted_element_d[j][k][i] < MEM_BURST_LENGTH ) begin
              for ( ii=0; ii<4; ii++ ) begin
                if ( i[3:1]==ii ) begin
                  if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
                    rdata_valid_local_per_chan[j][ii]      = 1'b1;
                  end
                  else begin
                    rdata_valid_local_per_chan[0][ii]      = 1'b1;
                  end
                end
              end
            end
          end
        end
      end
    end
  end

  always @ ( * ) begin
    if ( PROTOCOL_ENUM == "PROTOCOL_QDR4" ) begin
      for ( i=0; i<PORT_MEM_DQA_WIDTH; i++ ) begin
        rdata_valid_local[`_abphy_get_tile(PORT_MEM_DQA_PINLOC,i)][`_abphy_get_lane(PORT_MEM_DQA_PINLOC,i)]= rdata_valid_local_per_chan[0];
      end
      for ( i=0; i<PORT_MEM_DQB_WIDTH; i++ ) begin
        rdata_valid_local[`_abphy_get_tile(PORT_MEM_DQB_PINLOC,i)][`_abphy_get_lane(PORT_MEM_DQB_PINLOC,i)]= rdata_valid_local_per_chan[1];
      end
    end
    else begin
      for ( i=0; i<NUM_OF_RTL_TILES; i++ ) begin
        for ( j=0; j<LANES_PER_TILE; j++ ) begin
          rdata_valid_local[i][j] = rdata_valid_local_per_chan[0];
        end
      end
    end
  end


  always @ ( * ) begin
    for ( j=0; j<pNUM_OF_CHANS; j++ ) begin
      for ( i=0; i<4; i++ ) begin
        wr_ac_data_col_addr[j][i] = wr_ac_data[j][i][LOCAL_ADDR_WIDTH-1:0];
        wr_ac_data_row_addr[j][i] = wr_ac_data[j][i][(2*LOCAL_ADDR_WIDTH)-1:LOCAL_ADDR_WIDTH];
        wr_ac_data_bank[j][i]     = wr_ac_data[j][i][(2*LOCAL_ADDR_WIDTH)+:pBANK_WIDTH];
      end
    end
  end





  generate
    genvar geni4;
    for ( geni4=0; geni4<pNUM_OF_CHANS; ++geni4 ) begin : aa3
      for (geni2 = 0; geni2 < 8; ++geni2) begin : bb3
        for (geni3 = 0; geni3 < 4; ++geni3)
        begin: wr_data_gen
          assign wr_data_d[geni2][geni4][geni3]  = mem_data_pipe[geni4][geni3][PORT_MEM_DQ_WIDTH_PER_INTF-1+(PORT_MEM_DQ_WIDTH_PER_INTF*geni2):(PORT_MEM_DQ_WIDTH_PER_INTF*geni2)];
        end
      end
    end
  endgenerate

  always @ ( * ) begin
    for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
      for ( k=0; k<4; k++ ) begin
        for ( j=0; j<8; j++ ) begin
          rd_data_debug_d[i][k][j] =
                rd_mem_data_d[i][k][(PORT_MEM_DQ_WIDTH_PER_INTF*j)+:PORT_MEM_DQ_WIDTH_PER_INTF];
        end
      end
    end
  end

  always @ ( * ) begin
    for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
      for ( j=0; j<8; j++ ) begin
        rd_mem_data_shifted_debug_d[i][j] =
              rd_mem_data_shifted_d[i][(PORT_MEM_DQ_WIDTH_PER_INTF*j)+:PORT_MEM_DQ_WIDTH_PER_INTF];
      end
    end
  end


  initial begin
    first_wr=0;
    while ( first_wr==0 ) begin
      @ ( posedge phy_clk );
      for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
        for ( k=0; k<4; k++ ) begin
          first_wr |= wr_WRITE_en[i][k];
        end
      end
    end

    //$display("abphy : banks_per_write=%d",banks_per_write);


    repeat (20) @ ( posedge phy_clk );
    //$display("abphy : ----------------parameter values----------------------------------");
    //$display("abphy : NUM_OF_RTL_TILES        = %10d",NUM_OF_RTL_TILES);
    //$display("abphy : LANES_PER_TILE          = %10d",LANES_PER_TILE);
    //$display("abphy : PORT_MEM_DQ_WIDTH       = %10d",PORT_MEM_DQ_WIDTH);
    //$display("abphy : MEM_DATA_MASK_EN        = %10d",MEM_DATA_MASK_EN);
    //$display("abphy : USER_CLK_RATIO          = %10d",USER_CLK_RATIO);
    //$display("abphy : PHY_HMC_CLK_RATIO       = %10d",PHY_HMC_CLK_RATIO);
    //$display("abphy : PHY_PING_PONG_EN        = %10d",PHY_PING_PONG_EN);
    //$display("abphy : NUM_OF_HMC_PORTS        = %10d",NUM_OF_HMC_PORTS);
    //$display("abphy : pADDR_PIN_RATE          = %10s",pADDR_PIN_RATE);
    for ( i=0; i<pNUM_OF_CHANS; i++ ) begin
      //$display("abphy : afi_rlat CS%-d            = %10d",i,afi_rlat_sel[i]);
      //$display("abphy : afi_wlat CS%-d            = %10d",i,afi_wlat_sel[i]);
    end
    //$display("abphy :  tile=%d lane=%d %d",`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0),`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0),afi_wlat[`_abphy_get_tile(PORT_MEM_DQ_PINLOC, 0)][`_abphy_get_lane(PORT_MEM_DQ_PINLOC, 0)]);
    //$display("------------------------------------------------------------------");
  end


endmodule
