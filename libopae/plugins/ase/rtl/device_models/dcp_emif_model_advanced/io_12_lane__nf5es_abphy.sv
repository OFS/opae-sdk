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



module io_12_lane__nf5es_abphy (
   input             i50u_ref,
   input             ibp50u,
   input             ibp50u_cal,
   output            atbi_0,
   output            atbi_1,
   input             regulator_clk,
   input             reset_n,
   input       [7:0] phy_clk_phs,
   input       [4:0] phy_clk,
   input             broadcast_in_top,
   input             broadcast_in_bot,
   output            broadcast_out_top,
   output            broadcast_out_bot,
   input       [5:0] switch_up,
   input       [5:0] switch_dn,
   input       [5:0] up_ph,
   input       [5:0] dzoutx,
   output      [5:0] crnt_clk,
   output      [5:0] n_crnt_clk,
   output      [5:0] next_clk,
   output      [5:0] n_next_clk,
   output      [5:0] ioereg_locked,
   output     [11:0] weak_pullup_enable,
   output     [11:0] codin_p,
   output     [11:0] codin_pb,
   output     [11:0] codin_n,
   output     [11:0] codin_nb,
   output     [11:0] oct_enable,
   input      [23:0] dq_diff_in,
   input      [23:0] dq_sstl_in,
   input       [1:0] dqs_diff_in_0,
   input       [1:0] dqs_diff_in_1,
   input       [1:0] dqs_diff_in_2,
   input       [1:0] dqs_diff_in_3,
   input       [1:0] dqs_sstl_p_0,
   input       [1:0] dqs_sstl_p_1,
   input       [1:0] dqs_sstl_p_2,
   input       [1:0] dqs_sstl_p_3,
   input       [1:0] dqs_sstl_n_0,
   input       [1:0] dqs_sstl_n_1,
   input       [1:0] dqs_sstl_n_2,
   input       [1:0] dqs_sstl_n_3,
   input             avl_clk_in,
   output            avl_clk_out,
   input      [19:0] avl_address_in,
   output     [19:0] avl_address_out,
   input             avl_write_in,
   output            avl_write_out,
   input             avl_read_in,
   output            avl_read_out,
   input      [31:0] avl_writedata_in,
   output     [31:0] avl_writedata_out,
   input      [31:0] avl_readdata_in,
   output     [31:0] avl_readdata_out,
   input             csr_clk,
   input             csr_in,
   input             csr_en,
   input             csr_shift_n,
   output            csr_out,
   input             pll_locked,

   input             frzreg,
   input             niotri,
   input  [12*1-1:0] ncein,
   input  [12*1-1:0] nceout,
   input  [12*1-1:0] naclr,
   input  [12*1-1:0] nsclr,
   input  [12*1-1:0] fr_in_clk,
   input  [12*1-1:0] hr_in_clk,
   input  [12*1-1:0] fr_out_clk,
   input  [12*1-1:0] hr_out_clk,
   input             pll_clk,

   input             osc_sel_n,
   input             osc_en_n,
   output            osc_rocount_to_core,
   input             osc_enable_in,
   input             osc_mode_in,
   output            x1024_osc_out,
   input             test_fr_clk_en_n,
   input             test_hr_clk_en_n,
   input             test_tdf_select_n,
   input             atpg_en_n,
   input             pipeline_global_en_n,
   input             test_clr_n,
   input  [12*1-1:0] tpin,
   input  [12*1-1:0] tpctl,
   input  [12*1-1:0] progctl,
   input  [12*1-1:0] progout,
   input  [12*1-1:0] progoe,
   input             scan_shift_n,
   input  [5:0]      scanin,
   input             test_clk,
   output [5:0]      scanout,
   output [12*1-1:0] tpdata,
   output            test_xor_clk,
   input             test_pst_clk_en_n,
   input             test_phy_clk_lane_en_n,
   input             test_int_clk_en_n,
   input             test_interp_clk_en_n,
   input             test_clk_ph_buf_en_n,
   input             test_datovr_en_n,
   input             test_avl_clk_in_en_n,
   input             test_dqs_enable_en_n,
   input      [11:0] test_dbg_out,
   output     [11:0] test_dbg_in,

   input             jtag_shftdr,
   input             jtag_clk,
   input             jtag_highz,
   input             jtag_sdin,
   input             jtag_updtdr,
   input             jtag_mode,
   output            jtag_sdout,

   input      [47:0] oeb_from_core,
   input      [95:0] data_from_core,
   output     [95:0] data_to_core,
   input      [15:0] mrnk_read_core,
   input      [15:0] mrnk_write_core,
   input       [3:0] rdata_en_full_core,
   output      [3:0] rdata_valid_core,
   output      [5:0] afi_wlat_core,
   output      [5:0] afi_rlat_core,
   input             core2dbc_wr_data_vld0,
   input             core2dbc_wr_data_vld1,
   output            dbc2core_wr_data_rdy,
   output            dbc2core_rd_data_vld0,
   output            dbc2core_rd_data_vld1,
   output            dbc2core_rd_type,
   input             core2dbc_rd_data_rdy,
   input      [12:0] core2dbc_wr_ecc_info,
   output     [11:0] dbc2core_wb_pointer,
   input             test_phy_clk_en_n,
   input             dft_prbs_ena_n,
   output            dft_prbs_pass,
   output            dft_prbs_done,
   input       [7:0] dft_core2db,
   output      [7:0] dft_db2core,
   output      [1:0] dft_phy_clk,
   input             test_pst_dll_i,
   output            test_pst_dll_o,
   output            lane_cal_done,

   input      [95:0] ac_hmc,
   input             ctl2dbc_wrdata_vld0,
   input             ctl2dbc_mask_entry0,
   input             ctl2dbc_wb_rdptr_vld0,
   input       [5:0] ctl2dbc_wb_rdptr0,
   input             ctl2dbc_rb_wrptr_vld0,
   input       [5:0] ctl2dbc_rb_wrptr0,
   input       [1:0] ctl2dbc_rb_rdptr_vld0,
   input      [11:0] ctl2dbc_rb_rdptr0,
   input       [3:0] ctl2dbc_rdata_en_full0,
   input       [7:0] ctl2dbc_mrnk_read0,
   input             ctl2dbc_seq_en0,
   input             ctl2dbc_nop0,
   input       [1:0] ctl2dbc_cs0,
   input             ctl2dbc_rd_type0,
   input       [3:0] ctl2dbc_misc0,
   input             ctl2dbc_wrdata_vld1,
   input             ctl2dbc_mask_entry1,
   input             ctl2dbc_wb_rdptr_vld1,
   input       [5:0] ctl2dbc_wb_rdptr1,
   input             ctl2dbc_rb_wrptr_vld1,
   input       [5:0] ctl2dbc_rb_wrptr1,
   input       [1:0] ctl2dbc_rb_rdptr_vld1,
   input      [11:0] ctl2dbc_rb_rdptr1,
   input       [3:0] ctl2dbc_rdata_en_full1,
   input       [7:0] ctl2dbc_mrnk_read1,
   input             ctl2dbc_seq_en1,
   input             ctl2dbc_nop1,
   input       [1:0] ctl2dbc_cs1,
   input             ctl2dbc_rd_type1,
   input       [3:0] ctl2dbc_misc1,
   output            dbc2ctl_wb_retire_ptr_vld,
   output      [5:0] dbc2ctl_wb_retire_ptr,
   output            dbc2ctl_rb_retire_ptr_vld,
   output      [5:0] dbc2ctl_rb_retire_ptr,
   output            dbc2ctl_rd_data_vld,
   output            dbc2ctl_all_rd_done,
   output            dbc2db_wb_wrptr_vld,
   output      [5:0] dbc2db_wb_wrptr,
   input             cfg_dbc_ctrl_sel,
   input             cfg_reorder_rdata,
   input             cfg_rmw_en,
   input             cfg_output_regd,
   input             cfg_dbc_in_protocol,
   input             cfg_dbc_dualport_en,
   input       [2:0] cfg_dbc_pipe_lat,
   input       [2:0] cfg_cmd_rate,
   input             cfg_dbc_rc_en,
   input       [2:0] cfg_dbc_slot_rotate_en,
   input       [1:0] cfg_dbc_slot_offset,

   input             vref_ext,
`ifdef WREAL_VREF
   output real       vref_int,
`else
   output	     vref_int,
`endif
   output            xor_vref,

   input             sync_clk_bot_in,
   input             sync_data_bot_in,
   output            sync_clk_bot_out,
   output            sync_data_bot_out,
   input             sync_clk_top_in,
   input             sync_data_top_in,
   output            sync_clk_top_out,
   output            sync_data_top_out,

   input       [2:0] core_dll,
   input             clk_pll,
   input             reinit,
   input             entest,
   input             test_clk_pll_en_n,
   output     [12:0] dll_core,

   output      [1:0] lvds_rx_clk_chnl0,
   output      [1:0] lvds_tx_clk_chnl0,
   output      [1:0] lvds_rx_clk_chnl1,
   output      [1:0] lvds_tx_clk_chnl1,
   output      [1:0] lvds_rx_clk_chnl2,
   output      [1:0] lvds_tx_clk_chnl2,
   output      [1:0] lvds_rx_clk_chnl3,
   output      [1:0] lvds_tx_clk_chnl3,
   output      [1:0] lvds_rx_clk_chnl4,
   output      [1:0] lvds_tx_clk_chnl4,
   output      [1:0] lvds_rx_clk_chnl5,
   output      [1:0] lvds_tx_clk_chnl5,
   output      [2:0] fb_clkout,
   input  wire       early_csren,
   input  wire       bhniotri,
   input  wire       early_bhniotri,
   input  wire       enrnsl,
   input  wire       early_enrnsl,
   input  wire       early_frzreg,
   input  wire       nfrzdrv,
   input  wire       early_nfrzdrv,
   input  wire       early_niotri,
   input  wire       plniotri,
   input  wire       early_plniotri,
   input  wire       usrmode,
   input  wire       early_usrmode,
   input  wire       wkpullup,
   output wire       local_bhniotri,
   output wire       local_enrnsl,
   output wire       local_frzreg,
   output wire       local_nfrzdrv,
   output wire       local_niotri,
   output wire       local_plniotri,
   output wire       local_usrmode,
   output wire       local_wkpullup,
   output wire       hps_to_core_ctrl_en,
   input  wire       xprio_clk,
   input  wire       xprio_sync,
   input  wire [7:0] xprio_xbus,
   input             test_db_csr_in,
   input             test_dqs_csr_in,
   output            test_ioereg2_csr_out,
   output            test_vref_csr_out,
   input  wire   [4:0] cas_csrdin,
   output wire   [4:0] cas_csrdout,
   output            csr_clk_left,
   output            csr_en_left
);
  timeunit 1ps;
  timeprecision 1ps;



   wire       [9:0] pvt_ref_gry;
   wire      [95:0] data_from_ioreg;
   wire      [95:0] data_from_ioreg_abphy;
   wire      [47:0] avl_readchain_bot;
   wire      [47:0] avl_readchain_top;
   wire       [9:0] ioereg_csrout;
   wire       [4:0] ioereg_scanout;
   wire       [4:0] ioereg_jtag_sdout;
   wire      [19:0] avl_address_dbc;
   wire       [3:0] avl_wr_address_top;
   wire       [3:0] avl_wr_address_bot;
   wire       [3:0] avl_rd_address_top;
   wire       [3:0] avl_rd_address_bot;
   wire      [31:0] avl_writedata_dbc;
   wire      [15:0] avl_writedata_top;
   wire      [15:0] avl_writedata_bot;
   wire      [31:0] avl_readdata_dbc;
   wire       [1:0] avl_write_dbc;
   wire             avl_sync_write_top;
   wire             avl_sync_write_bot;
   wire             avl_destruct_read_top;
   wire             avl_destruct_read_bot;
   wire       [3:0] rdata_valid_local;
   wire             chk_x12_track_out_up_top;
   wire             chk_x12_track_out_up_bot;
   wire             chk_x12_track_out_dn_top;
   wire             chk_x12_track_out_dn_bot;
   wire       [1:0] fifo_pack_select_top;
   wire             fifo_read_enable_top;
   wire             fifo_reset_n_top;
   wire             fifo_rank_sel_top;
   wire       [1:0] fifo_pack_select_bot;
   wire             fifo_read_enable_bot;
   wire             fifo_reset_n_bot;
   wire             fifo_rank_sel_bot;
   wire       [5:0] pvt_write;
   wire       [2:0] pvt_address_top;
   wire       [2:0] pvt_address_bot;
   wire       [8:0] pvt_data_top;
   wire       [8:0] pvt_data_bot;
   wire       [1:0] dqs_out_a_n;
   wire       [1:0] dqs_out_b_n;
   wire       [1:0] rank_in_a_top;
   wire       [1:0] rank_in_a_bot;
   wire       [1:0] rank_in_b_top;
   wire       [1:0] rank_in_b_bot;
   wire       [7:0] rank_out_top;
   wire       [7:0] rank_out_bot;
   wire             oct_enable_out_top;
   wire             oct_enable_out_bot;
   wire             rb_phy_clk_mode_bot;
   wire       [3:0] rb_track_speed_bot;
   wire       [1:0] rb_kicker_size_bot;
   wire       [1:0] rb_mode_rate_in_bot;
   wire       [1:0] rb_mode_rate_out_bot;
   wire       [1:0] rb_filter_code_bot;
   wire             rb_phy_clk_mode_top;
   wire       [3:0] rb_track_speed_top;
   wire       [1:0] rb_kicker_size_top;
   wire       [1:0] rb_mode_rate_in_top;
   wire       [1:0] rb_mode_rate_out_top;
   wire       [1:0] rb_filter_code_top;
   wire             rb_phy_clk_mode_right;
   wire       [5:0] sel_vref;
   wire             csr_clk_top;
   wire             csr_clk_bot;
   wire             csr_clk_dbc;
   wire             csr_en_top;
   wire             csr_en_bot;
   wire             csr_en_dbc;
   wire             csr_shift_n_top;
   wire             csr_shift_n_bot;
   wire             csr_shift_n_left;
   wire             csr_shift_n_dbc;
   wire             nfrzdrv_top;
   wire             nfrzdrv_bot;
   wire             frzreg_top;
   wire             frzreg_bot;
   wire             atpg_en_n_top;
   wire             atpg_en_n_bot;
   wire             atpg_en_n_left;
   wire             pipeline_global_en_n_top;
   wire             pipeline_global_en_n_bot;
   wire             pipeline_global_en_n_left;
   wire             test_clr_n_top;
   wire             test_clr_n_bot;
   wire             test_fr_clk_en_n_top;
   wire             test_fr_clk_en_n_bot;
   wire             test_hr_clk_en_n_top;
   wire             test_hr_clk_en_n_bot;
   wire             test_tdf_select_n_top;
   wire             test_tdf_select_n_bot;
   wire             niotri_top;
   wire             niotri_bot;
   wire             test_clk_top;
   wire             test_clk_bot;
   wire             test_clk_left;
   wire             scan_shift_n_top;
   wire             scan_shift_n_bot;
   wire             scan_shift_n_dbc;
   wire             scan_shift_n_left;
   wire             jtag_shftdr_top;
   wire             jtag_shftdr_bot;
   wire             jtag_clk_top;
   wire             jtag_clk_bot;
   wire             jtag_highz_top;
   wire             jtag_highz_bot;
   wire             jtag_updtdr_top;
   wire             jtag_updtdr_bot;
   wire             jtag_mode_top;
   wire             jtag_mode_bot;
   wire      [12:0] avl_select_ioereg;
   wire      [47:0] oeb_to_ioreg;
   wire      [95:0] data_to_ioreg;
   wire      [11:0] chk_x1_track_in_up;
   wire      [11:0] chk_x1_track_in_dn;
   wire      [11:0] chk_x1_track_out_up;
   wire      [11:0] chk_x1_track_out_dn;
   wire             chk_x12_track_in_up;
   wire             chk_x12_track_in_dn;
   wire             chk_x12_track_out_up;
   wire             chk_x12_track_out_dn;
   wire       [3:0] rdata_en_full_local;
   wire       [7:0] mrnk_read_local;
   wire       [7:0] mrnk_write_local;
   wire      [23:0] dqs_clk_a;
   wire      [23:0] dqs_clk_b;
   wire       [5:0] xor_clk;
   wire             test_phy_clk_lane_en_n_top;
   wire             test_phy_clk_lane_en_n_bot;
   wire             test_int_clk_en_n_top;
   wire             test_int_clk_en_n_bot;
   wire             test_interp_clk_en_n_top;
   wire             test_interp_clk_en_n_bot;
   wire             test_clk_ph_buf_en_n_top;
   wire             test_clk_ph_buf_en_n_bot;
   wire             test_datovr_en_n_top;
   wire             test_datovr_en_n_bot;
   wire      [11:0] dqs_probe;
   wire      [35:0] rd_probe;
   wire      [11:0] link_gpio_dout_n;
   wire      [11:0] link_gpio_oeb;
   wire      [11:0] select_ac_hmc;
   wire      [23:0] dq_in_del_n;
   wire      [23:0] dq_in_tree;
   wire      [11:0] test_dbg_out_local;
   wire      [5:0] sync_clk_up_chn;
   wire      [5:0] sync_data_up_chn;
   wire      [5:0] sync_clk_dn_chn;
   wire      [5:0] sync_data_dn_chn;
   wire      [7:0] x64_osc_chain_p;
   wire      [7:0] x64_osc_chain_n;
   wire             x64_osc_mode_out;

   wire      [3:0] interpolator_clk_0;
   wire      [3:0] d_out_p_mux_0;
   wire      [3:0] d_out_n_mux_0;
   wire      [5:0] rb_dq_select_0;
   wire      [1:0] datovr_0;
   wire      [1:0] datovrb_0;
   wire      [1:0] data_dq_0;
   wire      [1:0] data_dqb_0;
   wire      [1:0] struct_latch_open_n_0;
   wire      [1:0] nfrzdrv_struct_0;

   wire      [3:0] interpolator_clk_1;
   wire      [3:0] d_out_p_mux_1;
   wire      [3:0] d_out_n_mux_1;
   wire      [5:0] rb_dq_select_1;
   wire      [1:0] datovr_1;
   wire      [1:0] datovrb_1;
   wire      [1:0] data_dq_1;
   wire      [1:0] data_dqb_1;
   wire      [1:0] struct_latch_open_n_1;
   wire      [1:0] nfrzdrv_struct_1;
   wire      [3:0] interpolator_clk_2;
   wire      [3:0] d_out_p_mux_2;
   wire      [3:0] d_out_n_mux_2;
   wire      [5:0] rb_dq_select_2;
   wire      [1:0] datovr_2;
   wire      [1:0] datovrb_2;
   wire      [1:0] data_dq_2;
   wire      [1:0] data_dqb_2;
   wire      [1:0] struct_latch_open_n_2;
   wire      [1:0] nfrzdrv_struct_2;
   wire      [3:0] interpolator_clk_3;
   wire      [3:0] d_out_p_mux_3;
   wire      [3:0] d_out_n_mux_3;
   wire      [5:0] rb_dq_select_3;
   wire      [1:0] datovr_3;
   wire      [1:0] datovrb_3;
   wire      [1:0] data_dq_3;
   wire      [1:0] data_dqb_3;
   wire      [1:0] struct_latch_open_n_3;
   wire      [1:0] nfrzdrv_struct_3;
   wire      [3:0] interpolator_clk_4;
   wire      [3:0] d_out_p_mux_4;
   wire      [3:0] d_out_n_mux_4;
   wire      [5:0] rb_dq_select_4;
   wire      [1:0] datovr_4;
   wire      [1:0] datovrb_4;
   wire      [1:0] data_dq_4;
   wire      [1:0] data_dqb_4;
   wire      [1:0] struct_latch_open_n_4;
   wire      [1:0] nfrzdrv_struct_4;
   wire      [3:0] interpolator_clk_5;
   wire      [3:0] d_out_p_mux_5;
   wire      [3:0] d_out_n_mux_5;
   wire      [5:0] rb_dq_select_5;
   wire      [1:0] datovr_5;
   wire      [1:0] datovrb_5;
   wire      [1:0] data_dq_5;
   wire      [1:0] data_dqb_5;
   wire      [1:0] struct_latch_open_n_5;
   wire      [1:0] nfrzdrv_struct_5;

   wire     [5:1] osc_in_up;
   wire     [5:0] osc_out_dn;
   wire            osc_start_in;
   wire            osc_start_out;

   wire            ioereg_reset_n;
   wire [1:0]      dqs_loop_back_0;
   wire [1:0]      dqs_loop_back_1;

   wire            dll_test_si1;
   wire            dll_test_si2;
   wire            dll_test_so1;
   wire            dll_test_so2;

   wire            xpr_clk_bot;
   wire            xpr_clk_top;
   wire      [2:0] xpr_write_bot;
   wire      [2:0] xpr_write_top;
   wire      [7:0] xpr_data_bot;
   wire      [7:0] xpr_data_top;
   wire            test_i_n_dll;
   wire            test_i_p_pst;
   wire            test_dqs_o1;
   wire            test_dqs_o2;

   wire            vcc_regphy;
   wire            db_csrin;
   wire            dqs_csrin;

   wire             afi_cal_success;

io_dft_mux__nf5es xdb_csrin_mux (
.testin (test_db_csr_in),
.usrin (csr_in),
.s (scan_shift_n),
.out (db_csrin)
);

io_dft_mux__nf5es xdqs_csrin_mux (
.testin (test_dqs_csr_in),
.usrin ( ioereg_csrout[3]),
.s (scan_shift_n),
.out (dqs_csrin)

);

assign test_ioereg2_csr_out = ioereg_csrout[3];
assign test_vref_csr_out = csr_out;

io_ioereg_top__nf5es ioereg_top_0_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[1:0]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[1:0]       ),
.core_data_out          ( data_to_ioreg[15:0]            ),
.core_oeb               ( oeb_to_ioreg[7:0]              ),
.core_data_in           ( data_from_ioreg[15:0]          ),
.avl_select             ( avl_select_ioereg[1:0]         ),
.dpa_track_out_up       ( switch_up[0]                   ),
.dpa_track_out_dn       ( switch_dn[0]                   ),
.phase_kicker           ( dzoutx[0]                      ),
.phase_direction        ( up_ph[0]                       ),
.interpolator_clk_p     ( {next_clk[0],crnt_clk[0]}      ),
.interpolator_clk_n     ( {n_next_clk[0],n_crnt_clk[0]}  ),
.codin_p                ( codin_p[1:0]                   ),
.codin_pb               ( codin_pb[1:0]                  ),
.codin_n                ( codin_n[1:0]                   ),
.codin_nb               ( codin_nb[1:0]                  ),
.dq_diff_in             ( dq_diff_in[3:0]                ),
.dq_sstl_in             ( dq_sstl_in[3:0]                ),
.dqs_clk_a              ( dqs_clk_a[3:0]                 ),
.dqs_clk_b              ( dqs_clk_b[3:0]                 ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_bot       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_bot       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_bot            ),
.rb_track_speed         ( rb_track_speed_bot[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_bot[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_bot[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_bot[1:0]      ),
.rb_filter_code         ( rb_filter_code_bot[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_bot[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_bot           ),
.fifo_reset_n           ( fifo_reset_n_bot               ),
.fifo_rank_sel          ( fifo_rank_sel_bot              ),
.rank_in_a              ( rank_in_a_bot[1:0]             ),
.rank_in_b              ( rank_in_b_bot[1:0]             ),
.rank_out               ( rank_out_bot[7:0]              ),
.oct_enable_out         ( oct_enable_out_bot             ),
.oct_enable_pin         ( oct_enable[1:0]                ),
.avl_wr_address         ( avl_wr_address_bot[3:0]        ),
.avl_rd_address         ( avl_rd_address_bot[3:0]        ),
.avl_sync_write         ( avl_sync_write_bot             ),
.avl_destruct_read      ( avl_destruct_read_bot          ),
.avl_writedata          ( avl_writedata_bot[15:0]        ),
.avl_readchain_in       ( 16'h0000                       ),
.avl_readchain_out      ( avl_readchain_bot[15:0]        ),
.pvt_write              ( pvt_write[0]                   ),
.pvt_address            ( pvt_address_bot[2:0]           ),
.pvt_data               ( pvt_data_bot[8:0]              ),
.csr_clk                ( csr_clk_bot                    ),
.csr_in                 ( ioereg_csrout[0]               ),
.csr_en                 ( csr_en_bot                     ),
.csr_shift_n            ( 1'b1                           ),
.csr_out                ( ioereg_csrout[1]               ),
.nfrzdrv                ( nfrzdrv_bot                    ),
.frzreg                 ( frzreg_bot                     ),
.ncein                  ( ncein[1:0]                     ),
.nceout                 ( nceout[1:0]                    ),
.naclr                  ( naclr[1:0]                     ),
.nsclr                  ( nsclr[1:0]                     ),
.fr_in_clk              ( fr_in_clk[1:0]                 ),
.hr_in_clk              ( hr_in_clk[1:0]                 ),
.fr_out_clk             ( fr_out_clk[1:0]                ),
.hr_out_clk             ( hr_out_clk[1:0]                ),
.pll_clk                ( 1'b0                           ),
.atpg_en_n              ( atpg_en_n_bot                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_bot       ),
.test_clr_n             ( test_clr_n_bot                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_bot           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_bot           ),
.test_tdf_select_n      ( test_tdf_select_n_bot          ),
.tpin                   ( tpin[1:0]                      ),
.tpctl                  ( tpctl[1:0]                     ),
.tpdata                 ( tpdata[1:0]                    ),
.niotri                 ( niotri_bot                     ),
.progctl                ( progctl[1:0]                   ),
.progout                ( progout[1:0]                   ),
.progoe                 ( progoe[1:0]                    ),
.test_clk               ( test_clk_bot                   ),
.scan_shift_n           ( scan_shift_n_bot               ),
.scan_in                ( scanin[0]                      ),
.scan_out               ( scanout[0]          		 ),
.xor_clk                ( xor_clk[0]                     ),
.test_dbg_out           ( test_dbg_out_local[1:0]        ),
.test_dbg_in            ( test_dbg_in[1:0]               ),
.probe_out              ( {dqs_probe[1:0],rd_probe[17:0]}),
.read_path_probe        ( rd_probe[5:0]                  ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_bot     ),
.test_int_clk_en_n      ( test_int_clk_en_n_bot          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_bot       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_bot       ),
.test_datovr_en_n       ( test_datovr_en_n_bot           ),
.jtag_shftdr            ( jtag_shftdr_bot                ),
.jtag_clk               ( jtag_clk_bot                   ),
.jtag_highz             ( jtag_highz_bot                 ),
.jtag_sdin              ( jtag_sdin                      ),
.jtag_updtdr            ( jtag_updtdr_bot                ),
.jtag_mode              ( jtag_mode_bot                  ),
.jtag_sdout             ( ioereg_jtag_sdout[0]           ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_out_dn[1]                  ),
.osc_in_up              ( osc_out_dn[0]                  ),
.osc_out_dn             ( osc_out_dn[0]                  ),
.osc_out_up             ( osc_in_up[1]                   ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[0]             ),
.x64_osc_in_n           ( x64_osc_chain_n[0]             ),
.x64_osc_out_p          ( x64_osc_chain_p[1]             ),
.x64_osc_out_n          ( x64_osc_chain_n[1]             ),
.sync_clk_bot_in        ( sync_clk_bot_in                ),
.sync_data_bot_in       ( sync_data_bot_in               ),
.sync_clk_bot_out       ( sync_clk_bot_out               ),
.sync_data_bot_out      ( sync_data_bot_out              ),
.sync_clk_top_in        ( sync_clk_dn_chn[0]             ),
.sync_data_top_in       ( sync_data_dn_chn[0]            ),
.sync_clk_top_out       ( sync_clk_up_chn[0]             ),
.sync_data_top_out      ( sync_data_up_chn[0]            ),
.interpolator_clk_out   ( interpolator_clk_0[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_0[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_0[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_0[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_0[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_0[3:0]             ),
.rb_dq_select           ( rb_dq_select_0[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_0[5:0]            ),
.datovr_out             ( datovr_0[1:0]                  ),
.datovr_in              ( datovr_0[1:0]                  ),
.datovrb_out            ( datovrb_0[1:0]                 ),
.datovrb_in             ( datovrb_0[1:0]                 ),
.data_dq_out            ( data_dq_0[1:0]                 ),
.data_dq_in             ( data_dq_0[1:0]                 ),
.data_dqb_out           ( data_dqb_0[1:0]                ),
.data_dqb_in            ( data_dqb_0[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_0[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_0[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_0[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_0[1:0]  	 ),
.ac_hmc                 ( ac_hmc[15:0]                   ),
.select_ac_hmc          ( select_ac_hmc[1:0]             ),
.dq_in_del              ( dq_in_del_n[3:0]               ),
.dq_in_tree             ( dq_in_tree[3:0]                ),
.dqs_loop_back          (                                ),
.xpr_clk                ( xpr_clk_bot                    ),
.xpr_write              ( xpr_write_bot[0]               ),
.xpr_data               ( xpr_data_bot[7:0]              ),
.ioereg_locked          ( ioereg_locked[0]               ),
.weak_pullup_enable     ( weak_pullup_enable[1:0]        )
);

io_ioereg_top__nf5es ioereg_top_1_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[3:2]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[3:2]       ),
.core_data_out          ( data_to_ioreg[31:16]           ),
.core_oeb               ( oeb_to_ioreg[15:8]             ),
.core_data_in           ( data_from_ioreg[31:16]         ),
.avl_select             ( avl_select_ioereg[3:2]         ),
.dpa_track_out_up       ( switch_up[1]                   ),
.dpa_track_out_dn       ( switch_dn[1]                   ),
.phase_kicker           ( dzoutx[1]                      ),
.phase_direction        ( up_ph[1]                       ),
.interpolator_clk_p     ( {next_clk[1],crnt_clk[1]}      ),
.interpolator_clk_n     ( {n_next_clk[1],n_crnt_clk[1]}  ),
.codin_p                ( codin_p[3:2]                   ),
.codin_pb               ( codin_pb[3:2]                  ),
.codin_n                ( codin_n[3:2]                   ),
.codin_nb               ( codin_nb[3:2]                  ),
.dq_diff_in             ( dq_diff_in[7:4]                ),
.dq_sstl_in             ( dq_sstl_in[7:4]                ),
.dqs_clk_a              ( dqs_clk_a[7:4]                 ),
.dqs_clk_b              ( dqs_clk_b[7:4]                 ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_bot       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_bot       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_bot            ),
.rb_track_speed         ( rb_track_speed_bot[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_bot[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_bot[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_bot[1:0]      ),
.rb_filter_code         ( rb_filter_code_bot[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_bot[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_bot           ),
.fifo_reset_n           ( fifo_reset_n_bot               ),
.fifo_rank_sel          ( fifo_rank_sel_bot              ),
.rank_in_a              ( rank_in_a_bot[1:0]             ),
.rank_in_b              ( rank_in_b_bot[1:0]             ),
.rank_out               ( rank_out_bot[7:0]              ),
.oct_enable_out         ( oct_enable_out_bot             ),
.oct_enable_pin         ( oct_enable[3:2]                ),
.avl_wr_address         ( avl_wr_address_bot[3:0]        ),
.avl_rd_address         ( avl_rd_address_bot[3:0]        ),
.avl_sync_write         ( avl_sync_write_bot             ),
.avl_destruct_read      ( avl_destruct_read_bot          ),
.avl_writedata          ( avl_writedata_bot[15:0]        ),
.avl_readchain_in       ( avl_readchain_bot[15:0]        ),
.avl_readchain_out      ( avl_readchain_bot[31:16]       ),
.pvt_write              ( pvt_write[1]                   ),
.pvt_address            ( pvt_address_bot[2:0]           ),
.pvt_data               ( pvt_data_bot[8:0]              ),
.csr_clk                ( csr_clk_bot                    ),
.csr_in                 ( ioereg_csrout[1]               ),
.csr_en                 ( csr_en_bot                     ),
.csr_shift_n            ( 1'b1                           ),
.csr_out                ( ioereg_csrout[2]               ),
.nfrzdrv                ( nfrzdrv_bot                    ),
.frzreg                 ( frzreg_bot                     ),
.ncein                  ( ncein[3:2]                     ),
.nceout                 ( nceout[3:2]                    ),
.naclr                  ( naclr[3:2]                     ),
.nsclr                  ( nsclr[3:2]                     ),
.fr_in_clk              ( fr_in_clk[3:2]                 ),
.hr_in_clk              ( hr_in_clk[3:2]                 ),
.fr_out_clk             ( fr_out_clk[3:2]                ),
.hr_out_clk             ( hr_out_clk[3:2]                ),
.pll_clk                ( 1'b0                           ),
.atpg_en_n              ( atpg_en_n_bot                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_bot       ),
.test_clr_n             ( test_clr_n_bot                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_bot           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_bot           ),
.test_tdf_select_n      ( test_tdf_select_n_bot          ),
.tpin                   ( tpin[3:2]                      ),
.tpctl                  ( tpctl[3:2]                     ),
.tpdata                 ( tpdata[3:2]                    ),
.niotri                 ( niotri_bot                     ),
.progctl                ( progctl[3:2]                   ),
.progout                ( progout[3:2]                   ),
.progoe                 ( progoe[3:2]                    ),
.test_clk               ( test_clk_bot                   ),
.scan_shift_n           ( scan_shift_n_bot               ),
.scan_in                ( scanin[1]              	 ),
.scan_out               ( scanout[1]             	 ),
.xor_clk                ( xor_clk[1]                     ),
.test_dbg_out           ( test_dbg_out_local[3:2]        ),
.test_dbg_in            ( test_dbg_in[3:2]               ),
.probe_out              ( {dqs_probe[3:2],rd_probe[17:0]}),
.read_path_probe        ( rd_probe[11:6]                 ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_bot     ),
.test_int_clk_en_n      ( test_int_clk_en_n_bot          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_bot       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_bot       ),
.test_datovr_en_n       ( test_datovr_en_n_bot           ),
.jtag_shftdr            ( jtag_shftdr_bot                ),
.jtag_clk               ( jtag_clk_bot                   ),
.jtag_highz             ( jtag_highz_bot                 ),
.jtag_sdin              ( ioereg_jtag_sdout[0]           ),
.jtag_updtdr            ( jtag_updtdr_bot                ),
.jtag_mode              ( jtag_mode_bot                  ),
.jtag_sdout             ( ioereg_jtag_sdout[1]           ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_out_dn[2]                  ),
.osc_in_up              ( osc_in_up[1]                   ),
.osc_out_dn             ( osc_out_dn[1]                  ),
.osc_out_up             ( osc_in_up[2]                   ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[1]             ),
.x64_osc_in_n           ( x64_osc_chain_n[1]             ),
.x64_osc_out_p          ( x64_osc_chain_p[2]             ),
.x64_osc_out_n          ( x64_osc_chain_n[2]             ),
.sync_clk_bot_in        ( sync_clk_up_chn[0]             ),
.sync_data_bot_in       ( sync_data_up_chn[0]            ),
.sync_clk_bot_out       ( sync_clk_dn_chn[0]             ),
.sync_data_bot_out      ( sync_data_dn_chn[0]            ),
.sync_clk_top_in        ( sync_clk_dn_chn[1]             ),
.sync_data_top_in       ( sync_data_dn_chn[1]            ),
.sync_clk_top_out       ( sync_clk_up_chn[1]             ),
.sync_data_top_out      ( sync_data_up_chn[1]            ),
.interpolator_clk_out   ( interpolator_clk_1[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_1[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_1[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_1[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_1[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_1[3:0]             ),
.rb_dq_select           ( rb_dq_select_1[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_1[5:0]            ),
.datovr_out             ( datovr_1[1:0]                  ),
.datovr_in              ( datovr_1[1:0]                  ),
.datovrb_out            ( datovrb_1[1:0]                 ),
.datovrb_in             ( datovrb_1[1:0]                 ),
.data_dq_out            ( data_dq_1[1:0]                 ),
.data_dq_in             ( data_dq_1[1:0]                 ),
.data_dqb_out           ( data_dqb_1[1:0]                ),
.data_dqb_in            ( data_dqb_1[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_1[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_1[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_1[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_1[1:0]  	 ),
.ac_hmc                 ( ac_hmc[31:16]                  ),
.select_ac_hmc          ( select_ac_hmc[3:2]             ),
.dq_in_del              ( dq_in_del_n[7:4]               ),
.dq_in_tree             ( dq_in_tree[7:4]                ),
.dqs_loop_back          (                                ),
.xpr_clk                ( xpr_clk_bot                    ),
.xpr_write              ( xpr_write_bot[1]               ),
.xpr_data               ( xpr_data_bot[7:0]              ),
.ioereg_locked          ( ioereg_locked[1]               ),
.weak_pullup_enable     ( weak_pullup_enable[3:2]        )
);

io_ioereg_top__nf5es ioereg_top_2_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[5:4]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[5:4]       ),
.core_data_out          ( data_to_ioreg[47:32]           ),
.core_oeb               ( oeb_to_ioreg[23:16]            ),
.core_data_in           ( data_from_ioreg[47:32]         ),
.avl_select             ( avl_select_ioereg[5:4]         ),
.dpa_track_out_up       ( switch_up[2]                   ),
.dpa_track_out_dn       ( switch_dn[2]                   ),
.phase_kicker           ( dzoutx[2]                      ),
.phase_direction        ( up_ph[2]                       ),
.interpolator_clk_p     ( {next_clk[2],crnt_clk[2]}      ),
.interpolator_clk_n     ( {n_next_clk[2],n_crnt_clk[2]}  ),
.codin_p                ( codin_p[5:4]                   ),
.codin_pb               ( codin_pb[5:4]                  ),
.codin_n                ( codin_n[5:4]                   ),
.codin_nb               ( codin_nb[5:4]                  ),
.dq_diff_in             ( dq_diff_in[11:8]               ),
.dq_sstl_in             ( dq_sstl_in[11:8]               ),
.dqs_clk_a              ( dqs_clk_a[11:8]                ),
.dqs_clk_b              ( dqs_clk_b[11:8]                ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_bot       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_bot       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_bot            ),
.rb_track_speed         ( rb_track_speed_bot[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_bot[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_bot[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_bot[1:0]      ),
.rb_filter_code         ( rb_filter_code_bot[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_bot[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_bot           ),
.fifo_reset_n           ( fifo_reset_n_bot               ),
.fifo_rank_sel          ( fifo_rank_sel_bot              ),
.rank_in_a              ( rank_in_a_bot[1:0]             ),
.rank_in_b              ( rank_in_b_bot[1:0]             ),
.rank_out               ( rank_out_bot[7:0]              ),
.oct_enable_out         ( oct_enable_out_bot             ),
.oct_enable_pin         ( oct_enable[5:4]                ),
.avl_wr_address         ( avl_wr_address_bot[3:0]        ),
.avl_rd_address         ( avl_rd_address_bot[3:0]        ),
.avl_sync_write         ( avl_sync_write_bot             ),
.avl_destruct_read      ( avl_destruct_read_bot          ),
.avl_writedata          ( avl_writedata_bot[15:0]        ),
.avl_readchain_in       ( avl_readchain_bot[31:16]       ),
.avl_readchain_out      ( avl_readchain_bot[47:32]       ),
.pvt_write              ( pvt_write[2]                   ),
.pvt_address            ( pvt_address_bot[2:0]           ),
.pvt_data               ( pvt_data_bot[8:0]              ),
.csr_clk                ( csr_clk_bot                    ),
.csr_in                 ( ioereg_csrout[2]               ),
.csr_en                 ( csr_en_bot                     ),
.csr_shift_n            ( csr_shift_n_bot                ),
.csr_out                ( ioereg_csrout[3]               ),
.nfrzdrv                ( nfrzdrv_bot                    ),
.frzreg                 ( frzreg_bot                     ),
.ncein                  ( ncein[5:4]                     ),
.nceout                 ( nceout[5:4]                    ),
.naclr                  ( naclr[5:4]                     ),
.nsclr                  ( nsclr[5:4]                     ),
.fr_in_clk              ( fr_in_clk[5:4]                 ),
.hr_in_clk              ( hr_in_clk[5:4]                 ),
.fr_out_clk             ( fr_out_clk[5:4]                ),
.hr_out_clk             ( hr_out_clk[5:4]                ),
.pll_clk                ( pll_clk                        ),
.atpg_en_n              ( atpg_en_n_bot                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_bot       ),
.test_clr_n             ( test_clr_n_bot                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_bot           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_bot           ),
.test_tdf_select_n      ( test_tdf_select_n_bot          ),
.tpin                   ( tpin[5:4]                      ),
.tpctl                  ( tpctl[5:4]                     ),
.tpdata                 ( tpdata[5:4]                    ),
.niotri                 ( niotri_bot                     ),
.progctl                ( progctl[5:4]                   ),
.progout                ( progout[5:4]                   ),
.progoe                 ( progoe[5:4]                    ),
.test_clk               ( test_clk_bot                   ),
.scan_shift_n           ( scan_shift_n_bot               ),
.scan_in                ( scanin[2]  	                 ),
.scan_out               ( scanout[2]                     ),
.xor_clk                ( xor_clk[2]                     ),
.test_dbg_out           ( test_dbg_out_local[5:4]        ),
.test_dbg_in            ( test_dbg_in[5:4]               ),
.probe_out              ( {dqs_probe[5:4],rd_probe[17:0]}),
.read_path_probe        ( rd_probe[17:12]                ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_bot     ),
.test_int_clk_en_n      ( test_int_clk_en_n_bot          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_bot       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_bot       ),
.test_datovr_en_n       ( test_datovr_en_n_bot           ),
.jtag_shftdr            ( jtag_shftdr_bot                ),
.jtag_clk               ( jtag_clk_bot                   ),
.jtag_highz             ( jtag_highz_bot                 ),
.jtag_sdin              ( ioereg_jtag_sdout[1]           ),
.jtag_updtdr            ( jtag_updtdr_bot                ),
.jtag_mode              ( jtag_mode_bot                  ),
.jtag_sdout             ( ioereg_jtag_sdout[2]           ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_out_dn[3]                  ),
.osc_in_up              ( osc_in_up[2]                   ),
.osc_out_dn             ( osc_out_dn[2]                  ),
.osc_out_up             ( osc_in_up[3]                   ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[2]             ),
.x64_osc_in_n           ( x64_osc_chain_n[2]             ),
.x64_osc_out_p          ( x64_osc_chain_p[3]             ),
.x64_osc_out_n          ( x64_osc_chain_n[3]             ),
.sync_clk_bot_in        ( sync_clk_up_chn[1]             ),
.sync_data_bot_in       ( sync_data_up_chn[1]            ),
.sync_clk_bot_out       ( sync_clk_dn_chn[1]             ),
.sync_data_bot_out      ( sync_data_dn_chn[1]            ),
.sync_clk_top_in        ( sync_clk_dn_chn[2]             ),
.sync_data_top_in       ( sync_data_dn_chn[2]            ),
.sync_clk_top_out       ( sync_clk_up_chn[2]             ),
.sync_data_top_out      ( sync_data_up_chn[2]            ),
.interpolator_clk_out   ( interpolator_clk_2[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_2[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_2[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_2[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_2[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_2[3:0]             ),
.rb_dq_select           ( rb_dq_select_2[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_2[5:0]            ),
.datovr_out             ( datovr_2[1:0]                  ),
.datovr_in              ( datovr_2[1:0]                  ),
.datovrb_out            ( datovrb_2[1:0]                 ),
.datovrb_in             ( datovrb_2[1:0]                 ),
.data_dq_out            ( data_dq_2[1:0]                 ),
.data_dq_in             ( data_dq_2[1:0]                 ),
.data_dqb_out           ( data_dqb_2[1:0]                ),
.data_dqb_in            ( data_dqb_2[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_2[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_2[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_2[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_2[1:0]  	 ),
.ac_hmc                 ( ac_hmc[47:32]                  ),
.select_ac_hmc          ( select_ac_hmc[5:4]             ),
.dq_in_del              ( dq_in_del_n[11:8]              ),
.dq_in_tree             ( dq_in_tree[11:8]               ),
.dqs_loop_back          ( dqs_loop_back_0[1:0]           ),
.xpr_clk                ( xpr_clk_bot                    ),
.xpr_write              ( xpr_write_bot[2]               ),
.xpr_data               ( xpr_data_bot[7:0]              ),
.ioereg_locked          ( ioereg_locked[2]               ),
.weak_pullup_enable     ( weak_pullup_enable[5:4]        )
);

io_ioereg_top__nf5es ioereg_top_3_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[7:6]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[7:6]       ),
.core_data_out          ( data_to_ioreg[63:48]           ),
.core_oeb               ( oeb_to_ioreg[31:24]            ),
.core_data_in           ( data_from_ioreg[63:48]         ),
.avl_select             ( avl_select_ioereg[7:6]         ),
.dpa_track_out_up       ( switch_up[3]                   ),
.dpa_track_out_dn       ( switch_dn[3]                   ),
.phase_kicker           ( dzoutx[3]                      ),
.phase_direction        ( up_ph[3]                       ),
.interpolator_clk_p     ( {next_clk[3],crnt_clk[3]}      ),
.interpolator_clk_n     ( {n_next_clk[3],n_crnt_clk[3]}  ),
.codin_p                ( codin_p[7:6]                   ),
.codin_pb               ( codin_pb[7:6]                  ),
.codin_n                ( codin_n[7:6]                   ),
.codin_nb               ( codin_nb[7:6]                  ),
.dq_diff_in             ( dq_diff_in[15:12]               ),
.dq_sstl_in             ( dq_sstl_in[15:12]               ),
.dqs_clk_a              ( dqs_clk_a[15:12]                ),
.dqs_clk_b              ( dqs_clk_b[15:12]                ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_top       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_top       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_top            ),
.rb_track_speed         ( rb_track_speed_top[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_top[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_top[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_top[1:0]      ),
.rb_filter_code         ( rb_filter_code_top[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_top[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_top           ),
.fifo_reset_n           ( fifo_reset_n_top               ),
.fifo_rank_sel          ( fifo_rank_sel_top              ),
.rank_in_a              ( rank_in_a_top[1:0]             ),
.rank_in_b              ( rank_in_b_top[1:0]             ),
.rank_out               ( rank_out_top[7:0]              ),
.oct_enable_out         ( oct_enable_out_top             ),
.oct_enable_pin         ( oct_enable[7:6]                ),
.avl_wr_address         ( avl_wr_address_top[3:0]        ),
.avl_rd_address         ( avl_rd_address_top[3:0]        ),
.avl_sync_write         ( avl_sync_write_top             ),
.avl_destruct_read      ( avl_destruct_read_top          ),
.avl_writedata          ( avl_writedata_top[15:0]        ),
.avl_readchain_in       ( avl_readchain_top[31:16]       ),
.avl_readchain_out      ( avl_readchain_top[47:32]       ),
.pvt_write              ( pvt_write[3]                   ),
.pvt_address            ( pvt_address_top[2:0]           ),
.pvt_data               ( pvt_data_top[8:0]              ),
.csr_clk                ( csr_clk_top                    ),
.csr_in                 ( ioereg_csrout[4]               ),
.csr_en                 ( csr_en_top                     ),
.csr_shift_n            ( 1'b1                           ),
.csr_out                ( ioereg_csrout[5]               ),
.nfrzdrv                ( nfrzdrv_top                    ),
.frzreg                 ( frzreg_top                     ),
.ncein                  ( ncein[7:6]                     ),
.nceout                 ( nceout[7:6]                    ),
.naclr                  ( naclr[7:6]                     ),
.nsclr                  ( nsclr[7:6]                     ),
.fr_in_clk              ( fr_in_clk[7:6]                 ),
.hr_in_clk              ( hr_in_clk[7:6]                 ),
.fr_out_clk             ( fr_out_clk[7:6]                ),
.hr_out_clk             ( hr_out_clk[7:6]                ),
.pll_clk                ( pll_clk                        ),
.atpg_en_n              ( atpg_en_n_top                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_top       ),
.test_clr_n             ( test_clr_n_top                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_top           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_top           ),
.test_tdf_select_n      ( test_tdf_select_n_top          ),
.tpin                   ( tpin[7:6]                      ),
.tpctl                  ( tpctl[7:6]                     ),
.tpdata                 ( tpdata[7:6]                    ),
.niotri                 ( niotri_top                     ),
.progctl                ( progctl[7:6]                   ),
.progout                ( progout[7:6]                   ),
.progoe                 ( progoe[7:6]                    ),
.test_clk               ( test_clk_top                   ),
.scan_shift_n           ( scan_shift_n_top               ),
.scan_in                ( scanin[3]             	 ),
.scan_out               ( scanout[3]             	 ),
.xor_clk                ( xor_clk[3]                     ),
.test_dbg_out           ( test_dbg_out_local[7:6]        ),
.test_dbg_in            ( test_dbg_in[7:6]               ),
.probe_out              ( {dqs_probe[7:6],rd_probe[35:18]} ),
.read_path_probe        ( rd_probe[23:18]                ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_top     ),
.test_int_clk_en_n      ( test_int_clk_en_n_top          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_top       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_top       ),
.test_datovr_en_n       ( test_datovr_en_n_top           ),
.jtag_shftdr            ( jtag_shftdr_top                ),
.jtag_clk               ( jtag_clk_top                   ),
.jtag_highz             ( jtag_highz_top                 ),
.jtag_sdin              ( ioereg_jtag_sdout[2]           ),
.jtag_updtdr            ( jtag_updtdr_top                ),
.jtag_mode              ( jtag_mode_top                  ),
.jtag_sdout             ( ioereg_jtag_sdout[3]           ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_out_dn[4]                  ),
.osc_in_up              ( osc_in_up[3]                   ),
.osc_out_dn             ( osc_out_dn[3]                  ),
.osc_out_up             ( osc_in_up[4]                   ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[3]             ),
.x64_osc_in_n           ( x64_osc_chain_n[3]             ),
.x64_osc_out_p          ( x64_osc_chain_p[4]             ),
.x64_osc_out_n          ( x64_osc_chain_n[4]             ),
.sync_clk_bot_in        ( sync_clk_up_chn[3]             ),
.sync_data_bot_in       ( sync_data_up_chn[3]            ),
.sync_clk_bot_out       ( sync_clk_dn_chn[3]             ),
.sync_data_bot_out      ( sync_data_dn_chn[3]            ),
.sync_clk_top_in        ( sync_clk_dn_chn[4]             ),
.sync_data_top_in       ( sync_data_dn_chn[4]            ),
.sync_clk_top_out       ( sync_clk_up_chn[4]             ),
.sync_data_top_out      ( sync_data_up_chn[4]            ),
.interpolator_clk_out   ( interpolator_clk_3[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_3[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_3[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_3[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_3[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_3[3:0]             ),
.rb_dq_select           ( rb_dq_select_3[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_3[5:0]            ),
.datovr_out             ( datovr_3[1:0]                  ),
.datovr_in              ( datovr_3[1:0]                  ),
.datovrb_out            ( datovrb_3[1:0]                 ),
.datovrb_in             ( datovrb_3[1:0]                 ),
.data_dq_out            ( data_dq_3[1:0]                 ),
.data_dq_in             ( data_dq_3[1:0]                 ),
.data_dqb_out           ( data_dqb_3[1:0]                ),
.data_dqb_in            ( data_dqb_3[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_3[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_3[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_3[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_3[1:0]  	 ),
.ac_hmc                 ( ac_hmc[63:48]                  ),
.select_ac_hmc          ( select_ac_hmc[7:6]             ),
.dq_in_del              ( dq_in_del_n[15:12]             ),
.dq_in_tree             ( dq_in_tree[15:12]              ),
.dqs_loop_back          ( dqs_loop_back_1[1:0]           ),
.xpr_clk                ( xpr_clk_top                    ),
.xpr_write              ( xpr_write_top[0]               ),
.xpr_data               ( xpr_data_top[7:0]              ),
.ioereg_locked          ( ioereg_locked[3]               ),
.weak_pullup_enable     ( weak_pullup_enable[7:6]        )
);

io_ioereg_top__nf5es ioereg_top_4_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[9:8]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[9:8]       ),
.core_data_out          ( data_to_ioreg[79:64]           ),
.core_oeb               ( oeb_to_ioreg[39:32]            ),
.core_data_in           ( data_from_ioreg[79:64]         ),
.avl_select             ( avl_select_ioereg[9:8]         ),
.dpa_track_out_up       ( switch_up[4]                   ),
.dpa_track_out_dn       ( switch_dn[4]                   ),
.phase_kicker           ( dzoutx[4]                      ),
.phase_direction        ( up_ph[4]                       ),
.interpolator_clk_p     ( {next_clk[4],crnt_clk[4]}      ),
.interpolator_clk_n     ( {n_next_clk[4],n_crnt_clk[4]}  ),
.codin_p                ( codin_p[9:8]                   ),
.codin_pb               ( codin_pb[9:8]                  ),
.codin_n                ( codin_n[9:8]                   ),
.codin_nb               ( codin_nb[9:8]                  ),
.dq_diff_in             ( dq_diff_in[19:16]               ),
.dq_sstl_in             ( dq_sstl_in[19:16]               ),
.dqs_clk_a              ( dqs_clk_a[19:16]                ),
.dqs_clk_b              ( dqs_clk_b[19:16]                ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_top       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_top       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_top            ),
.rb_track_speed         ( rb_track_speed_top[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_top[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_top[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_top[1:0]      ),
.rb_filter_code         ( rb_filter_code_top[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_top[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_top           ),
.fifo_reset_n           ( fifo_reset_n_top               ),
.fifo_rank_sel          ( fifo_rank_sel_top              ),
.rank_in_a              ( rank_in_a_top[1:0]             ),
.rank_in_b              ( rank_in_b_top[1:0]             ),
.rank_out               ( rank_out_top[7:0]              ),
.oct_enable_out         ( oct_enable_out_top             ),
.oct_enable_pin         ( oct_enable[9:8]                ),
.avl_wr_address         ( avl_wr_address_top[3:0]        ),
.avl_rd_address         ( avl_rd_address_top[3:0]        ),
.avl_sync_write         ( avl_sync_write_top             ),
.avl_destruct_read      ( avl_destruct_read_top          ),
.avl_writedata          ( avl_writedata_top[15:0]        ),
.avl_readchain_in       ( avl_readchain_top[15:0]       ),
.avl_readchain_out      ( avl_readchain_top[31:16]       ),
.pvt_write              ( pvt_write[4]                   ),
.pvt_address            ( pvt_address_top[2:0]           ),
.pvt_data               ( pvt_data_top[8:0]              ),
.csr_clk                ( csr_clk_top                    ),
.csr_in                 ( ioereg_csrout[5]               ),
.csr_en                 ( csr_en_top                     ),
.csr_shift_n            ( 1'b1                           ),
.csr_out                ( ioereg_csrout[6]               ),
.nfrzdrv                ( nfrzdrv_top                    ),
.frzreg                 ( frzreg_top                     ),
.ncein                  ( ncein[9:8]                     ),
.nceout                 ( nceout[9:8]                    ),
.naclr                  ( naclr[9:8]                     ),
.nsclr                  ( nsclr[9:8]                     ),
.fr_in_clk              ( fr_in_clk[9:8]                 ),
.hr_in_clk              ( hr_in_clk[9:8]                 ),
.fr_out_clk             ( fr_out_clk[9:8]                ),
.hr_out_clk             ( hr_out_clk[9:8]                ),
.pll_clk                ( 1'b0                           ),
.atpg_en_n              ( atpg_en_n_top                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_top       ),
.test_clr_n             ( test_clr_n_top                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_top           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_top           ),
.test_tdf_select_n      ( test_tdf_select_n_top          ),
.tpin                   ( tpin[9:8]                      ),
.tpctl                  ( tpctl[9:8]                     ),
.tpdata                 ( tpdata[9:8]                    ),
.niotri                 ( niotri_top                     ),
.progctl                ( progctl[9:8]                   ),
.progout                ( progout[9:8]                   ),
.progoe                 ( progoe[9:8]                    ),
.test_clk               ( test_clk_top                   ),
.scan_shift_n           ( scan_shift_n_top               ),
.scan_in                ( scanin[4]             	 ),
.scan_out               ( scanout[4]             	 ),
.xor_clk                ( xor_clk[4]                     ),
.test_dbg_out           ( test_dbg_out_local[9:8]        ),
.test_dbg_in            ( test_dbg_in[9:8]               ),
.probe_out              ( {dqs_probe[9:8],rd_probe[35:18]} ),
.read_path_probe        ( rd_probe[29:24]                ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_top     ),
.test_int_clk_en_n      ( test_int_clk_en_n_top          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_top       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_top       ),
.test_datovr_en_n       ( test_datovr_en_n_top           ),
.jtag_shftdr            ( jtag_shftdr_top                ),
.jtag_clk               ( jtag_clk_top                   ),
.jtag_highz             ( jtag_highz_top                 ),
.jtag_sdin              ( ioereg_jtag_sdout[3]           ),
.jtag_updtdr            ( jtag_updtdr_top                ),
.jtag_mode              ( jtag_mode_top                  ),
.jtag_sdout             ( ioereg_jtag_sdout[4]           ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_out_dn[5]                  ),
.osc_in_up              ( osc_in_up[4]                   ),
.osc_out_dn             ( osc_out_dn[4]                  ),
.osc_out_up             ( osc_in_up[5]                   ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[4]             ),
.x64_osc_in_n           ( x64_osc_chain_n[4]             ),
.x64_osc_out_p          ( x64_osc_chain_p[5]             ),
.x64_osc_out_n          ( x64_osc_chain_n[5]             ),
.sync_clk_bot_in        ( sync_clk_up_chn[4]             ),
.sync_data_bot_in       ( sync_data_up_chn[4]            ),
.sync_clk_bot_out       ( sync_clk_dn_chn[4]             ),
.sync_data_bot_out      ( sync_data_dn_chn[4]            ),
.sync_clk_top_in        ( sync_clk_dn_chn[5]             ),
.sync_data_top_in       ( sync_data_dn_chn[5]            ),
.sync_clk_top_out       ( sync_clk_up_chn[5]             ),
.sync_data_top_out      ( sync_data_up_chn[5]            ),
.interpolator_clk_out   ( interpolator_clk_4[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_4[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_4[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_4[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_4[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_4[3:0]             ),
.rb_dq_select           ( rb_dq_select_4[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_4[5:0]            ),
.datovr_out             ( datovr_4[1:0]                  ),
.datovr_in              ( datovr_4[1:0]                  ),
.datovrb_out            ( datovrb_4[1:0]                 ),
.datovrb_in             ( datovrb_4[1:0]                 ),
.data_dq_out            ( data_dq_4[1:0]                 ),
.data_dq_in             ( data_dq_4[1:0]                 ),
.data_dqb_out           ( data_dqb_4[1:0]                ),
.data_dqb_in            ( data_dqb_4[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_4[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_4[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_4[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_4[1:0]  	 ),
.ac_hmc                 ( ac_hmc[79:64]                  ),
.select_ac_hmc          ( select_ac_hmc[9:8]             ),
.dq_in_del              ( dq_in_del_n[19:16]             ),
.dq_in_tree             ( dq_in_tree[19:16]              ),
.dqs_loop_back          ( 		                 ),
.xpr_clk                ( xpr_clk_top                    ),
.xpr_write              ( xpr_write_top[1]               ),
.xpr_data               ( xpr_data_top[7:0]              ),
.ioereg_locked          ( ioereg_locked[4]               ),
.weak_pullup_enable     ( weak_pullup_enable[9:8]        )
);

io_ioereg_top__nf5es ioereg_top_5_ (
.reset_n                ( ioereg_reset_n                 ),
.phy_clk_phs            ( 8'd0               ),
.phy_clk                ( 'd0                   ),
.chk_x1_track_out_up    ( chk_x1_track_out_up[11:10]       ),
.chk_x1_track_out_dn    ( chk_x1_track_out_dn[11:10]       ),
.core_data_out          ( data_to_ioreg[95:80]           ),
.core_oeb               ( oeb_to_ioreg[47:40]            ),
.core_data_in           ( data_from_ioreg[95:80]         ),
.avl_select             ( avl_select_ioereg[11:10]         ),
.dpa_track_out_up       ( switch_up[5]                   ),
.dpa_track_out_dn       ( switch_dn[5]                   ),
.phase_kicker           ( dzoutx[5]                      ),
.phase_direction        ( up_ph[5]                       ),
.interpolator_clk_p     ( {next_clk[5],crnt_clk[5]}      ),
.interpolator_clk_n     ( {n_next_clk[5],n_crnt_clk[5]}  ),
.codin_p                ( codin_p[11:10]                   ),
.codin_pb               ( codin_pb[11:10]                  ),
.codin_n                ( codin_n[11:10]                   ),
.codin_nb               ( codin_nb[11:10]                  ),
.dq_diff_in             ( dq_diff_in[23:20]               ),
.dq_sstl_in             ( dq_sstl_in[23:20]               ),
.dqs_clk_a              ( dqs_clk_a[23:20]                ),
.dqs_clk_b              ( dqs_clk_b[23:20]                ),
.chk_x12_track_out_up   ( chk_x12_track_out_up_top       ),
.chk_x12_track_out_dn   ( chk_x12_track_out_dn_top       ),
.rb_phy_clk_mode        ( rb_phy_clk_mode_top            ),
.rb_track_speed         ( rb_track_speed_top[3:0]        ),
.rb_kicker_size         ( rb_kicker_size_top[1:0]        ),
.rb_mode_rate_in        ( rb_mode_rate_in_top[1:0]       ),
.rb_mode_rate_out       ( rb_mode_rate_out_top[1:0]      ),
.rb_filter_code         ( rb_filter_code_top[1:0]        ),
.fifo_pack_select       ( fifo_pack_select_top[1:0]      ),
.fifo_read_enable       ( fifo_read_enable_top           ),
.fifo_reset_n           ( fifo_reset_n_top               ),
.fifo_rank_sel          ( fifo_rank_sel_top              ),
.rank_in_a              ( rank_in_a_top[1:0]             ),
.rank_in_b              ( rank_in_b_top[1:0]             ),
.rank_out               ( rank_out_top[7:0]              ),
.oct_enable_out         ( oct_enable_out_top             ),
.oct_enable_pin         ( oct_enable[11:10]              ),
.avl_wr_address         ( avl_wr_address_top[3:0]        ),
.avl_rd_address         ( avl_rd_address_top[3:0]        ),
.avl_sync_write         ( avl_sync_write_top             ),
.avl_destruct_read      ( avl_destruct_read_top          ),
.avl_writedata          ( avl_writedata_top[15:0]        ),
.avl_readchain_in       ( 16'h0000                       ),
.avl_readchain_out      ( avl_readchain_top[15:0]        ),
.pvt_write              ( pvt_write[5]                   ),
.pvt_address            ( pvt_address_top[2:0]           ),
.pvt_data               ( pvt_data_top[8:0]              ),
.csr_clk                ( csr_clk_top                    ),
.csr_in                 ( ioereg_csrout[6]               ),
.csr_en                 ( csr_en_top                     ),
.csr_shift_n            ( 1'b1                           ),
.csr_out                ( ioereg_csrout[7]               ),
.nfrzdrv                ( nfrzdrv_top                    ),
.frzreg                 ( frzreg_top                     ),
.ncein                  ( ncein[11:10]                   ),
.nceout                 ( nceout[11:10]                  ),
.naclr                  ( naclr[11:10]                   ),
.nsclr                  ( nsclr[11:10]                   ),
.fr_in_clk              ( fr_in_clk[11:10]               ),
.hr_in_clk              ( hr_in_clk[11:10]               ),
.fr_out_clk             ( fr_out_clk[11:10]              ),
.hr_out_clk             ( hr_out_clk[11:10]              ),
.pll_clk                ( 1'b0                           ),
.atpg_en_n              ( atpg_en_n_top                  ),
.pipeline_global_en_n   ( pipeline_global_en_n_top       ),
.test_clr_n             ( test_clr_n_top                 ),
.test_fr_clk_en_n       ( test_fr_clk_en_n_top           ),
.test_hr_clk_en_n       ( test_hr_clk_en_n_top           ),
.test_tdf_select_n      ( test_tdf_select_n_top          ),
.tpin                   ( tpin[11:10]                    ),
.tpctl                  ( tpctl[11:10]                   ),
.tpdata                 ( tpdata[11:10]                  ),
.niotri                 ( niotri_top                     ),
.progctl                ( progctl[11:10]                 ),
.progout                ( progout[11:10]                 ),
.progoe                 ( progoe[11:10]                  ),
.test_clk               ( test_clk_top                   ),
.scan_shift_n           ( scan_shift_n_top               ),
.scan_in                ( scanin[5]             	 ),
.scan_out               ( scanout[5]            	 ),
.xor_clk                ( xor_clk[5]                     ),
.test_dbg_out           ( test_dbg_out_local[11:10]      ),
.test_dbg_in            ( test_dbg_in[11:10]             ),
.probe_out              ( {dqs_probe[11:10],rd_probe[35:18]}),
.read_path_probe        ( rd_probe[35:30]                ),
.test_phy_clk_lane_en_n ( test_phy_clk_lane_en_n_top     ),
.test_int_clk_en_n      ( test_int_clk_en_n_top          ),
.test_interp_clk_en_n   ( test_interp_clk_en_n_top       ),
.test_clk_ph_buf_en_n   ( test_clk_ph_buf_en_n_top       ),
.test_datovr_en_n       ( test_datovr_en_n_top           ),
.jtag_shftdr            ( jtag_shftdr_top                ),
.jtag_clk               ( jtag_clk_top                   ),
.jtag_highz             ( jtag_highz_top                 ),
.jtag_sdin              ( ioereg_jtag_sdout[4]           ),
.jtag_updtdr            ( jtag_updtdr_top                ),
.jtag_mode              ( jtag_mode_top                  ),
.jtag_sdout             ( jtag_sdout           		 ),
.osc_sel_n              ( osc_sel_n                      ),
.osc_in_dn              ( osc_start_in                   ),
.osc_in_up              ( osc_in_up[5]                   ),
.osc_out_dn             ( osc_out_dn[5]                  ),
.osc_out_up             ( osc_start_out                  ),
.x64_osc_mode           ( x64_osc_mode_out               ),
.x64_osc_in_p           ( x64_osc_chain_p[5]             ),
.x64_osc_in_n           ( x64_osc_chain_n[5]             ),
.x64_osc_out_p          ( x64_osc_chain_p[6]             ),
.x64_osc_out_n          ( x64_osc_chain_n[6]             ),
.sync_clk_bot_in        ( sync_clk_up_chn[5]             ),
.sync_data_bot_in       ( sync_data_up_chn[5]            ),
.sync_clk_bot_out       ( sync_clk_dn_chn[5]             ),
.sync_data_bot_out      ( sync_data_dn_chn[5]            ),
.sync_clk_top_in        ( sync_clk_top_in                ),
.sync_data_top_in       ( sync_data_top_in               ),
.sync_clk_top_out       ( sync_clk_top_out               ),
.sync_data_top_out      ( sync_data_top_out              ),
.interpolator_clk_out   ( interpolator_clk_5[3:0]        ),
.interpolator_clk_in    ( interpolator_clk_5[3:0]        ),
.d_out_p_mux_out        ( d_out_p_mux_5[3:0]             ),
.d_out_p_mux_in         ( d_out_p_mux_5[3:0]             ),
.d_out_n_mux_out        ( d_out_n_mux_5[3:0]             ),
.d_out_n_mux_in         ( d_out_n_mux_5[3:0]             ),
.rb_dq_select           ( rb_dq_select_5[5:0]            ),
.rb_dq_select_in        ( rb_dq_select_5[5:0]            ),
.datovr_out             ( datovr_5[1:0]                  ),
.datovr_in              ( datovr_5[1:0]                  ),
.datovrb_out            ( datovrb_5[1:0]                 ),
.datovrb_in             ( datovrb_5[1:0]                 ),
.data_dq_out            ( data_dq_5[1:0]                 ),
.data_dq_in             ( data_dq_5[1:0]                 ),
.data_dqb_out           ( data_dqb_5[1:0]                ),
.data_dqb_in            ( data_dqb_5[1:0]                ),
.struct_latch_open_n_out( struct_latch_open_n_5[1:0]     ),
.nfrzdrv_struct_out     ( nfrzdrv_struct_5[1:0]  	 ),
.struct_latch_open_n_in ( struct_latch_open_n_5[1:0]     ),
.nfrzdrv_struct_in      ( nfrzdrv_struct_5[1:0]  	 ),
.ac_hmc                 ( ac_hmc[95:80]                  ),
.select_ac_hmc          ( select_ac_hmc[11:10]           ),
.dq_in_del              ( dq_in_del_n[23:20]             ),
.dq_in_tree             ( dq_in_tree[23:20]              ),
.dqs_loop_back          ( 		                 ),
.xpr_clk                ( xpr_clk_top                    ),
.xpr_write              ( xpr_write_top[2]               ),
.xpr_data               ( xpr_data_top[7:0]              ),
.ioereg_locked          ( ioereg_locked[5]               ),
.weak_pullup_enable     ( weak_pullup_enable[11:10]      )
);

io_dqs_lgc_top__nf5es xio_dqs_lgc_top (
  .reset_n                    ( ioereg_reset_n             ),
  .phy_clk_phs                ( 8'd0           ),
  .phy_clk                    ( afi_cal_success ? 'd0 : phy_clk[1:0]               ),
  .broadcast_in_top           ( broadcast_in_top           ),
  .broadcast_in_bot           ( broadcast_in_bot           ),
  .broadcast_out_top          ( broadcast_out_top          ),
  .broadcast_out_bot          ( broadcast_out_bot          ),
  .avl_clk_in                 ( avl_clk_in                 ),
  .avl_clk_out                ( avl_clk_out                ),
  .avl_address_in             ( avl_address_in[19:0]       ),
  .avl_address_out            ( avl_address_out[19:0]      ),
  .avl_address_dbc            ( avl_address_dbc[19:0]      ),
  .avl_wr_address_top         ( avl_wr_address_top[3:0]    ),
  .avl_rd_address_top         ( avl_rd_address_top[3:0]    ),
  .avl_wr_address_bot         ( avl_wr_address_bot[3:0]    ),
  .avl_rd_address_bot         ( avl_rd_address_bot[3:0]    ),
  .avl_writedata_in           ( avl_writedata_in[31:0]     ),
  .avl_writedata_out          ( avl_writedata_out[31:0]    ),
  .avl_writedata_dbc          ( avl_writedata_dbc[31:0]    ),
  .avl_writedata_top          ( avl_writedata_top[15:0]    ),
  .avl_writedata_bot          ( avl_writedata_bot[15:0]    ),
  .avl_readdata_in            ( avl_readdata_in[31:0]      ),
  .avl_readdata_out           ( avl_readdata_out[31:0]     ),
  .avl_readdata_dbc           ( avl_readdata_dbc[31:0]     ),
  .avl_readchain_top          ( avl_readchain_top[47:32]   ),
  .avl_readchain_bot          ( avl_readchain_bot[47:32]   ),
  .avl_write_in               ( avl_write_in               ),
  .avl_write_out              ( avl_write_out              ),
  .avl_write_dbc              ( avl_write_dbc[1:0]         ),
  .avl_sync_write_top         ( avl_sync_write_top         ),
  .avl_sync_write_bot         ( avl_sync_write_bot         ),
  .avl_read_in                ( avl_read_in                ),
  .avl_read_out               ( avl_read_out               ),
  .avl_destruct_read_top      ( avl_destruct_read_top      ),
  .avl_destruct_read_bot      ( avl_destruct_read_bot      ),
  .avl_select                 ( avl_select_ioereg[12]      ),
  .afi_rddata_en_full         ( rdata_en_full_local[3:0]   ),
  .afi_mrnk_read              ( mrnk_read_local[7:0]       ),
  .afi_mrnk_write             ( mrnk_write_local[7:0]      ),
  .afi_rddata_valid           (      ),
  .chk_x1_track_in_up         ( chk_x1_track_in_up[11:0]   ),
  .chk_x1_track_in_dn         ( chk_x1_track_in_dn[11:0]   ),
  .chk_x12_track_in_up        ( chk_x12_track_in_up        ),
  .chk_x12_track_in_dn        ( chk_x12_track_in_dn        ),
  .chk_x12_track_out_up       ( chk_x12_track_out_up       ),
  .chk_x12_track_out_dn       ( chk_x12_track_out_dn       ),
  .chk_x12_track_out_up_top   ( chk_x12_track_out_up_top   ),
  .chk_x12_track_out_up_bot   ( chk_x12_track_out_up_bot   ),
  .chk_x12_track_out_dn_top   ( chk_x12_track_out_dn_top   ),
  .chk_x12_track_out_dn_bot   ( chk_x12_track_out_dn_bot   ),
  .fifo_pack_select_top       ( fifo_pack_select_top[1:0]  ),
  .fifo_read_enable_top       ( fifo_read_enable_top       ),
  .fifo_reset_n_top           ( fifo_reset_n_top           ),
  .fifo_rank_sel_top          ( fifo_rank_sel_top          ),
  .fifo_pack_select_bot       ( fifo_pack_select_bot[1:0]  ),
  .fifo_read_enable_bot       ( fifo_read_enable_bot       ),
  .fifo_reset_n_bot           ( fifo_reset_n_bot           ),
  .fifo_rank_sel_bot          ( fifo_rank_sel_bot          ),
  .dqs_diff_in_0              ( dqs_diff_in_0[1:0]         ),
  .dqs_diff_in_1              ( dqs_diff_in_1[1:0]         ),
  .dqs_diff_in_2              ( dqs_diff_in_2[1:0]         ),
  .dqs_diff_in_3              ( dqs_diff_in_3[1:0]         ),
  .dqs_sstl_p_0               ( dqs_sstl_p_0[1:0]          ),
  .dqs_sstl_p_1               ( dqs_sstl_p_1[1:0]          ),
  .dqs_sstl_p_2               ( dqs_sstl_p_2[1:0]          ),
  .dqs_sstl_p_3               ( dqs_sstl_p_3[1:0]          ),
  .dqs_sstl_n_0               ( dqs_sstl_n_0[1:0]          ),
  .dqs_sstl_n_1               ( dqs_sstl_n_1[1:0]          ),
  .dqs_sstl_n_2               ( dqs_sstl_n_2[1:0]          ),
  .dqs_sstl_n_3               ( dqs_sstl_n_3[1:0]          ),
  .pvt_ref_gry                ( pvt_ref_gry[9:0]           ),
  .pvt_write                  ( pvt_write[5:0]             ),
  .pvt_address_top            ( pvt_address_top[2:0]       ),
  .pvt_address_bot            ( pvt_address_bot[2:0]       ),
  .pvt_data_top               ( pvt_data_top[8:0]          ),
  .pvt_data_bot               ( pvt_data_bot[8:0]          ),
  .dqs_out_a                  ( dqs_out_a_n[1:0]           ),
  .dqs_out_b                  ( dqs_out_b_n[1:0]           ),
  .rank_in_a_top              ( rank_in_a_top[1:0]         ),
  .rank_in_a_bot              ( rank_in_a_bot[1:0]         ),
  .rank_in_b_top              ( rank_in_b_top[1:0]         ),
  .rank_in_b_bot              ( rank_in_b_bot[1:0]         ),
  .rank_out_top               ( rank_out_top[7:0]          ),
  .rank_out_bot               ( rank_out_bot[7:0]          ),
  .oct_enable_out_top         ( oct_enable_out_top         ),
  .oct_enable_out_bot         ( oct_enable_out_bot         ),
  .rb_phy_clk_mode_bot        ( rb_phy_clk_mode_bot        ),
  .rb_track_speed_bot         ( rb_track_speed_bot[3:0]    ),
  .rb_kicker_size_bot         ( rb_kicker_size_bot[1:0]    ),
  .rb_mode_rate_in_bot        ( rb_mode_rate_in_bot[1:0]   ),
  .rb_mode_rate_out_bot       ( rb_mode_rate_out_bot[1:0]  ),
  .rb_filter_code_bot         ( rb_filter_code_bot[1:0]    ),
  .rb_phy_clk_mode_top        ( rb_phy_clk_mode_top        ),
  .rb_track_speed_top         ( rb_track_speed_top[3:0]    ),
  .rb_kicker_size_top         ( rb_kicker_size_top[1:0]    ),
  .rb_mode_rate_in_top        ( rb_mode_rate_in_top[1:0]   ),
  .rb_mode_rate_out_top       ( rb_mode_rate_out_top[1:0]  ),
  .rb_filter_code_top         ( rb_filter_code_top[1:0]    ),
  .rb_phy_clk_mode_right      ( rb_phy_clk_mode_right      ),
  .sel_vref                   ( sel_vref[5:0]              ),
  .csr_clk                    ( csr_clk                    ),
  .csr_clk_top                ( csr_clk_top                ),
  .csr_clk_bot                ( csr_clk_bot                ),
  .csr_clk_left               ( csr_clk_left               ),
  .csr_clk_dbc                ( csr_clk_dbc                ),
  .csr_in                     ( dqs_csrin                  ),
  .csr_out                    ( ioereg_csrout[4]           ),
  .csr_en                     ( csr_en                     ),
  .csr_en_top                 ( csr_en_top                 ),
  .csr_en_bot                 ( csr_en_bot                 ),
  .csr_en_left                ( csr_en_left                ),
  .csr_en_dbc                 ( csr_en_dbc                 ),
  .csr_shift_n                ( csr_shift_n                ),
  .csr_shift_n_top            ( csr_shift_n_top            ),
  .csr_shift_n_bot            ( csr_shift_n_bot            ),
  .csr_shift_n_left           ( csr_shift_n_left           ),
  .csr_shift_n_dbc            ( csr_shift_n_dbc            ),
  .nfrzdrv                    ( local_nfrzdrv              ),
  .nfrzdrv_top                ( nfrzdrv_top                ),
  .nfrzdrv_bot                ( nfrzdrv_bot                ),
  .frzreg                     ( local_frzreg               ),
  .frzreg_top                 ( frzreg_top                 ),
  .frzreg_bot                 ( frzreg_bot                 ),
  .dll_lock                   ( dll_core[10]               ),
  .test_fr_clk_en_n           ( test_fr_clk_en_n           ),
  .test_fr_clk_en_n_top       ( test_fr_clk_en_n_top       ),
  .test_fr_clk_en_n_bot       ( test_fr_clk_en_n_bot       ),
  .test_hr_clk_en_n           ( test_hr_clk_en_n           ),
  .test_hr_clk_en_n_top       ( test_hr_clk_en_n_top       ),
  .test_hr_clk_en_n_bot       ( test_hr_clk_en_n_bot       ),
  .test_tdf_select_n          ( test_tdf_select_n          ),
  .test_tdf_select_n_top      ( test_tdf_select_n_top      ),
  .test_tdf_select_n_bot      ( test_tdf_select_n_bot      ),
  .atpg_en_n                  ( atpg_en_n                  ),
  .atpg_en_n_top              ( atpg_en_n_top              ),
  .atpg_en_n_bot              ( atpg_en_n_bot              ),
  .atpg_en_n_left             ( atpg_en_n_left             ),
  .pipeline_global_en_n       ( pipeline_global_en_n       ),
  .pipeline_global_en_n_top   ( pipeline_global_en_n_top   ),
  .pipeline_global_en_n_bot   ( pipeline_global_en_n_bot   ),
  .pipeline_global_en_n_left  ( pipeline_global_en_n_left  ),
  .test_clr_n                 ( test_clr_n                 ),
  .test_clr_n_top             ( test_clr_n_top             ),
  .test_clr_n_bot             ( test_clr_n_bot             ),
  .niotri                     ( local_niotri               ),
  .niotri_top                 ( niotri_top                 ),
  .niotri_bot                 ( niotri_bot                 ),
  .test_clk                   ( test_clk                   ),
  .test_clk_top               ( test_clk_top               ),
  .test_clk_bot               ( test_clk_bot               ),
  .test_clk_left              ( test_clk_left              ),
  .scan_shift_n               ( scan_shift_n               ),
  .scan_shift_n_top           ( scan_shift_n_top           ),
  .scan_shift_n_bot           ( scan_shift_n_bot           ),
  .scan_shift_n_dbc           ( scan_shift_n_dbc           ),
  .scan_shift_n_left          ( scan_shift_n_left          ),
  .jtag_shftdr                ( jtag_shftdr                ),
  .jtag_shftdr_top            ( jtag_shftdr_top            ),
  .jtag_shftdr_bot            ( jtag_shftdr_bot            ),
  .jtag_clk                   ( jtag_clk                   ),
  .jtag_clk_top               ( jtag_clk_top               ),
  .jtag_clk_bot               ( jtag_clk_bot               ),
  .jtag_highz                 ( jtag_highz                 ),
  .jtag_highz_top             ( jtag_highz_top             ),
  .jtag_highz_bot             ( jtag_highz_bot             ),
  .jtag_updtdr                ( jtag_updtdr                ),
  .jtag_updtdr_top            ( jtag_updtdr_top            ),
  .jtag_updtdr_bot            ( jtag_updtdr_bot            ),
  .jtag_mode                  ( jtag_mode                  ),
  .jtag_mode_top              ( jtag_mode_top              ),
  .jtag_mode_bot              ( jtag_mode_bot              ),
  .xor_clk                    ( xor_clk[5:0]               ),
  .test_xor_clk               ( test_xor_clk               ),
  .test_phy_clk_lane_en_n     ( test_phy_clk_lane_en_n     ),
  .test_phy_clk_lane_en_n_bot ( test_phy_clk_lane_en_n_bot ),
  .test_phy_clk_lane_en_n_top ( test_phy_clk_lane_en_n_top ),
  .test_int_clk_en_n          ( test_int_clk_en_n          ),
  .test_int_clk_en_n_bot      ( test_int_clk_en_n_bot      ),
  .test_int_clk_en_n_top      ( test_int_clk_en_n_top      ),
  .test_interp_clk_en_n       ( test_interp_clk_en_n       ),
  .test_interp_clk_en_n_bot   ( test_interp_clk_en_n_bot   ),
  .test_interp_clk_en_n_top   ( test_interp_clk_en_n_top   ),
  .test_clk_ph_buf_en_n       ( test_clk_ph_buf_en_n       ),
  .test_clk_ph_buf_en_n_bot   ( test_clk_ph_buf_en_n_bot   ),
  .test_clk_ph_buf_en_n_top   ( test_clk_ph_buf_en_n_top   ),
  .test_datovr_en_n           ( test_datovr_en_n           ),
  .test_datovr_en_n_bot       ( test_datovr_en_n_bot       ),
  .test_datovr_en_n_top       ( test_datovr_en_n_top       ),
  .test_avl_clk_in_en_n       ( test_avl_clk_in_en_n       ),
  .test_dqs_enable_en_n       ( test_dqs_enable_en_n       ),
  .probe_out                  ( dqs_probe[11:0]            ),
  .sync_data_bot_in           ( sync_data_up_chn[2]        ),
  .sync_clk_bot_out           ( sync_clk_dn_chn[2]         ),
  .sync_data_bot_out          ( sync_data_dn_chn[2]        ),
  .sync_data_top_in           ( sync_data_dn_chn[3]        ),
  .sync_clk_top_out           ( sync_clk_up_chn[3]         ),
  .sync_data_top_out          ( sync_data_up_chn[3]        ),
  .pst_test_i_n               ( test_pst_dll_i             ),
  .pst_test_o_n               ( test_i_n_dll               ),
  .pst_test_i_p               ( test_i_p_pst               ),
  .pst_test_o_p               ( test_pst_dll_o             ),
  .test_dqs_i1                ( core_dll[0]                ),
  .test_dqs_i2                ( core_dll[1]                ),
  .test_dqs_o1                ( test_dqs_o1                ),
  .test_dqs_o2                ( test_dqs_o2                ),
  .test_pst_clk_en_n_a        ( test_pst_clk_en_n          ),
  .test_pst_clk_en_n_b        ( test_pst_clk_en_n          ),
  .xprio_clk                  ( xprio_clk                  ),
  .xprio_sync                 ( xprio_sync                 ),
  .xprio_xbus                 ( xprio_xbus[7:0]            ),
  .xpr_clk_bot                ( xpr_clk_bot                ),
  .xpr_clk_top                ( xpr_clk_top                ),
  .xpr_write_bot              ( xpr_write_bot[2:0]         ),
  .xpr_write_top              ( xpr_write_top[2:0]         ),
  .xpr_data_bot               ( xpr_data_bot[7:0]          ),
  .xpr_data_top               ( xpr_data_top[7:0]          ),
  .dqs_loop_back_0a           ( dqs_loop_back_0[1:0]       ),
  .dqs_loop_back_1a           ( dqs_loop_back_1[1:0]       ),
  .dqs_loop_back_0b           ( dqs_loop_back_0[1:0]       ),
  .dqs_loop_back_1b           ( dqs_loop_back_1[1:0]       ),
  .x128_osc_in_p              ( x64_osc_chain_p[7]        ),
  .x128_osc_in_n              ( x64_osc_chain_n[7]        ),
  .x128_osc_out_p             ( x64_osc_chain_p[0]         ),
  .x128_osc_out_n             ( x64_osc_chain_n[0]         ),
  .osc_enable_in              ( osc_enable_in              ),
  .osc_mode_in                ( osc_mode_in                ),
  .osc_mode_out               ( x64_osc_mode_out           ),
  .x1024_osc_out              ( x1024_osc_out              )
);


wire  [5:0]  dummy_a, dummy_b;

io_data_buffer__nf5es data_buffer (
  .phy_clk                   ( phy_clk[1:0]                   ),
  .frzreg                    ( local_frzreg                   ),
  .pll_locked                ( pll_locked                     ),
  .global_reset_n            ( reset_n                        ),
  .scan_shift_n              ( scan_shift_n_dbc               ),
  .csr_scan_shift_n          ( 1'b1                           ),
  .atpg_en_n                 ( atpg_en_n                      ),
  .dft_pipeline_global_en_n  ( pipeline_global_en_n           ),
  .test_clk                  ( test_clk                       ),
  .test_phy_clk_en_n         ( test_phy_clk_en_n              ),
  .dft_prbs_ena_n            ( dft_prbs_ena_n                 ),
  .dft_prbs_pass             ( dft_prbs_pass                  ),
  .dft_prbs_done             ( dft_prbs_done                  ),
  .dft_core2db               ( dft_core2db[7:0]               ),
  .dft_db2core               ( dft_db2core[7:0]               ),
  .dft_phy_clk               ( dft_phy_clk[1:0]               ),
  .avl_address               ( avl_address_dbc[19:0]          ),
  .avl_write                 ( avl_write_dbc[1:0]             ),
  .avl_writedata             ( avl_writedata_dbc[31:0]        ),
  .avl_readdata              ( avl_readdata_dbc[31:0]         ),
  .avl_select                ( avl_select_ioereg[12:0]        ),
  .ctl2dbc_wrdata_vld0       ( ctl2dbc_wrdata_vld0            ),
  .ctl2dbc_mask_entry0       ( ctl2dbc_mask_entry0            ),
  .ctl2dbc_wb_rdptr_vld0     ( ctl2dbc_wb_rdptr_vld0          ),
  .ctl2dbc_wb_rdptr0         ( ctl2dbc_wb_rdptr0[5:0]         ),
  .ctl2dbc_rb_wrptr_vld0     ( ctl2dbc_rb_wrptr_vld0          ),
  .ctl2dbc_rb_wrptr0         ( ctl2dbc_rb_wrptr0[5:0]         ),
  .ctl2dbc_rb_rdptr_vld0     ( ctl2dbc_rb_rdptr_vld0[1:0]     ),
  .ctl2dbc_rb_rdptr0         ( ctl2dbc_rb_rdptr0[11:0]        ),
  .ctl2dbc_rdata_en_full0    ( ctl2dbc_rdata_en_full0[3:0]    ),
  .ctl2dbc_mrnk_read0        ( ctl2dbc_mrnk_read0[7:0]        ),
  .ctl2dbc_seq_en0           ( ctl2dbc_seq_en0                ),
  .ctl2dbc_nop0              ( ctl2dbc_nop0                   ),
  .ctl2dbc_cs0               ( ctl2dbc_cs0[1:0]               ),
  .ctl2dbc_rd_type0          ( ctl2dbc_rd_type0               ),
  .ctl2dbc_misc0             ( ctl2dbc_misc0[3:0]             ),
  .ctl2dbc_wrdata_vld1       ( ctl2dbc_wrdata_vld1            ),
  .ctl2dbc_mask_entry1       ( ctl2dbc_mask_entry1            ),
  .ctl2dbc_wb_rdptr_vld1     ( ctl2dbc_wb_rdptr_vld1          ),
  .ctl2dbc_wb_rdptr1         ( ctl2dbc_wb_rdptr1[5:0]         ),
  .ctl2dbc_rb_wrptr_vld1     ( ctl2dbc_rb_wrptr_vld1          ),
  .ctl2dbc_rb_wrptr1         ( ctl2dbc_rb_wrptr1[5:0]         ),
  .ctl2dbc_rb_rdptr_vld1     ( ctl2dbc_rb_rdptr_vld1[1:0]     ),
  .ctl2dbc_rb_rdptr1         ( ctl2dbc_rb_rdptr1[11:0]        ),
  .ctl2dbc_rdata_en_full1    ( ctl2dbc_rdata_en_full1[3:0]    ),
  .ctl2dbc_mrnk_read1        ( ctl2dbc_mrnk_read1[7:0]        ),
  .ctl2dbc_seq_en1           ( ctl2dbc_seq_en1                ),
  .ctl2dbc_nop1              ( ctl2dbc_nop1                   ),
  .ctl2dbc_cs1               ( ctl2dbc_cs1[1:0]               ),
  .ctl2dbc_rd_type1          ( ctl2dbc_rd_type1               ),
  .ctl2dbc_misc1             ( ctl2dbc_misc1[3:0]             ),
  .dbc2ctl_wb_retire_ptr_vld ( dbc2ctl_wb_retire_ptr_vld      ),
  .dbc2ctl_wb_retire_ptr     ( dbc2ctl_wb_retire_ptr[5:0]     ),
  .dbc2ctl_rb_retire_ptr_vld ( dbc2ctl_rb_retire_ptr_vld      ),
  .dbc2ctl_rb_retire_ptr     ( dbc2ctl_rb_retire_ptr[5:0]     ),
  .dbc2ctl_rd_data_vld       ( dbc2ctl_rd_data_vld            ),
  .dbc2ctl_all_rd_done       ( dbc2ctl_all_rd_done            ),
  .dbc2db_wb_wrptr_vld       ( dbc2db_wb_wrptr_vld            ),
  .dbc2db_wb_wrptr           ( dbc2db_wb_wrptr[5:0]           ),
  .cfg_dbc_ctrl_sel          ( cfg_dbc_ctrl_sel               ),
  .cfg_reorder_rdata         ( cfg_reorder_rdata              ),
  .cfg_rmw_en                ( cfg_rmw_en                     ),
  .cfg_output_regd           ( cfg_output_regd                ),
  .cfg_dbc_in_protocol       ( cfg_dbc_in_protocol            ),
  .cfg_dbc_dualport_en       ( cfg_dbc_dualport_en            ),
  .cfg_dbc_pipe_lat          ( cfg_dbc_pipe_lat[2:0]          ),
  .cfg_cmd_rate              ( cfg_cmd_rate[2:0]              ),
  .cfg_dbc_rc_en             ( cfg_dbc_rc_en                  ),
  .cfg_dbc_slot_rotate_en    ( cfg_dbc_slot_rotate_en[2:0]    ),
  .cfg_dbc_slot_offset       ( cfg_dbc_slot_offset[1:0]       ),
  .rb_phy_clk_mode           ( rb_phy_clk_mode_right          ),
  .oe_to_ioreg               ( oeb_to_ioreg[47:0]             ),
  .data_to_ioreg             ( data_to_ioreg[95:0]            ),
  .data_from_ioreg           ( data_from_ioreg_abphy[95:0]    ),
  .read_data_valid           ( rdata_valid_local[3:0]         ),
  .dq_in_dly_up              ( chk_x1_track_in_up[11:0]       ),
  .dq_in_dly_dn              ( chk_x1_track_in_dn[11:0]       ),
  .dq_out_dly_up             ( chk_x1_track_out_up[11:0]      ),
  .dq_out_dly_dn             ( chk_x1_track_out_dn[11:0]      ),
  .dqs_in_dly_up             ( chk_x12_track_in_up            ),
  .dqs_in_dly_dn             ( chk_x12_track_in_dn            ),
  .dqs_out_dly_up            ( chk_x12_track_out_up           ),
  .dqs_out_dly_dn            ( chk_x12_track_out_dn           ),
  .rdata_en_full             ( rdata_en_full_local[3:0]       ),
  .mrnk_read                 ( mrnk_read_local[7:0]           ),
  .mrnk_write                ( mrnk_write_local[7:0]          ),
  .select_ac_hmc             ( select_ac_hmc[11:0]            ),
  .ioereg_reset_n            ( ioereg_reset_n                 ),
  .test_dbg_in               ( test_dbg_in[11:0]              ),
  .test_dbg_out_db_in        ( test_dbg_out[11:0]             ),
  .test_dbg_out_db_out       ( test_dbg_out_local[11:0]       ),
  .oe_from_core              ( oeb_from_core[47:0]            ),
  .data_from_core            ( data_from_core[95:0]           ),
  .data_to_core              ( data_to_core[95:0]             ),
  .rdata_en_full_core        ( rdata_en_full_core[3:0]        ),
  .mrnk_read_core            ( mrnk_read_core[15:0]           ),
  .mrnk_write_core           ( mrnk_write_core[15:0]          ),
  .rdata_valid_core          ( rdata_valid_core[3:0]          ),
  .afi_wlat_core             ( afi_wlat_core[5:0]             ),
  .afi_rlat_core             ( afi_rlat_core[5:0]             ),
  .core2dbc_wr_data_vld0     ( core2dbc_wr_data_vld0          ),
  .core2dbc_wr_data_vld1     ( core2dbc_wr_data_vld1          ),
  .dbc2core_wr_data_rdy      ( dbc2core_wr_data_rdy           ),
  .dbc2core_rd_data_vld0     ( dbc2core_rd_data_vld0          ),
  .dbc2core_rd_data_vld1     ( dbc2core_rd_data_vld1          ),
  .dbc2core_rd_type          ( dbc2core_rd_type               ),
  .core2dbc_rd_data_rdy      ( core2dbc_rd_data_rdy           ),
  .core2dbc_wr_ecc_info      ( core2dbc_wr_ecc_info[12:0]     ),
  .dbc2core_wb_pointer       ( dbc2core_wb_pointer[11:0]      ),
  .csr_clk                   ( csr_clk_dbc                    ),
  .csr_ena                   ( csr_en_dbc                     ),
  .csr_din                   ( db_csrin                         ),
  .csr_dout                  ( ioereg_csrout[0]               )
);


io_phy_ckt__nf5es phy_ckt (
.lvds_rxloaden_in      ( {7{phy_clk[0]}}           ),
.lvds_rxclk_in         ( {7{phy_clk[1]}}           ),
.fb_clkin              ( {21{phy_clk[2]}}          ),
.lvds_txloaden_in      ( {6{phy_clk[3]}}           ),
.lvds_txclk_in         ( {6{phy_clk[4]}}           ),
.lvds_rx_clk_chnl0     ( lvds_rx_clk_chnl0[1:0]    ),
.lvds_tx_clk_chnl0     ( lvds_tx_clk_chnl0[1:0]    ),
.lvds_rx_clk_chnl1     ( lvds_rx_clk_chnl1[1:0]    ),
.lvds_tx_clk_chnl1     ( lvds_tx_clk_chnl1[1:0]    ),
.lvds_rx_clk_chnl2     ( lvds_rx_clk_chnl2[1:0]    ),
.lvds_tx_clk_chnl2     ( lvds_tx_clk_chnl2[1:0]    ),
.lvds_rx_clk_chnl3     ( lvds_rx_clk_chnl3[1:0]    ),
.lvds_tx_clk_chnl3     ( lvds_tx_clk_chnl3[1:0]    ),
.lvds_rx_clk_chnl4     ( lvds_rx_clk_chnl4[1:0]    ),
.lvds_tx_clk_chnl4     ( lvds_tx_clk_chnl4[1:0]    ),
.lvds_rx_clk_chnl5     ( lvds_rx_clk_chnl5[1:0]    ),
.lvds_tx_clk_chnl5     ( lvds_tx_clk_chnl5[1:0]    ),
.phy_clk0fb            ( fb_clkout[0]              ),
.phy_clk1fb            ( fb_clkout[1]              ),
.fb_clkout             ( fb_clkout[2]              )
);


io_dqs_ckt__nf5es dqs_ckt (
.dqsin_a              ( dqs_out_a_n[1:0]   ),
.dqsin_b              ( dqs_out_b_n[1:0]   ),
.dq0_in               ( dq_in_del_n[1:0]   ),
.dq1_in               ( dq_in_del_n[3:2]   ),
.dq2_in               ( dq_in_del_n[5:4]   ),
.dq3_in               ( dq_in_del_n[7:6]   ),
.dq4_in               ( dq_in_del_n[9:8]   ),
.dq5_in               ( dq_in_del_n[11:10] ),
.dq6_in               ( dq_in_del_n[13:12] ),
.dq7_in               ( dq_in_del_n[15:14] ),
.dq8_in               ( dq_in_del_n[17:16] ),
.dq9_in               ( dq_in_del_n[19:18] ),
.dq10_in              ( dq_in_del_n[21:20] ),
.dq11_in              ( dq_in_del_n[23:22] ),
.n_dqsout_a_ioreg0    ( dqs_clk_a[1:0]     ),
.n_dqsout_b_ioreg0    ( dqs_clk_b[1:0]     ),
.n_dqsout_a_ioreg1    ( dqs_clk_a[3:2]     ),
.n_dqsout_b_ioreg1    ( dqs_clk_b[3:2]     ),
.n_dqsout_a_ioreg2    ( dqs_clk_a[5:4]     ),
.n_dqsout_b_ioreg2    ( dqs_clk_b[5:4]     ),
.n_dqsout_a_ioreg3    ( dqs_clk_a[7:6]     ),
.n_dqsout_b_ioreg3    ( dqs_clk_b[7:6]     ),
.n_dqsout_a_ioreg4    ( dqs_clk_a[9:8]     ),
.n_dqsout_b_ioreg4    ( dqs_clk_b[9:8]     ),
.n_dqsout_a_ioreg5    ( dqs_clk_a[11:10]   ),
.n_dqsout_b_ioreg5    ( dqs_clk_b[11:10]   ),
.n_dqsout_a_ioreg6    ( dqs_clk_a[13:12]   ),
.n_dqsout_b_ioreg6    ( dqs_clk_b[13:12]   ),
.n_dqsout_a_ioreg7    ( dqs_clk_a[15:14]   ),
.n_dqsout_b_ioreg7    ( dqs_clk_b[15:14]   ),
.n_dqsout_a_ioreg8    ( dqs_clk_a[17:16]   ),
.n_dqsout_b_ioreg8    ( dqs_clk_b[17:16]   ),
.n_dqsout_a_ioreg9    ( dqs_clk_a[19:18]   ),
.n_dqsout_b_ioreg9    ( dqs_clk_b[19:18]   ),
.n_dqsout_a_ioreg10   ( dqs_clk_a[21:20]   ),
.n_dqsout_b_ioreg10   ( dqs_clk_b[21:20]   ),
.n_dqsout_a_ioreg11   ( dqs_clk_a[23:22]   ),
.n_dqsout_b_ioreg11   ( dqs_clk_b[23:22]   ),
.n_dq0out             ( dq_in_tree[1:0]    ),
.n_dq1out             ( dq_in_tree[3:2]    ),
.n_dq2out             ( dq_in_tree[5:4]    ),
.n_dq3out             ( dq_in_tree[7:6]    ),
.n_dq4out             ( dq_in_tree[9:8]    ),
.n_dq5out             ( dq_in_tree[11:10]  ),
.n_dq6out             ( dq_in_tree[13:12]  ),
.n_dq7out             ( dq_in_tree[15:14]  ),
.n_dq8out             ( dq_in_tree[17:16]  ),
.n_dq9out             ( dq_in_tree[19:18]  ),
.n_dq10out            ( dq_in_tree[21:20]  ),
.n_dq11out            ( dq_in_tree[23:22]  )
);


io_dll_top__nf5es xio_dll_top (
.core_dll                 ( 'd0          ),
.csrclk                   ( csr_clk_left           ),
.csren                    ( csr_en_left            ),
.early_csren              ( early_csren            ),
.csrdatain                ( ioereg_csrout[7]       ),
.clk_pll                  ( 'd0                ),
.reinit                   ( 'd0                 ),
.entest                   ( entest                 ),
.csr_scan_shift_n         ( 1'b1                   ),
.cas_csrdin               ( cas_csrdin[4:0]        ),
.cas_csrdout              ( cas_csrdout[4:0]       ),
.scan_shift_n             ( scan_shift_n_left      ),
.atpg_en_n                ( atpg_en_n_left         ),
.dll_core                 ( dll_core[12:0]         ),
.pvt_ref_gry              ( pvt_ref_gry[9:0]       ),
.csrdataout               ( ioereg_csrout[8]       ),
.dft_pipeline_global_en_n ( pipeline_global_en_n_left   ),
.test_clk                 ( test_clk_left          ),
.test_clr_n               ( test_clr_n_bot         ),
.test_clk_pll_en_n        ( test_clk_pll_en_n      ),
.bhniotri                 ( bhniotri               ),
.early_bhniotri           ( early_bhniotri         ),
.enrnsl                   ( enrnsl                 ),
.early_enrnsl             ( early_enrnsl           ),
.frzreg                   ( frzreg                 ),
.early_frzreg             ( early_frzreg           ),
.nfrzdrv                  ( nfrzdrv                ),
.early_nfrzdrv            ( early_nfrzdrv          ),
.niotri                   ( niotri                 ),
.early_niotri             ( early_niotri           ),
.plniotri                 ( plniotri               ),
.early_plniotri           ( early_plniotri         ),
.usermode                 ( usrmode                ),
.early_usermode           ( early_usrmode          ),
.wkpullup                 ( wkpullup               ),
.local_bhniotri           ( local_bhniotri         ),
.local_enrnsl             ( local_enrnsl           ),
.local_frzreg             ( local_frzreg           ),
.local_nfrzdrv            ( local_nfrzdrv          ),
.local_niotri             ( local_niotri           ),
.local_plniotri           ( local_plniotri         ),
.local_usermode           ( local_usrmode          ),
.local_wkpullup           ( local_wkpullup         ),
.hps_to_core_ctrl_en      ( hps_to_core_ctrl_en    ),
.test_si_dll              ( test_i_n_dll           ),
.test_so_dll              ( test_i_p_pst           ),
.test_dqs_o1              ( test_dqs_o1            ),
.test_dqs_o2              ( test_dqs_o2            ),
.osc_mode                 ( x64_osc_mode_out       ),
.osc_in_p                 ( 'd0     ),
.osc_in_n                 ( 'd0     ),
.osc_out_p                ( x64_osc_chain_p[7]     ),
.osc_out_n                ( x64_osc_chain_n[7]     )
);

io_regulator__nf5es xio_regulator (
.ibp50u             ( ibp50u                 ),
.clk                ( regulator_clk          ),
.vfb                ( vcc_regphy             ),
.ibp50u_cal         ( ibp50u_cal             ),
.vcca_io            ( 1'b1                   ),
.vcc_io             ( 1'b1                   ),
.vss_io             ( 1'b0                   ),
.atbi_0             ( atbi_0                 ),
.atbi_1             ( atbi_1                 ),
.cal_done           ( lane_cal_done          ),
.vcc_regphy         ( vcc_regphy             ),
.csr_shift_n        ( 1'b1                   ),
.csr_in             ( ioereg_csrout[8]      ),
.csr_en             ( csr_en_left            ),
.csr_clk            ( csr_clk_left           ),
.csr_out            ( ioereg_csrout[9]      )
);


io_vref__nf5es vref (
.vref_ext           ( vref_ext           ),
.sel_vref           ( sel_vref[5:0]      ),
.vref_int           ( vref_int           ),
.i50u_ref           ( i50u_ref           ),
.xor_vref           ( xor_vref		 ),
.csrdin             ( ioereg_csrout[9]   ),
.csrclk             ( csr_clk_left       ),
.csren              ( csr_en_left        ),
.csr_scan_shift_n   ( csr_shift_n_left   ),
.csrdout            ( csr_out            )
);


io_gpio_osc_en__nf5es xio_gpio_osc_en (
.osc_en_n         ( osc_en_n             ),
.osc_in           ( osc_start_out         ),
.osc_out          ( osc_start_in          ),
.rocount_to_core  ( osc_rocount_to_core   )
);


`ifdef ENABLE_IO_12_LANE_ASSERTIONS


wire  phy_clk_lane;

assign phy_clk_lane = rb_phy_clk_mode_right ? phy_clk[1] : phy_clk[0];


reg     valid_state;
time    phy_clk_lane_period;
time    old_phy_clk_lane_time;
time    phy_clk_phs_period;
time    old_phy_clk_phs_time;
real    expected_ratio;
real    frequency_ratio;
reg     delayed_reset_pulse;

always @(posedge phy_clk_lane)
  begin
    phy_clk_lane_period = $time - old_phy_clk_lane_time;
    old_phy_clk_lane_time = $time;
  end

always @(posedge phy_clk_phs[0])
  begin
    phy_clk_phs_period = $time - old_phy_clk_phs_time;
    old_phy_clk_phs_time = $time;
  end

always @(*) frequency_ratio = real'(phy_clk_lane_period) / real'(phy_clk_phs_period);

always @(posedge reset_n)
  begin
     #10000 delayed_reset_pulse = reset_n;
     #1     delayed_reset_pulse = 1'b0;
  end

initial valid_state = 1;

always @(*)
  begin
    if (~reset_n) valid_state = 0;
    else if (delayed_reset_pulse)
         casez ({rb_mode_rate_out_bot[1:0],rb_mode_rate_in_bot[1:0]})
            4'b00_0? : if ((frequency_ratio > 26)  && (frequency_ratio < 38) ) begin valid_state = 1; expected_ratio = 32.0; display_good_clock; end
                       else                                                    begin valid_state = 0; expected_ratio = 32.0; display_bad_clock; end
            4'b00_10 : if ((frequency_ratio > 13)  && (frequency_ratio < 19) ) begin valid_state = 1; expected_ratio = 16.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 16.0; display_bad_clock; end
            4'b00_11 : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 8.0; display_bad_clock; end
            4'b01_0? : if ((frequency_ratio > 13)  && (frequency_ratio < 19) ) begin valid_state = 1; expected_ratio = 16.0; display_good_clock; end
                       else                                                    begin valid_state = 0; expected_ratio = 16.0; display_bad_clock; end
            4'b01_10 : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 8.0; display_bad_clock; end
            4'b01_11 : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 4.0; display_bad_clock; end
            4'b10_0? : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 8.0; display_bad_clock; end
            4'b10_10 : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 4.0; display_bad_clock; end
            4'b10_11 : if ((frequency_ratio > 1.6) && (frequency_ratio < 2.4)) begin valid_state = 1; expected_ratio = 2.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 2.0; display_bad_clock; end
            4'b11_0? : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 4.0; display_bad_clock; end
            4'b11_10 : if ((frequency_ratio > 1.6) && (frequency_ratio < 2.4)) begin valid_state = 1; expected_ratio = 2.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 2.0; display_bad_clock; end
            4'b11_11 : if ((frequency_ratio > 0.8) && (frequency_ratio < 1.2)) begin valid_state = 1; expected_ratio = 1.0; display_good_clock;  end
                       else                                                    begin valid_state = 0; expected_ratio = 1.0; display_bad_clock; end
         endcase
    else if ($time < 300) valid_state = 0;
    else casez ({valid_state,rb_mode_rate_out_bot[1:0],rb_mode_rate_in_bot[1:0]})
            5'b0_00_0? : if ((frequency_ratio > 26)  && (frequency_ratio < 38) ) begin valid_state = 1; expected_ratio = 32.0; display_good_clock; end
            5'b0_00_10 : if ((frequency_ratio > 13)  && (frequency_ratio < 19) ) begin valid_state = 1; expected_ratio = 16.0; display_good_clock;  end
            5'b0_00_11 : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
            5'b0_01_0? : if ((frequency_ratio > 13)  && (frequency_ratio < 19) ) begin valid_state = 1; expected_ratio = 16.0; display_good_clock; end
            5'b0_01_10 : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
            5'b0_01_11 : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
            5'b0_10_0? : if ((frequency_ratio > 6.4) && (frequency_ratio < 9.6)) begin valid_state = 1; expected_ratio = 8.0; display_good_clock;  end
            5'b0_10_10 : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
            5'b0_10_11 : if ((frequency_ratio > 1.6) && (frequency_ratio < 2.4)) begin valid_state = 1; expected_ratio = 2.0; display_good_clock;  end
            5'b0_11_0? : if ((frequency_ratio > 3.2) && (frequency_ratio < 4.8)) begin valid_state = 1; expected_ratio = 4.0; display_good_clock;  end
            5'b0_11_10 : if ((frequency_ratio > 1.6) && (frequency_ratio < 2.4)) begin valid_state = 1; expected_ratio = 2.0; display_good_clock;  end
            5'b0_11_11 : if ((frequency_ratio > 0.8) && (frequency_ratio < 1.2)) begin valid_state = 1; expected_ratio = 1.0; display_good_clock;  end
            5'b1_00_0? : if ((frequency_ratio < 26)  || (frequency_ratio > 38) ) begin valid_state = 0; expected_ratio = 32.0; display_bad_clock;  end
            5'b1_00_10 : if ((frequency_ratio < 13)  || (frequency_ratio > 19) ) begin valid_state = 0; expected_ratio = 16.0; display_bad_clock;  end
            5'b1_00_11 : if ((frequency_ratio < 6.4) || (frequency_ratio > 9.6)) begin valid_state = 0; expected_ratio = 8.0; display_bad_clock;  end
            5'b1_01_0? : if ((frequency_ratio < 13)  || (frequency_ratio > 19) ) begin valid_state = 0; expected_ratio = 16.0; display_bad_clock;  end
            5'b1_01_10 : if ((frequency_ratio < 6.4) || (frequency_ratio > 9.6)) begin valid_state = 0; expected_ratio = 8.0; display_bad_clock;  end
            5'b1_01_11 : if ((frequency_ratio < 3.2) || (frequency_ratio > 4.8)) begin valid_state = 0; expected_ratio = 4.0; display_bad_clock;  end
            5'b1_10_0? : if ((frequency_ratio < 6.4) || (frequency_ratio > 9.6)) begin valid_state = 0; expected_ratio = 8.0; display_bad_clock;  end
            5'b1_10_10 : if ((frequency_ratio < 3.2) || (frequency_ratio > 4.8)) begin valid_state = 0; expected_ratio = 4.0; display_bad_clock;  end
            5'b1_10_11 : if ((frequency_ratio < 1.6) || (frequency_ratio > 2.4)) begin valid_state = 0; expected_ratio = 2.0; display_bad_clock;  end
            5'b1_11_0? : if ((frequency_ratio < 3.2) || (frequency_ratio > 4.8)) begin valid_state = 0; expected_ratio = 4.0; display_bad_clock;  end
            5'b1_11_10 : if ((frequency_ratio < 1.6) || (frequency_ratio > 2.4)) begin valid_state = 0; expected_ratio = 2.0; display_bad_clock;  end
            5'b1_11_11 : if ((frequency_ratio < 0.8) || (frequency_ratio > 1.2)) begin valid_state = 0; expected_ratio = 1.0; display_bad_clock;  end
         endcase
  end

task display_good_clock;
begin
  //$display("\nIO_12_LANE Message, The clock ratio of phy_clk_lane/phy_clk_phs is good, the expected ratio is %f,  the actual ratio is %f", expected_ratio, frequency_ratio );
  //$display("Time = %t,\n  %m\n",$time);
end
endtask

task display_bad_clock;
begin
  //$display("\nIO_12_LANE Message, The clock ratio of phy_clk_lane/phy_clk_phs is bad, the expected ratio is %f,  the actual ratio is %f", expected_ratio, frequency_ratio );
  //$display("Time = %t,\n  %m\n",$time);
end
endtask

`endif

endmodule
