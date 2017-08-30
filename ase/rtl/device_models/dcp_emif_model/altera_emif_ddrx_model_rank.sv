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



module altera_emif_ddrx_model_rank
    # (
   parameter PROTOCOL_ENUM                                           = "",
   parameter PORT_MEM_BA_WIDTH                                       = 1,
   parameter PORT_MEM_BG_WIDTH                                       = 1,
   parameter PORT_MEM_C_WIDTH                                        = 1,
   parameter PORT_MEM_A_WIDTH                                        = 1,
   parameter MEM_CHIP_ID_WIDTH                                       = 0,
   parameter MEM_ROW_ADDR_WIDTH                                      = 1,
   parameter MEM_COL_ADDR_WIDTH                                      = 1,
   parameter PORT_MEM_DM_WIDTH                                       = 1,
   parameter PORT_MEM_DBI_N_WIDTH                                    = 1,
   parameter PORT_MEM_DQS_WIDTH                                      = 1,
   parameter PORT_MEM_DQ_WIDTH                                       = 1,
   parameter MEM_DM_EN                                               = 0,
   parameter MEM_PAR_ALERT_PW                                        = 48,
   parameter MEM_TRTP                                                = 0,
   parameter MEM_TRCD                                                = 0,
   parameter MEM_DQS_TO_CLK_CAPTURE_DELAY                            = 100,
   parameter MEM_CLK_TO_DQS_CAPTURE_DELAY                            = 100000,
   parameter MEM_MIRROR_ADDRESSING                                   = 0,
   parameter MEM_DEPTH_IDX                                           = -1,
   parameter MEM_WIDTH_IDX                                           = 0,
   parameter MEM_RANK_IDX                                            = -1,
   parameter MEM_VERBOSE                                             = 1,
   parameter MEM_GUARANTEED_WRITE_INIT                               = 0,
   parameter REFRESH_BURST_VALIDATION                                = 0,
   parameter MEM_INIT_MRS0                                           = 0,
   parameter MEM_INIT_MRS1                                           = 0,
   parameter MEM_INIT_MRS2                                           = 0,
   parameter MEM_INIT_MRS3                                           = 0,
   parameter MEM_CFG_GEN_SBE                                         = 0,
   parameter MEM_CFG_GEN_DBE                                         = 0,
   parameter MEM_MICRON_AUTOMATA                                     = 0
   ) (

   input  logic                      [PORT_MEM_A_WIDTH-1:0]          mem_a,
   input  logic                      [PORT_MEM_BA_WIDTH-1:0]         mem_ba,
   input  logic                      [PORT_MEM_BG_WIDTH-1:0]         mem_bg,
   input  logic                      [PORT_MEM_C_WIDTH-1:0]          mem_c,
   input  logic                                                      mem_ck,
   input  logic                                                      mem_ck_n,
   input  logic                                                      mem_cke,
   input  logic                                                      mem_ras_n,
   input  logic                                                      mem_cas_n,
   input  logic                                                      mem_we_n,
   input  logic                                                      mem_act_n,
   input  logic                                                      mem_reset_n,
   input  logic                     [PORT_MEM_DM_WIDTH-1:0]          mem_dm,
   inout  tri                       [PORT_MEM_DBI_N_WIDTH-1:0]       mem_dbi_n,
   inout  tri                       [PORT_MEM_DQ_WIDTH-1:0]          mem_dq,
   inout  tri                       [PORT_MEM_DQS_WIDTH-1:0]         mem_dqs,
   inout  tri                       [PORT_MEM_DQS_WIDTH-1:0]         mem_dqs_n,
   input  logic                                                      mem_odt,
   input  logic                                                      mem_cs_n,
   output logic                                                      mem_alert_n,
   input  logic                                                      mem_par
   );
   timeunit 1ps;
   timeprecision 1ps;


   localparam MEM_DQS_GROUP_SIZE                                     = PORT_MEM_DQ_WIDTH / PORT_MEM_DQS_WIDTH;
   localparam ALERT_N_PIPELINE_SIZE                                  = 2 * (MEM_PAR_ALERT_PW+16) + 1;
   localparam DISABLE_NOP_DISPLAY                                    = 1;
   localparam CHECK_VIOLATIONS                                       = 1;
   localparam REFRESH_INTERVAL_PS                                    = 36000000;
   localparam FULL_BURST_REFRESH_COUNT                               = 8192;
   localparam STD_REFRESH_INTERVAL_PS                                = 7800000;
   localparam MAX_LATENCY                                            = 64;
   localparam MAX_BURST                                              = 8;
   localparam OPCODE_WIDTH                                           = 5;

   localparam INT_MEM_A_WIDTH                                        = ((PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
                                                                        ? (2 * PORT_MEM_A_WIDTH) : PORT_MEM_A_WIDTH);
   localparam INT_MEM_BA_WIDTH                                       = ((PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
                                                                        ? 3 : PORT_MEM_BA_WIDTH);

   localparam NUM_STACK_LEVELS                                       = ((PROTOCOL_ENUM == "PROTOCOL_DDR4") ? (2**MEM_CHIP_ID_WIDTH) : 1);

   localparam NUM_BANKS_PER_GROUP                                    = 2**INT_MEM_BA_WIDTH;
   localparam NUM_BANK_GROUPS                                        = 2**PORT_MEM_BG_WIDTH;
   localparam NUM_BANKS                                              = NUM_BANKS_PER_GROUP * NUM_BANK_GROUPS;

   wire                             [INT_MEM_A_WIDTH - 1:0]          mem_a_wire;
   wire                             [INT_MEM_BA_WIDTH - 1:0]         mem_ba_wire;
   wire                             [PORT_MEM_BG_WIDTH - 1:0]        mem_bg_wire;

   reg                              [PORT_MEM_A_WIDTH - 1:0]         mem_a_posedge;
   reg                              [PORT_MEM_A_WIDTH - 1:0]         mem_a_negedge;

   wire                             [PORT_MEM_DQS_WIDTH - 1:0]       mem_dqs_shifted;
   wire                             [PORT_MEM_DQS_WIDTH - 1:0]       mem_dqs_n_shifted;

   wire                             [PORT_MEM_DQS_WIDTH - 1:0]       mem_dqs_n_shifted_2;
   reg                              [PORT_MEM_DQS_WIDTH - 1:0]       mem_dqs_n_shifted_2_prev = 'z;


   typedef enum logic[OPCODE_WIDTH-1:0] {
      OPCODE_PRECHARGE     = 'b01010,
      OPCODE_ACTIVATE      = 'b01011,
      OPCODE_DDR4_ACTIVATE = 'b00xxx,
      OPCODE_WRITE         = 'b01100,
      OPCODE_READ          = 'b01101,
      OPCODE_MRS           = 'b01000,
      OPCODE_REFRESH       = 'b01001,
      OPCODE_DES           = 'b1xxxx,
      OPCODE_ZQC           = 'b01110,
      OPCODE_NOP           = 'b01111
   } OPCODE_TYPE;

   typedef enum logic[OPCODE_WIDTH-1:0] {
      LPDDR3_OPCODE_PRECHARGE = 'b0_1101,
      LPDDR3_OPCODE_ACTIVATE  = 'b0_01xx,
      LPDDR3_OPCODE_WRITE     = 'b0_100x,
      LPDDR3_OPCODE_READ      = 'b0_101x,
      LPDDR3_OPCODE_MRW       = 'b0_0000,
      LPDDR3_OPCODE_MRR       = 'b0_0001,
      LPDDR3_OPCODE_REFRESH   = 'b0_001x,
      LPDDR3_OPCODE_DES       = 'b1_xxxx,
      LPDDR3_OPCODE_NOP       = 'b0_111x
   } LPDDR3_OPCODE_TYPE;

   typedef enum {
      CA_TRAINING_OFF,
      CA_TRAINING_MR41,
      CA_TRAINING_MR48
   } CA_TRAINING_MODE;

   typedef enum {

      DDR_BURST_TYPE_BL16,
      DDR_BURST_TYPE_BL8,
      DDR_BURST_TYPE_OTF,
      DDR_BURST_TYPE_BL4

   } DDR_BURST_TYPE;

   typedef enum {

      DDR_AL_TYPE_ZERO,
      DDR_AL_TYPE_CL_MINUS_1,
      DDR_AL_TYPE_CL_MINUS_2,
      DDR_AL_TYPE_CL_MINUS_3

   } DDR_AL_TYPE;

   DDR_BURST_TYPE                          burst_type;
   int                                     cas_latency;
   int                                     cas_write_latency;
   DDR_AL_TYPE                             al_type;
   int                                     parity_latency;
   bit                                     wlevel_en;
   bit [1:0]                               lpasr;
   bit                                     mpr_mode;
   bit [1:0]                               mpr_page;
   bit [1:0]                               mpr_read_format;
   bit                                     read_preamble_training_mode;
   bit                                     read_preamble_2ck_mode;
   bit                                     geardown_mode;
   bit [2:0]                               fine_granularity_refresh_mode;
   bit                                     max_power_saving_en;
   bit                                     temp_controlled_refresh_range;
   bit                                     temp_controlled_refresh_en;

   int tRTP_cycles                         = MEM_TRTP;
   int tRCD_cycles                         = MEM_TRCD;

   int                                     clock_cycle;

   reg                                     clock_stable;

   time                                    last_refresh_time;
   bit                                     refresh_burst_active;
   int                                     refresh_executed_count;
   int                                     refresh_debt;
   time                                    refresh_required_time;

   CA_TRAINING_MODE                        lpddr3_ca_training = CA_TRAINING_OFF;

   typedef struct {

      bit   [MEM_ROW_ADDR_WIDTH - 1:0]  opened_row;
      time                              last_ref_time;
      int                               last_ref_cycle;
      int                               last_activate_cycle;
      int                               last_precharge_cycle;
      int                               last_write_cmd_cycle;
      int                               last_write_access_cycle;
      int                               last_read_cmd_cycle;
      int                               last_read_access_cycle;

   } bank_struct;

   typedef struct {
      bank_struct bank[NUM_BANKS_PER_GROUP-1:0];
   } bg_struct;

   typedef struct {
      bg_struct bg[NUM_BANK_GROUPS - 1:0];
   } stack_level_struct;

   stack_level_struct device [NUM_STACK_LEVELS - 1:0];

   bit [PORT_MEM_DQ_WIDTH - 1:0]           mem_data[*];
   bit [3:0][7:0]                          mpr_p0_data;

   typedef enum {

      DDR_CMD_TYPE_PRECHARGE,
      DDR_CMD_TYPE_ACTIVATE,
      DDR_CMD_TYPE_WRITE,
      DDR_CMD_TYPE_READ,
      DDR_CMD_TYPE_REFRESH,
      DDR_CMD_TYPE_NOP,
      DDR_CMD_TYPE_MRS,
      DDR_CMD_TYPE_MRW,
      DDR_CMD_TYPE_MRR,
      DDR_CMD_TYPE_DES,
      DDR_CMD_TYPE_ZQC,
      DDR_CMD_TYPE_ERROR

   } DDR_CMD_TYPE;

   typedef struct {
      DDR_CMD_TYPE                      cmd_type;
      int                               word_count;
      int                               burst_length;
      bit   [INT_MEM_BA_WIDTH - 1:0]    bank;
      bit   [PORT_MEM_BG_WIDTH - 1:0]   bank_group;
      bit   [PORT_MEM_C_WIDTH - 1:0]    chip_id;
      bit   [INT_MEM_A_WIDTH - 1:0]     address;
      bit   [OPCODE_WIDTH-1:0]          opcode;
   } command_struct;

   typedef enum {
      RTT_DISABLED,
      RTT_RZQ_2,
      RTT_RZQ_3,
      RTT_RZQ_4,
      RTT_RZQ_5,
      RTT_RZQ_6,
      RTT_RZQ_7,
      RTT_RZQ_8,
      RTT_RZQ_9,
      RTT_RZQ_10,
      RTT_RZQ_11,
      RTT_RZQ_12,
      RTT_RESERVED,
      RTT_UNKNOWN
   } RTT_TERM_TYPE;

   typedef struct {
      RTT_TERM_TYPE rtt_nom;
      RTT_TERM_TYPE rtt_drv;
      RTT_TERM_TYPE rtt_wr;
   } rtt_struct;


   DDR_CMD_TYPE                               write_command_queue[$];
   int                                        write_word_count_queue[$];
   int                                        write_burst_length_queue[$];
   bit         [INT_MEM_A_WIDTH - 1:0]        write_address_queue[$];
   bit         [INT_MEM_BA_WIDTH - 1:0]       write_bank_queue[$];
   bit         [PORT_MEM_BG_WIDTH - 1:0]      write_bank_group_queue[$];
   bit         [PORT_MEM_C_WIDTH-1:0]         write_chip_id_queue[$];

   DDR_CMD_TYPE read_command_queue[$];
   int                                        read_word_count_queue[$];
   int                                        read_burst_length_queue[$];
   bit        [INT_MEM_A_WIDTH - 1:0]         read_address_queue[$];
   bit        [INT_MEM_BA_WIDTH - 1:0]        read_bank_queue[$];
   bit        [PORT_MEM_BG_WIDTH - 1:0]       read_bank_group_queue[$];
   bit        [PORT_MEM_C_WIDTH-1:0]          read_chip_id_queue[$];

   DDR_CMD_TYPE precharge_command_queue[$];
   bit        [INT_MEM_BA_WIDTH - 1:0]        precharge_bank_queue[$];
   bit        [PORT_MEM_BG_WIDTH - 1:0]       precharge_bank_group_queue[$];
   bit        [PORT_MEM_C_WIDTH-1:0]          precharge_chip_id_queue[$];

   DDR_CMD_TYPE activate_command_queue[$];
   bit        [INT_MEM_A_WIDTH-1:0]           activate_row_queue[$];
   bit        [INT_MEM_BA_WIDTH-1:0]          activate_bank_queue[$];
   bit        [PORT_MEM_BG_WIDTH-1:0]         activate_bank_group_queue[$];
   bit        [PORT_MEM_C_WIDTH-1:0]          activate_chip_id_queue[$];

   command_struct parity_latency_queue[$];
   bit        [2 * MAX_LATENCY + 1:0]         parity_latency_pipeline;
   bit        [ALERT_N_PIPELINE_SIZE:0]       parity_alert_n_pipeline;

   command_struct                             active_command;
   command_struct                             new_command;
   command_struct                             precharge_command;
   command_struct                             activate_command;
   rtt_struct rtt_values;

   bit        [2 * MAX_LATENCY + 1:0]         read_command_pipeline;
   bit        [2 * MAX_LATENCY + 1:0]         write_command_pipeline;
   bit        [2 * MAX_LATENCY + 1:0]         precharge_command_pipeline;
   bit        [2 * MAX_LATENCY + 1:0]         activate_command_pipeline;

   reg        [PORT_MEM_DQ_WIDTH - 1:0]       mem_dq_from_mem;
   reg        [PORT_MEM_DQ_WIDTH - 1:0]       mem_dq_int;
   reg        [PORT_MEM_DQ_WIDTH - 1:0]       mem_dq_ca_map;
   reg        [PORT_MEM_DQ_WIDTH - 1:0]       mem_dq_captured;
   reg        [PORT_MEM_DQ_WIDTH - 1:0]       mem_ck_sampled_by_dqs;
   reg        [PORT_MEM_DQS_WIDTH - 1:0]      mem_dm_captured;
   bit                                        mem_dq_en;
   bit                                        mem_dqs_en;
   bit                                        mem_dqs_preamble_no_toggle;
   bit                                        mem_dqs_preamble_toggle;
   bit                                        mem_dqs_pod_pullup;
   wire       [PORT_MEM_DQ_WIDTH - 1:0]       full_mask;
   logic      [PORT_MEM_DQ_WIDTH - 1:0]       full_dbi_n;
   wire       [PORT_MEM_DQ_WIDTH - 1:0]       full_dbi_n_in;
   reg        [PORT_MEM_DQS_WIDTH - 1:0]      dbi_n;

   time                                       mem_dqs_time[PORT_MEM_DQS_WIDTH];
   time                                       mem_ck_time;

   bit                                        wdbi_en;
   bit                                        rdbi_en;
   bit                                        dm_n_en;


   function automatic string bank_str (input [PORT_MEM_C_WIDTH-1:0] chip_id, input [PORT_MEM_BG_WIDTH-1:0] bank_group, input [INT_MEM_BA_WIDTH-1:0] bank);
      string result;
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         $sformat(result, "C [ %0h ] - BG [ %0h ] - BANK [ %0h ]", chip_id, bank_group, bank);
      end else begin
         $sformat(result, "BANK [ %0h ]", bank);
      end
      return result;
   endfunction

   task init_guaranteed_write (input integer option);

      static int burst_length = 8;
      static int other_bank = 3;
      bit [32-1:0] five_s;
      bit [32-1:0] a_s;

      int i;
      command_struct cmd;

      $display("Pre-initializing memory for guaranteed write");

      if (option == -1) begin
         $display("option=%0d: distorting guaranteed write data", option);
         five_s = 32'h55554;
         a_s     = 32'hAAAAB;
      end else begin
         five_s = 32'h55555;
         a_s     = 32'hAAAAA;
      end

      cmd.word_count = 0;
      cmd.burst_length = burst_length;
      cmd.address = 0;
      cmd.bank = 0;
      cmd.bank_group = 0;
      cmd.chip_id = 0;

      if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
         cmd.opcode = LPDDR3_OPCODE_WRITE;
      else
         cmd.opcode = OPCODE_WRITE;

      cmd.address = burst_length;
      cmd.bank = 0;
      for (i = 0; i < burst_length; i++) begin
         cmd.word_count = i;
         write_memory(cmd, five_s, '0, '0);
      end

      cmd.address = 0;
      cmd.bank = other_bank;
      for (i = 0; i < burst_length; i++) begin
         cmd.word_count = i;
         write_memory(cmd, five_s, '0, '0);
      end

      cmd.address = burst_length;
      cmd.bank = other_bank;
      for (i = 0; i < burst_length; i++) begin
         cmd.word_count = i;
         write_memory(cmd, a_s, '0, '0);
      end

      cmd.address = 0;
      cmd.bank = 0;
      for (i = 0; i < burst_length; i++) begin
         cmd.word_count = i;
         write_memory(cmd, a_s, '0, '0);
      end

   endtask

   function automatic int min;
      input int a;
      input int b;
      int result = (a < b) ? a : b;
      return result;
   endfunction

   task automatic initialize_db;
      while (write_command_queue.size() > 0)
         write_command_queue.delete(0);
      while (write_word_count_queue.size() > 0)
         write_word_count_queue.delete(0);
      while (write_burst_length_queue.size() > 0)
         write_burst_length_queue.delete(0);
      while (write_address_queue.size() > 0)
         write_address_queue.delete(0);
      while (write_bank_queue.size() > 0)
         write_bank_queue.delete(0);

      while (read_command_queue.size() > 0)
         read_command_queue.delete(0);
      while (read_word_count_queue.size() > 0)
         read_word_count_queue.delete(0);
      while (read_burst_length_queue.size() > 0)
         read_burst_length_queue.delete(0);
      while (read_address_queue.size() > 0)
         read_address_queue.delete(0);
      while (read_bank_queue.size() > 0)
         read_bank_queue.delete(0);

      while (precharge_command_queue.size() > 0)
         precharge_command_queue.delete(0);
      while (precharge_bank_queue.size() > 0)
         precharge_bank_queue.delete(0);

      while (activate_command_queue.size() > 0)
         activate_command_queue.delete(0);
      while (activate_bank_queue.size() > 0)
         activate_bank_queue.delete(0);
      while (activate_row_queue.size() > 0)
         activate_row_queue.delete(0);

      mem_data.delete();
   endtask

   task automatic set_cas_latency (input bit [3:0] code);
      if(PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         case(code)
            4'b0000 : cas_latency = 9;
            4'b0001 : cas_latency = 10;
            4'b0010 : cas_latency = 11;
            4'b0011 : cas_latency = 12;
            4'b0100 : cas_latency = 13;
            4'b0101 : cas_latency = 14;
            4'b0110 : cas_latency = 15;
            4'b0111 : cas_latency = 16;
            4'b1101 : cas_latency = 17;
            4'b1000 : cas_latency = 18;
            4'b1110 : cas_latency = 19;
            4'b1001 : cas_latency = 20;
            4'b1111 : cas_latency = 21;
            4'b1010 : cas_latency = 22;
            4'b1011 : cas_latency = 24;
            default: begin
               $display("Error: Use of reserved DDR4 CAS latency code : %b", code);
               $stop(1);
            end
         endcase
      end else if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3") begin
         case(code)
            4'b0001 : cas_latency = 3;
            4'b0100 : cas_latency = 6;
            4'b0110 : cas_latency = 8;
            4'b0111 : cas_latency = 9;
            4'b1000 : cas_latency = 10;
            4'b1001 : cas_latency = 11;
            4'b1010 : cas_latency = 12;
            4'b1100 : cas_latency = 14;
            4'b1110 : cas_latency = 16;
            default : begin
               $display("Error: Use of a reserved LPDDR3 READ latency code : %b", code);
               $stop(1);
            end
         endcase
      end else begin
         case(code)
            4'b0001 : cas_latency = 5;
            4'b0010 : cas_latency = 6;
            4'b0011 : cas_latency = 7;
            4'b0100 : cas_latency = 8;
            4'b0101 : cas_latency = 9;
            4'b0110 : cas_latency = 10;
            4'b0111 : cas_latency = 11;
            4'b1000 : cas_latency = 12;
            4'b1001 : cas_latency = 13;
            4'b1010 : cas_latency = 14;
            default: begin
            end
         endcase
      end

      if (MEM_VERBOSE) begin
         $display("   CAS LATENCY set to : %0d", cas_latency);
      end

   endtask

   task automatic set_additive_latency (input bit [1:0] code);
      case(code)
         3'b00 : begin
            if (MEM_VERBOSE)
               $display("   Setting Additive CAS LATENCY to 0");
            al_type = DDR_AL_TYPE_ZERO;
         end
         3'b01 : begin
            if (MEM_VERBOSE)
               $display("   Setting Additive CAS LATENCY to CL - 1");
            al_type = DDR_AL_TYPE_CL_MINUS_1;
         end
         3'b10 : begin
            if (MEM_VERBOSE)
               $display("   Setting Additive CAS LATENCY to CL - 2");
            al_type = DDR_AL_TYPE_CL_MINUS_2;
         end
         3'b11 : begin
            if (MEM_VERBOSE)
               $display("   Setting Additive CAS LATENCY to CL - 3");
            al_type = DDR_AL_TYPE_CL_MINUS_3;
         end
      endcase
   endtask

   task automatic set_write_leveling_mode (input bit code);
      wlevel_en = code;
      if (MEM_VERBOSE)
         $display("   Setting write_leveling mode to %d", wlevel_en);
   endtask

   function automatic int get_additive_latency;
      int additive_latency = 0;
      case(al_type)
         DDR_AL_TYPE_ZERO : begin
         end
         DDR_AL_TYPE_CL_MINUS_1 : begin
            additive_latency = cas_latency - 1;
         end
         DDR_AL_TYPE_CL_MINUS_2 : begin
            additive_latency = cas_latency - 2;
         end
         DDR_AL_TYPE_CL_MINUS_3 : begin
            additive_latency = cas_latency - 3;
         end
         default : begin
            $display("Error: Unknown additive latency type: %0d", al_type);
         end
      endcase
      return additive_latency;
    endfunction

   task automatic set_parity_latency (input bit [2:0] code);

      int i;

      case(code)
         3'b000 : begin
            if (MEM_VERBOSE)
               $display("   Setting A/C parity to DISABLED");
            parity_latency = 0;
         end
         3'b001 : begin
            if (MEM_VERBOSE)
               $display("   Setting A/C parity to 4CK");
            parity_latency = 4;
         end
         3'b010 : begin
            if (MEM_VERBOSE)
               $display("   Setting A/C parity to 5CK");
            parity_latency = 5;
         end
         3'b011 : begin
            if (MEM_VERBOSE)
               $display("   Setting A/C parity to 6CK");
            parity_latency = 6;
         end
         3'b100 : begin
            if (MEM_VERBOSE)
               $display("   Setting A/C parity to 8CK");
            parity_latency = 8;
         end
         default : begin
            $display("Error: Use of reserved A/C parity latency code : %b", code);
            $stop(1);
         end
      endcase

      while (parity_latency_queue.size() > 0)
         parity_latency_queue.delete(0);

      for (i = 0; i < 2 * MAX_LATENCY; i++) begin
         parity_latency_pipeline[i] = 0;
      end

   endtask

   function automatic int get_read_latency;
      int read_latency = cas_latency + get_additive_latency();
      return read_latency;
   endfunction

   function automatic int get_write_latency;
      int write_latency = cas_write_latency + get_additive_latency();
      return write_latency;
   endfunction

   function automatic int get_precharge_latency;
      return tRTP_cycles + get_additive_latency();
   endfunction

   task automatic set_cas_write_latency (input bit [4:0] code);
      if(PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         case(code[2:0])
            3'b000 : cas_write_latency = 9;
            3'b001 : cas_write_latency = 10;
            3'b010 : cas_write_latency = 11;
            3'b011 : cas_write_latency = 12;
            3'b100 : cas_write_latency = 14;
            3'b101 : cas_write_latency = 16;
            3'b110 : cas_write_latency = 18;
            default : begin
               $display("Error: Use of reserved DDR4 CAS WRITE latency code : %b", code);
               $stop(1);
            end
         endcase
      end else if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3") begin
         casex(code[4:0])
            5'bx_0001 : cas_write_latency = 1;
            5'bx_0100 : cas_write_latency = 3;
            5'bx_0110 : cas_write_latency = 4;
            5'bx_0111 : cas_write_latency = 5;
            5'b0_1000 : cas_write_latency = 6;
            5'b0_1001 : cas_write_latency = 6;
            5'b0_1010 : cas_write_latency = 6;
            5'b0_1100 : cas_write_latency = 8;
            5'b0_1110 : cas_write_latency = 8;
            5'b1_1000 : cas_write_latency = 8;
            5'b1_1001 : cas_write_latency = 9;
            5'b1_1010 : cas_write_latency = 9;
            5'b1_1100 : cas_write_latency = 11;
            5'b1_1110 : cas_write_latency = 13;
            default : begin
               $display("Error: Use of reserved LPDDR3 WRITE latency code : %b", code);
               $stop(1);
            end
         endcase
      end else begin
         case(code[2:0])
            3'b000 : cas_write_latency = 5;
            3'b001 : cas_write_latency = 6;
            3'b010 : cas_write_latency = 7;
            3'b011 : cas_write_latency = 8;
            3'b100 : cas_write_latency = 9;
            3'b101 : cas_write_latency = 10;
            default : begin
               $display("Error: Use of reserved CAS WRITE latency code : %b", code);
               $stop(1);
            end
         endcase
      end
      if (MEM_VERBOSE)
         $display("   CAS WRITE LATENCY set to : %0d", cas_write_latency);
   endtask

   task automatic set_rtt_nom (input bit [2:0] code);
        case (code)
            3'b000: rtt_values.rtt_nom = RTT_DISABLED;
            3'b001: rtt_values.rtt_nom = RTT_RZQ_4;
            3'b010: rtt_values.rtt_nom = RTT_RZQ_2;
            3'b011: rtt_values.rtt_nom = RTT_RZQ_6;
            3'b100: rtt_values.rtt_nom = RTT_RZQ_12;
            3'b101: rtt_values.rtt_nom = RTT_RZQ_8;
            default:rtt_values.rtt_nom = RTT_RESERVED;
        endcase
        if (MEM_VERBOSE) $display("   RTT_NOM set to : %s (%m)", rtt_values.rtt_nom.name());
    endtask

    task automatic set_rtt_drv (input bit [1:0] code);
        case (code)
            2'b00:   rtt_values.rtt_drv = RTT_RZQ_6;
            2'b01:   rtt_values.rtt_drv = RTT_RZQ_7;
            default: rtt_values.rtt_drv = RTT_RESERVED;
        endcase
        if (MEM_VERBOSE) $display("   RTT_DRV set to : %s (%m)", rtt_values.rtt_drv.name());
    endtask

    task automatic set_rtt_wr (input bit [1:0] code);
        case (code)
            2'b00: rtt_values.rtt_wr = RTT_DISABLED;
            2'b01: rtt_values.rtt_wr = RTT_RZQ_4;
            2'b10: rtt_values.rtt_wr = RTT_RZQ_2;
            2'b11: rtt_values.rtt_wr = RTT_RESERVED;
        endcase
        if (MEM_VERBOSE) $display("   RTT_WR set to : %s (%m)", rtt_values.rtt_wr.name());
    endtask

   task automatic reset_dll (input bit code);
      if(code == 1'b1) begin
         if (MEM_VERBOSE)
            $display("   Resetting DLL");
      end
   endtask

   task automatic set_burst_type (input bit [1:0] burst_mode);
      case (burst_mode)
         2'b00 : begin
            if (MEM_VERBOSE)
               $display("   Setting burst length Fixed BL8");
            burst_type = DDR_BURST_TYPE_BL8;
         end
         2'b01 : begin
            if (MEM_VERBOSE)
               $display("   Setting burst length on-the-fly");
            burst_type = DDR_BURST_TYPE_OTF;
         end
         2'b10 : begin
            if (MEM_VERBOSE)
               $display("   Setting burst length Fixed BL4");
            burst_type = DDR_BURST_TYPE_BL4;
         end
         default : begin
            $display("ERROR: Invalid burst type mode %0d specified!", burst_mode);
            $finish(1);
         end
      endcase
   endtask

   task automatic set_lpasr (input bit [1:0] code);
      if (code ^ lpasr) begin
         case (code)
            2'b00 : begin
               if (MEM_VERBOSE)
                  $display("   Setting low power array self refresh mode: Manual, Normal temperature range");
            end
            2'b01 : begin
               if (MEM_VERBOSE)
                  $display("   Setting low power array self refresh mode: Manual, Reduced temperature range");
            end
            2'b10 : begin
               if (MEM_VERBOSE)
                  $display("   Setting low power array self refresh mode: Manual, Extended temperature range");
            end
            2'b11 : begin
               if (MEM_VERBOSE)
                  $display("   Setting low power array self refresh mode: Auto self-refresh");
            end
            default : begin
               $display("ERROR: Invalid low power array self refresh mode %0d specified!", code);
               $finish(1);
            end
         endcase
         lpasr = code;
         $display("    Low power array self refresh mode behavior is not implemented in this memory model.");
      end
   endtask

   task automatic set_mpr_mode (input bit [4:0] code);

      mpr_mode = code[0];
      mpr_page = code[2:1];
      mpr_read_format = code[4:3];

      if (code[0]) begin
         if (code[2:1] != 2'b00) begin
            $display("ERROR: MPR page %0d is unsupported!", code);
            $finish(1);
         end
         if (code[4:3] != 2'b00) begin
            $display("ERROR: MPR read format %0d is unsupported!", code);
            $finish(1);
         end
         $display("    MPR access is on.");

      end else begin
         $display("    MPR access is off.");
      end
   endtask

   task automatic set_geardown_mode (input bit code);
      if (code ^ geardown_mode) begin
         $display("    Setting geardown mode: %d", code);
         if (code)
            $display("    Geardown mode behavior is not implemented in this memory model.");
         geardown_mode = code;
      end
   endtask

   task automatic set_fine_granularity_refresh_mode (input bit [2:0] code);
      if (code ^ fine_granularity_refresh_mode) begin
         case (code)
            3'b000 : begin
               if (MEM_VERBOSE)
                  $display("   Setting fine granularity refresh mode: Fixed 1x");
            end
            3'b001 : begin
               if (MEM_VERBOSE)
                  $display("   Setting fine granularity refresh mode: Fixed 2x");
            end
            3'b010 : begin
               if (MEM_VERBOSE)
                  $display("   Setting fine granularity refresh mode: Fixed 4x");
            end
            3'b101 : begin
               if (MEM_VERBOSE)
                  $display("   Setting fine granularity refresh mode: On-the-fly 2x");
            end
            3'b110 : begin
               if (MEM_VERBOSE)
                  $display("   Setting fine granularity refresh mode: On-the-fly 4x");
            end
            default : begin
               $display("ERROR: Invalid fine granularity refresh mode %0d specified!", code);
               $finish(1);
            end
         endcase
         fine_granularity_refresh_mode = code;
         $display("    Fine granularity refresh mode behavior is not implemented in this memory model.");
      end
   endtask

   task automatic set_max_power_saving (input bit code);
      if (code ^ max_power_saving_en) begin
         $display("    Setting maximum power saving mode: %d", code);
         if (code)
            $display("    Maximum power saving mode behavior is not implemented in this memory model.");
         max_power_saving_en = code;
      end
   endtask
   task automatic set_temp_controlled_refresh_range(input bit code);
      if (code ^ temp_controlled_refresh_range) begin
         $display("    Setting temperature controlled refresh range: %d", code);
         temp_controlled_refresh_range = code;
      end
   endtask
   task automatic set_temp_controlled_refresh_enable(input bit code);
      if (code ^ temp_controlled_refresh_en) begin
         $display("    Setting temperature controlled refresh enable: %d", code);
         if (code)
            $display("    Temperature controlled refresh behavior is not implemented in this memory model.");
         temp_controlled_refresh_en = code;
      end
   endtask
   task automatic set_read_preamble_training_mode(input bit code);
      if (code ^ read_preamble_training_mode) begin
         $display("    Setting read preamble training mode: %d", code);
         if (code)
            $display("    Read preamble training mode behavior is not fully implemented in this memory model.");
         read_preamble_training_mode = code;
      end
   endtask
   task automatic set_read_preamble_2ck_mode(input bit code);
      if (code ^ read_preamble_2ck_mode) begin
         $display("    Setting read preamble 2ck mode: %d", code);
         read_preamble_2ck_mode = code;
      end
   endtask

   task automatic cmd_nop;
      if (MEM_VERBOSE && !DISABLE_NOP_DISPLAY)
         $display("[%0t] [DWR=%0d%0d%0d]:  NOP Command", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
   endtask

   task automatic cmd_des;
      if (MEM_VERBOSE && !DISABLE_NOP_DISPLAY)
         $display("[%0t] [DWR=%0d%0d%0d]:  DES Command", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
   endtask

   task automatic cmd_zqc;
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         if (new_command.chip_id !== 0) begin
            $display("Error: ZQC commands sent with chip_id != 0");
            $stop(1);
         end
      end

      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  ZQC Command", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
   endtask


   task automatic cmd_unknown;
      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  WARNING: Unknown Command (OPCODE %b). Command ignored.", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.opcode);
   endtask

   task automatic cmd_set_activate;
      int activate_latency = min(get_read_latency(), get_write_latency()) + 1;

      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  ACTIVATE (queue) - %s - ROW [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), new_command.address);
      activate_command_queue.push_back(DDR_CMD_TYPE_ACTIVATE);
      activate_row_queue.push_back(new_command.address);
      activate_bank_queue.push_back(new_command.bank);
      activate_bank_group_queue.push_back(new_command.bank_group);
      activate_chip_id_queue.push_back(new_command.chip_id);
      activate_command_pipeline[ 2 * activate_latency ] = 1;
      device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_activate_cycle = clock_cycle;
   endtask

   task automatic cmd_activate(bit [PORT_MEM_C_WIDTH-1:0] chip_id, bit [PORT_MEM_BG_WIDTH-1:0] bank_group, bit [INT_MEM_BA_WIDTH-1:0] bank, bit [INT_MEM_A_WIDTH-1:0] address);
      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  ACTIVATE (execute) - %s - ROW [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(chip_id, bank_group, bank), address);
      device[chip_id].bg[bank_group].bank[bank].opened_row = address;
   endtask

   task automatic cmd_precharge(bit [PORT_MEM_C_WIDTH-1:0] chip_id, bit [PORT_MEM_BG_WIDTH-1:0] bank_group, bit [INT_MEM_BA_WIDTH-1:0] bank, bit all_banks);
      if (MEM_VERBOSE)
         if(all_banks)
            $display("[%0t] [DWR=%0d%0d%0d]:  PRECHARGE - C [ %0h ] - ALL BANKS", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, chip_id);
         else
            $display("[%0t] [DWR=%0d%0d%0d]:  PRECHARGE - %s", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(chip_id, bank_group, bank));
      device[chip_id].bg[bank_group].bank[bank].last_precharge_cycle = clock_cycle;
   endtask

   task automatic cmd_mrs;
      int mrs_idx;

      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         mrs_idx = {new_command.bank_group[0], new_command.bank[1:0]};

         if (PORT_MEM_BG_WIDTH > 1) begin
            if (new_command.bank_group[1] !== 1'b0) begin
               $display("Error: BG1 must be programmed to 0 during MRS");
               $stop(1);
            end
         end
         if (INT_MEM_A_WIDTH >= 18) begin
            if (new_command.address[17] !== 1'b0) begin
               $display("Error: A17 must be programmed to 0 during MRS");
               $stop(1);
            end
         end
         if (new_command.address[13] !== 1'b0) begin
            $display("Error: A13 must be programmed to 0 during MRS");
            $stop(1);
         end

         if (new_command.chip_id !== 0) begin
            $display("Error: MRS commands must be issued with chip_id 0");
            $stop(1);
         end
      end else begin
         mrs_idx = new_command.bank;
      end

      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  MRS Command - MRS [ %0d ] -> %0h", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, mrs_idx, new_command.address);

      case(mrs_idx)
         3'b000 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 0");
            set_burst_type(new_command.address[1:0]);
            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               set_cas_latency({new_command.address[6:4], new_command.address[2:2] });
            end else begin
               set_cas_latency({new_command.address[2:2], new_command.address[6:4]});
            end
            reset_dll(new_command.address[8]);
         end

         3'b001 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 1");
            set_additive_latency(new_command.address[4:3]);
            set_write_leveling_mode(new_command.address[7]);
            if (PROTOCOL_ENUM == "PROTOCOL_DDR3") begin
               set_rtt_nom({new_command.address[9],new_command.address[6],new_command.address[2]});
               set_rtt_drv({new_command.address[5],new_command.address[1]});
            end
         end

         3'b010 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 2");
            set_cas_write_latency({2'b0, new_command.address[5:3]});
            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               set_lpasr(new_command.address[7:6]);
            end
            else if (PROTOCOL_ENUM == "PROTOCOL_DDR3") begin
               set_rtt_wr(new_command.address[10:9]);
            end
         end

         3'b011 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 3");
            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               set_mpr_mode({new_command.address[12:11], new_command.address[1:0], new_command.address[2]});
               set_geardown_mode(new_command.address[3]);
               set_fine_granularity_refresh_mode(new_command.address[8:6]);
            end
         end

         3'b100 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 4");
            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               set_max_power_saving(new_command.address[1]);
               set_temp_controlled_refresh_range(new_command.address[2]);
               set_temp_controlled_refresh_enable(new_command.address[3]);
               set_read_preamble_training_mode(new_command.address[10]);
               set_read_preamble_2ck_mode(new_command.address[11]);
            end
         end

         3'b101 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 5");
               if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
                  set_parity_latency(new_command.address[2:0]);
                  dm_n_en = new_command.address[10];
                  wdbi_en = new_command.address[11];
                  rdbi_en = new_command.address[12];
               end
            end

         3'b110 : begin
            if (MEM_VERBOSE)
               $display("   MRS - 6: not supported");
         end

         3'b111 : begin
            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               if (MEM_VERBOSE) begin
                  $display("   Detected RCD/DB Control Word");
               end
            end else begin
               $display("Error: MRS Invalid Bank Address: %0d", mrs_idx);
               $stop(1);
            end
         end
      endcase
   endtask

   task automatic cmd_mrr;
      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  MRR Command - MRR [ %0d ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.address[11:4]);

      $display("Warning: MRR not implemented");
   endtask

   task automatic cmd_mrw;
      if (lpddr3_ca_training == CA_TRAINING_OFF || new_command.address[11:4] == 41 || new_command.address[11:4] == 42 || new_command.address[11:4] == 48) begin
         if (MEM_VERBOSE)
            $display("[%0t] [DWR=%0d%0d%0d]:  MRW Command - MRW [ %0d ] -> %0h", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.address[11:4], new_command.address[19:12]);

         case (new_command.address[11:4])
            1 : begin
            end
            2 : begin
               set_cas_write_latency({new_command.address[18], new_command.address[15:12]});
               set_cas_latency(new_command.address[15:12]);
            end
            3 : begin
            end
            9 : begin
               $display("   Warning: MRW 9 not implemented in model");
            end
            10 : begin
               $display("   Warning: MRW 10 not implemented in model");
            end
            11 : begin
               $display("   Warning: MRW 11 not implemented in model");
            end
            16 : begin
               $display("   Warning: setting PASR bank mask not implemented in model");
            end
            17 : begin
               $display("   Warning: setting PASR segment mask not implemented in model");
            end
            41 : begin
               lpddr3_ca_training = CA_TRAINING_MR41;
               $display("   CA Training Phase 1");
            end
            42 : begin
               lpddr3_ca_training = CA_TRAINING_OFF;
               $display("   CA Training End");
            end
            48 : begin
               lpddr3_ca_training = CA_TRAINING_MR48;
               $display("   CA Training Phase 2");
            end
            63 : begin
               $display("   MRW Reset Issued");
            end
            default : begin
               $display("Warning: Attempted Write to Read-Only Mode Register: %0d", new_command.address[11:4]);
            end
         endcase
      end
   endtask

   task automatic cmd_refresh(input int chip_id_num);
      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  REFRESH Command C [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, chip_id_num);

      for (int g = 0; g < NUM_BANK_GROUPS; g++) begin
         for (int b = 0; b < NUM_BANKS_PER_GROUP; b++) begin
            refresh_bank(chip_id_num, g, b);
         end
      end
   endtask

   task automatic cmd_read;
      int read_latency = get_read_latency();
      int precharge_latency = get_precharge_latency();

      int auto_precharge;
      if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
         auto_precharge = mem_a[0];
      else
         auto_precharge = mem_a_wire[10];

      if (MEM_VERBOSE) begin
         if (mpr_mode) begin
            $display("[%0t] [DWR=%0d%0d%0d]:  MPR READ - PAGE [ %0d ] - LOC[ %0d ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, mpr_page, new_command.bank);
         end else if(auto_precharge) begin
            $display("[%0t] [DWR=%0d%0d%0d]:  READ with AP (BL%0d) - %s - COL [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.burst_length, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), new_command.address);
         end else begin
            $display("[%0t] [DWR=%0d%0d%0d]:  READ (BL%0d) - %s - COL [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.burst_length, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), new_command.address);
         end
      end

      new_command.word_count = 0;
      read_command_queue.push_back(new_command.cmd_type);
      read_word_count_queue.push_back(new_command.word_count);
      read_burst_length_queue.push_back(new_command.burst_length);
      read_address_queue.push_back(new_command.address);
      read_bank_queue.push_back(new_command.bank);
      read_bank_group_queue.push_back(new_command.bank_group);
      read_chip_id_queue.push_back(new_command.chip_id);

      if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
         read_command_pipeline[(2 * read_latency) + 1] = 1;
      else
         read_command_pipeline[2 * read_latency] = 1;

      device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_read_cmd_cycle = clock_cycle;

      if (!mpr_mode) begin
         refresh_bank(new_command.chip_id, new_command.bank_group, new_command.bank);
      end

      if(auto_precharge) begin
         precharge_command_queue.push_back(DDR_CMD_TYPE_PRECHARGE);
         precharge_bank_queue.push_back(new_command.bank);
         precharge_bank_group_queue.push_back(new_command.bank_group);
         precharge_chip_id_queue.push_back(new_command.chip_id);
         precharge_command_pipeline[ 2 * precharge_latency ] = 1;
      end
   endtask

   task automatic cmd_write;
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4" && mpr_mode) begin
         mpr_p0_data[new_command.bank] = new_command.address[7:0];
         $display("[%0t] [DWR=%0d%0d%0d]:  MPR WRITE - PAGE [ %d ] - LOC[ %0d ] - DATA = %0h", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, mpr_page, new_command.bank, new_command.address[7:0]);

      end else begin
         int write_latency = get_write_latency();

         int auto_precharge;
         if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
            auto_precharge = mem_a[0];
         else
            auto_precharge = mem_a_wire[10];

         if (MEM_VERBOSE) begin
            if(auto_precharge)
               $display("[%0t] [DWR=%0d%0d%0d]:  WRITE with AP (BL%0d) - %s - COL [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.burst_length, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), new_command.address);
            else
               $display("[%0t] [DWR=%0d%0d%0d]:  WRITE (BL%0d) - %s - COL [ %0h ]", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, new_command.burst_length, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), new_command.address);
         end

         new_command.word_count = 0;
         write_command_queue.push_back(new_command.cmd_type);
         write_word_count_queue.push_back(new_command.word_count);
         write_burst_length_queue.push_back(new_command.burst_length);
         write_address_queue.push_back(new_command.address);
         write_bank_queue.push_back(new_command.bank);
         write_bank_group_queue.push_back(new_command.bank_group);
         write_chip_id_queue.push_back(new_command.chip_id);

         if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3")
            write_command_pipeline[(2 * write_latency) + 1] = 1'b1;
         else
            write_command_pipeline[2 * write_latency] = 1'b1;

         device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_write_cmd_cycle = clock_cycle;
      end
   endtask

   task automatic refresh_bank(input int chip_id_num, input int bank_group_num, input int bank_num);
      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  Refreshing - %s", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(chip_id_num, bank_group_num, bank_num));
      device[chip_id_num].bg[bank_group_num].bank[bank_num].last_ref_time = $time;
      device[chip_id_num].bg[bank_group_num].bank[bank_num].last_ref_cycle = clock_cycle;
   endtask

   task automatic init_banks;
      int c, b,g;
      for (c = 0; c < NUM_STACK_LEVELS; c++) begin
         for (g = 0; g < NUM_BANK_GROUPS; g++) begin
            for (b = 0; b < NUM_BANKS_PER_GROUP; b++) begin
               if (MEM_VERBOSE)
                  $display("[%0t] [DWR=%0d%0d%0d]:  Initializing - %s", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(c, g, b));
               device[c].bg[g].bank[b].opened_row = '0;
               device[c].bg[g].bank[b].last_ref_time = 0;
               device[c].bg[g].bank[b].last_ref_cycle = 0;
               device[c].bg[g].bank[b].last_activate_cycle = 0;
               device[c].bg[g].bank[b].last_precharge_cycle = 0;
               device[c].bg[g].bank[b].last_read_cmd_cycle = 0;
               device[c].bg[g].bank[b].last_read_access_cycle = 0;
               device[c].bg[g].bank[b].last_write_cmd_cycle = 0;
               device[c].bg[g].bank[b].last_write_access_cycle = 0;
            end
         end
      end
   endtask

   task automatic check_violations;

      /* **** *
       * tRCD *
       * **** */

      if(new_command.cmd_type == DDR_CMD_TYPE_READ) begin
         if(!mpr_mode && device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_activate_cycle > device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_read_cmd_cycle + get_additive_latency() - tRCD_cycles) begin
            $display("[%0t] [DWR=%0d%0d%0d]:  ERROR: tRCD violation (READ) on %s @ cycle %0d", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), clock_cycle);
            $display("    tRCD = %0d", tRCD_cycles);
            $display("    Last ACTIVATE @ %0d", device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_activate_cycle);
            $display("    Last READ CMD @ %0d", device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_read_cmd_cycle);
            $finish(1);
         end
      end
      if(new_command.cmd_type == DDR_CMD_TYPE_WRITE) begin
         if(!mpr_mode && device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_activate_cycle > device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_write_cmd_cycle + get_additive_latency() - tRCD_cycles) begin
            $display("[%0t] [DWR=%0d%0d%0d]:  ERROR: tRCD violation (WRITE) on %s @ cycle %0d", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, bank_str(new_command.chip_id, new_command.bank_group, new_command.bank), clock_cycle);
            $display("    tRCD = %0d", tRCD_cycles);
            $display("    Last ACTIVATE @ %0d", device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_activate_cycle);
            $display("    Last WRITE CMD @ %0d", device[new_command.chip_id].bg[new_command.bank_group].bank[new_command.bank].last_write_cmd_cycle);
            $finish(1);
         end
      end
   endtask

   task write_memory(
      input command_struct write_command,
      input [PORT_MEM_DQ_WIDTH - 1:0] write_data,
      input [PORT_MEM_DQ_WIDTH - 1:0] data_mask,
      input [PORT_MEM_DQ_WIDTH - 1:0] dbi_n);

      bit [PORT_MEM_C_WIDTH - 1:0] chip_id;
      bit [PORT_MEM_BG_WIDTH - 1:0] bank_group;
      bit [INT_MEM_BA_WIDTH - 1:0] bank_address;
      bit [MEM_ROW_ADDR_WIDTH - 1:0] row_address;
      bit [MEM_COL_ADDR_WIDTH - 1:0] col_address;
      bit [PORT_MEM_BG_WIDTH + INT_MEM_BA_WIDTH + MEM_ROW_ADDR_WIDTH + MEM_COL_ADDR_WIDTH - 1 : 0] address;
      bit [PORT_MEM_DQ_WIDTH - 1:0] masked_data;

      integer i;

      chip_id = write_command.chip_id;
      bank_group = write_command.bank_group;
      bank_address = write_command.bank;
      row_address = device[chip_id].bg[bank_group].bank[bank_address].opened_row;
      col_address = write_command.address;
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         address = {chip_id, bank_group, bank_address, row_address, col_address} + write_command.word_count;
      end else begin
         address = {bank_address, row_address, col_address} + write_command.word_count;
      end

      for(i = 0; i < PORT_MEM_DQ_WIDTH; i = i + 1) begin
         if (data_mask[i] !== 0 && data_mask[i] !== 1)
            masked_data[i] = 'x;
         else if (wdbi_en) begin
            masked_data[i] = dbi_n[i] ? write_data[i] : ~write_data[i];
         end else if (PROTOCOL_ENUM == "PROTOCOL_DDR4" ? ~data_mask[i] : data_mask[i])
         begin
            if (mem_data.exists(address))
               masked_data[i] = mem_data[address][i];
            else
               masked_data[i] = 'x;
         end
         else
            masked_data[i] = write_data[i];
      end

      if (MEM_VERBOSE)
         $display("[%0t] [DWR=%0d%0d%0d]:  Writing data %h (%h/%h) @ %0h (CGBRC=%0h/%0h/%0h/%0h/%0h ) burst %0d",
            $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, masked_data, write_data, PROTOCOL_ENUM == "PROTOCOL_DDR4" ? data_mask : ~data_mask, address, chip_id, bank_group, bank_address, row_address, col_address, write_command.word_count);

      mem_data[address] = masked_data;
      device[chip_id].bg[bank_group].bank[bank_address].last_write_access_cycle = clock_cycle;
   endtask

   task read_memory(
      input command_struct read_command,
      output [PORT_MEM_DQ_WIDTH - 1:0] read_data,
      output [PORT_MEM_DQS_WIDTH - 1:0] dbi_n);

      bit [PORT_MEM_C_WIDTH - 1:0] chip_id;
      bit [PORT_MEM_BG_WIDTH - 1:0] bank_group;
      bit [INT_MEM_BA_WIDTH - 1:0] bank_address;
      bit [MEM_ROW_ADDR_WIDTH - 1:0] row_address;
      bit [MEM_COL_ADDR_WIDTH - 1:0] col_address;
      bit [PORT_MEM_BG_WIDTH + INT_MEM_BA_WIDTH + MEM_ROW_ADDR_WIDTH + MEM_COL_ADDR_WIDTH - 1 : 0] address;
      reg  [1:0] int_error_inject;
      integer bit_index;

      chip_id = read_command.chip_id;
      bank_group = read_command.bank_group;
      bank_address = read_command.bank;
      row_address = device[chip_id].bg[bank_group].bank[bank_address].opened_row;
      col_address = read_command.address;
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         address = {chip_id, bank_group, bank_address, row_address, col_address} + read_command.word_count;
      end else begin
         address = {bank_address, row_address, col_address} + read_command.word_count;
      end

      if (mpr_mode) begin
         read_data = {PORT_MEM_DQ_WIDTH{mpr_p0_data[read_command.bank][7 - read_command.word_count]}};
         if (MEM_VERBOSE)
            $display("[%0t] [DWR=%0d%0d%0d]:  Reading MPR data %h @ %0d %0d burst %0d",
               $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, read_data, mpr_page, read_command.bank, read_command.word_count);

      end else if (mem_data.exists(address)) begin
         integer i, j;
         if (rdbi_en) begin
            for (i = 0; i < PORT_MEM_DQS_WIDTH; i = i + 1) begin
               integer sum;
               sum = 0;
               for (j = 0; j < (MEM_DQS_GROUP_SIZE); j = j + 1) begin
                  sum = sum + mem_data[address][i*(MEM_DQS_GROUP_SIZE) + j];
               end
               dbi_n[i] = sum >= 4;
            end
            read_data = mem_data[address];
         end else begin
            dbi_n = 'z;
            if (MEM_MICRON_AUTOMATA && address [2:0] >= 3'b100)
                read_data = {PORT_MEM_DQ_WIDTH{1'b0}};
            else
                read_data = mem_data[address];
         end
         for (i = 0; i < PORT_MEM_DQ_WIDTH; i = i + 1) begin: dbi_n_in_mapping
             full_dbi_n [i] = dbi_n[i / MEM_DQS_GROUP_SIZE];
         end

         if (MEM_CFG_GEN_SBE == 1) begin
            int_error_inject = 2'b01;
         end
         else if (MEM_CFG_GEN_DBE == 1) begin
            int_error_inject = 2'b11;
         end
         else begin
            int_error_inject = 2'b00;
         end
         bit_index = {$random} % PORT_MEM_DQ_WIDTH;
         read_data[bit_index] = read_data[bit_index] ^ int_error_inject[0];
         if (bit_index < PORT_MEM_DQ_WIDTH-1) begin
            read_data[bit_index+1] = read_data[bit_index+1] ^ int_error_inject[1];
         end

         if (MEM_VERBOSE)
            $display("[%0t] [DWR=%0d%0d%0d]:  Reading data %h @ %0h (CGBRC=%0h/%0h/%0h/%0h/%0h ) burst %0d",
               $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, read_data, address, chip_id, bank_group, bank_address, row_address, col_address, read_command.word_count);
      end
      else begin
         if (MEM_VERBOSE)
            $display("[%0t] [DWR=%0d%0d%0d]:  WARNING: Attempting to read from uninitialized location @ %0h (CGBRC=%0h/%0h/%0h/%0h/%0h) burst %0d",
                $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, address, chip_id, bank_group, bank_address, row_address, col_address, read_command.word_count);

         if (rdbi_en) begin
            read_data = '1;
            dbi_n = '1;
            full_dbi_n = '1;
         end else begin
            read_data = '0;
            dbi_n = 'z;
            full_dbi_n = 'z;
         end
      end

      device[chip_id].bg[bank_group].bank[bank_address].last_read_access_cycle = clock_cycle;
   endtask

   if(MEM_MIRROR_ADDRESSING) begin
      if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
         if (PORT_MEM_A_WIDTH > 14) begin
            assign mem_a_wire = {mem_a[PORT_MEM_A_WIDTH - 1:14], mem_a[11], mem_a[12], mem_a[13], mem_a[10:9], mem_a[7], mem_a[8], mem_a[5], mem_a[6], mem_a[3], mem_a[4], mem_a[2:0]};
         end else begin
            assign mem_a_wire = {mem_a[11], mem_a[12], mem_a[13], mem_a[10:9], mem_a[7], mem_a[8], mem_a[5], mem_a[6], mem_a[3], mem_a[4], mem_a[2:0]};
         end

         if(PORT_MEM_BA_WIDTH > 2) begin
            assign mem_ba_wire = {mem_ba[PORT_MEM_BA_WIDTH - 1:2], mem_ba[0], mem_ba[1]};
         end else if(PORT_MEM_BA_WIDTH > 1) begin
            assign mem_ba_wire = {mem_ba[0], mem_ba[1]};
         end else begin
            assign mem_ba_wire = mem_ba;
         end

         if(PORT_MEM_BG_WIDTH > 2) begin
            assign mem_bg_wire = {mem_bg[PORT_MEM_BG_WIDTH - 1:2], mem_bg[0], mem_bg[1]};
         end else if(PORT_MEM_BG_WIDTH > 1) begin
            assign mem_bg_wire = {mem_bg[0], mem_bg[1]};
         end else begin
            assign mem_bg_wire = mem_bg;
         end

      end else begin
         assign mem_a_wire = {mem_a[PORT_MEM_A_WIDTH - 1:9], mem_a[7], mem_a[8], mem_a[5], mem_a[6], mem_a[3], mem_a[4], mem_a[2:0]};

         if(PORT_MEM_BA_WIDTH > 2) begin
            assign mem_ba_wire = {mem_ba[PORT_MEM_BA_WIDTH - 1:2], mem_ba[0], mem_ba[1]};
         end else begin
            assign mem_ba_wire = {mem_ba[0], mem_ba[1]};
         end

         assign mem_bg_wire = mem_bg;
      end
   end else begin
      assign mem_a_wire = mem_a;
      assign mem_ba_wire = mem_ba;
      assign mem_bg_wire = mem_bg;
   end

   logic mem_ck_diff;
   logic mem_ck_ca;
   always @(posedge mem_ck) begin
      if (mem_cke == 1'b1) begin
         #8 mem_ck_diff <= mem_ck;
      end

      if (lpddr3_ca_training != CA_TRAINING_OFF) begin
         #8 mem_ck_ca <= mem_ck;
      end
   end

   always @(posedge mem_ck_n) begin
      if (mem_cke == 1'b1) begin
         #8 mem_ck_diff <= ~mem_ck_n;
      end

      if (lpddr3_ca_training != CA_TRAINING_OFF) begin
         #8 mem_ck_ca <= mem_ck;
      end
   end

   initial begin
      int i;

      $display("Intel FPGA Generic DDRx Memory Model");
      if (MEM_VERBOSE) begin
         $display("[%0t] [DWR=%0d%0d%0d]:  Max refresh interval of %0d ps", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, REFRESH_INTERVAL_PS);
      end

      clock_cycle = 0;
      clock_stable = 1'b0;
      initialize_db;
      set_burst_type(2'b0);
      init_banks();

      mem_data.delete();

      if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3") begin
         set_burst_type(2'b0);
         set_cas_latency(MEM_INIT_MRS2[3:0]);
         set_cas_write_latency({MEM_INIT_MRS2[7], MEM_INIT_MRS2[3:0]});
         set_additive_latency(2'b0);
         lpddr3_ca_training = CA_TRAINING_OFF;
         mem_dq_ca_map = 16'b0;
      end else begin
         if (MEM_VERBOSE) begin
            $display("   MRS - 0");
         end

         set_burst_type(MEM_INIT_MRS0[1:0]);
         if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
            set_cas_latency({MEM_INIT_MRS0[6:4], MEM_INIT_MRS0[2]});
         end else begin
            set_cas_latency({MEM_INIT_MRS0[2], MEM_INIT_MRS0[6:4]});
         end

         if (MEM_VERBOSE) begin
            $display("   MRS - 1");
         end

         set_additive_latency(MEM_INIT_MRS1[4:3]);

         if (MEM_VERBOSE) begin
            $display("   MRS - 2");
         end

         set_cas_write_latency({2'b0, MEM_INIT_MRS2[5:3]});

         if (MEM_VERBOSE) begin
            $display("   MRS - 3: not supported");
         end
      end

      parity_latency = 0;
      wdbi_en = 0;
      rdbi_en = 0;
      mpr_mode = 0;
      read_preamble_training_mode = 0;
      read_preamble_2ck_mode = 0;
      max_power_saving_en = 0;
      temp_controlled_refresh_range = 0;
      temp_controlled_refresh_en = 0;

      if (MEM_GUARANTEED_WRITE_INIT != 0) begin
         init_guaranteed_write(MEM_GUARANTEED_WRITE_INIT);
      end

      active_command.cmd_type <= DDR_CMD_TYPE_NOP;

      for (i = 0; i < 2 * MAX_LATENCY; i++) begin
         read_command_pipeline[i] = 0;
         write_command_pipeline[i] = 0;
         parity_latency_pipeline[i] = 0;
      end

      for (i = 0; i <= ALERT_N_PIPELINE_SIZE; i++) begin
         parity_alert_n_pipeline[i] = 1'b1;
      end

      last_refresh_time = 0;
      refresh_burst_active = 0;
      refresh_executed_count = 0;
      refresh_required_time = 0;
      refresh_debt = 0;
      mem_ck_sampled_by_dqs = '0;
   end

   always @ (posedge mem_ck) begin
      clock_cycle <= clock_cycle + 1;
      if (clock_cycle == 4) clock_stable <= 1'b1;
   end

   wire [MEM_COL_ADDR_WIDTH-1:0] col_addr;
   generate
      if(MEM_COL_ADDR_WIDTH <= 10) begin : col_addr_gen1
         assign col_addr = mem_a_wire[9:0];
      end
      else if(MEM_COL_ADDR_WIDTH == 11) begin : col_addr_gen2
         assign col_addr = {mem_a_wire[11],mem_a_wire[9:0]};
      end
      else begin : col_addr_gen3
         assign col_addr = {mem_a_wire[MEM_COL_ADDR_WIDTH+1:13],mem_a_wire[11],mem_a_wire[9:0]};
      end
   endgenerate

   always @ (posedge mem_ck_diff or negedge mem_ck_diff) begin
      int i;

      mem_ck_time = $time;
      read_command_pipeline = read_command_pipeline >> 1;
      write_command_pipeline = write_command_pipeline >> 1;
      activate_command_pipeline = activate_command_pipeline >> 1;
      parity_latency_pipeline = parity_latency_pipeline >> 1;
      parity_alert_n_pipeline = parity_alert_n_pipeline >> 1;

      parity_alert_n_pipeline[ALERT_N_PIPELINE_SIZE] = 1'b1;

      if(mem_ck_diff && clock_stable && (PROTOCOL_ENUM != "PROTOCOL_LPDDR3")) begin
         new_command.bank = mem_ba_wire;
         new_command.bank_group = mem_bg_wire;
         new_command.chip_id = mem_c;
         new_command.word_count = 0;
         if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
            new_command.opcode = {mem_cs_n, mem_act_n, mem_ras_n, mem_cas_n, mem_we_n};
         end else begin
            new_command.opcode = {mem_cs_n, 1'b1, mem_ras_n, mem_cas_n, mem_we_n};
         end

         case (burst_type)
            DDR_BURST_TYPE_BL8 : new_command.burst_length = 8;
            DDR_BURST_TYPE_BL4 : new_command.burst_length = 4;
            DDR_BURST_TYPE_OTF : new_command.burst_length = (mem_a_wire[12]) ? 8 : 4;
         endcase

         casex (new_command.opcode)
            OPCODE_PRECHARGE : new_command.cmd_type = DDR_CMD_TYPE_PRECHARGE;
            OPCODE_ACTIVATE : new_command.cmd_type = DDR_CMD_TYPE_ACTIVATE;
            OPCODE_DDR4_ACTIVATE : new_command.cmd_type = DDR_CMD_TYPE_ACTIVATE;
            OPCODE_WRITE : new_command.cmd_type = DDR_CMD_TYPE_WRITE;
            OPCODE_READ : new_command.cmd_type = DDR_CMD_TYPE_READ;
            OPCODE_MRS : new_command.cmd_type = DDR_CMD_TYPE_MRS;
            OPCODE_REFRESH : new_command.cmd_type = DDR_CMD_TYPE_REFRESH;
            OPCODE_NOP : new_command.cmd_type = DDR_CMD_TYPE_NOP;
            OPCODE_DES : new_command.cmd_type = DDR_CMD_TYPE_DES;
            OPCODE_ZQC : new_command.cmd_type = DDR_CMD_TYPE_ZQC;
            default : new_command.cmd_type = DDR_CMD_TYPE_ERROR;
         endcase

         new_command.address = mem_a_wire;
         if(new_command.cmd_type == DDR_CMD_TYPE_READ || new_command.cmd_type == DDR_CMD_TYPE_WRITE) begin
            new_command.address = {'0,col_addr};
         end

         if (REFRESH_BURST_VALIDATION) begin
            if (new_command.cmd_type == DDR_CMD_TYPE_REFRESH) begin
               if (!refresh_burst_active) begin
                  refresh_burst_active = 1;
                  refresh_executed_count = 1;
                  refresh_required_time = mem_ck_time - last_refresh_time;
                  $display("[%0t] [DWR=%0d%0d%0d]:  Time since last refresh %0t ps", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_required_time);
                  last_refresh_time = mem_ck_time;
               end else begin
                  refresh_executed_count = refresh_executed_count + 1;
               end
            end else if (new_command.cmd_type == DDR_CMD_TYPE_NOP || new_command.cmd_type == DDR_CMD_TYPE_DES) begin
            end else begin
               if (refresh_burst_active) begin
                  refresh_burst_active = 0;
                  if (refresh_executed_count >= FULL_BURST_REFRESH_COUNT)
                     refresh_debt = -(STD_REFRESH_INTERVAL_PS * 9);
                  else
                     refresh_debt = refresh_debt + (refresh_required_time - (STD_REFRESH_INTERVAL_PS * refresh_executed_count));

                  if (refresh_debt > STD_REFRESH_INTERVAL_PS * 9) begin
                     $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: REFRESH interval has exceeded allowable buffer! %0d refreshes executed. Debt: %0t ps",
                      $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_executed_count, refresh_debt);
                     $finish(1);
                  end else begin
                     $display("[%0t] [DWR=%0d%0d%0d]:  REFRESH burst complete! %0d refreshes executed. Buffer: %0d ps",
                     $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_executed_count, refresh_debt);
                  end
               end
            end
         end

         if (parity_latency > 0) begin

            reg my_parity;
            my_parity = ^{mem_a, mem_ba, mem_bg, mem_act_n, mem_c};
            if (mem_cs_n == 1'b0) begin
               if (my_parity != mem_par) begin
                  for (i = 0; i < 2*parity_latency + 2*(parity_latency + MEM_PAR_ALERT_PW); i = i + 1) begin
                     if (i >= 2*parity_latency) begin
                        parity_alert_n_pipeline[i] = 1'b0;
                     end
                  end
               end else begin
                  parity_latency_queue.push_back(new_command);
                  parity_latency_pipeline[2*parity_latency] = 1'b1;
               end
            end

            if (parity_latency_pipeline[0]) begin
               if (parity_latency_queue.size() == 0) begin
                 $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: Parity latency command queue empty but commands expected!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
                 $stop(1);
              end else begin
                 new_command = parity_latency_queue.pop_front();
              end
            end else begin
              new_command.cmd_type = DDR_CMD_TYPE_DES;
            end

            if (parity_alert_n_pipeline[0] == 1'b0) begin
               new_command.cmd_type = DDR_CMD_TYPE_ERROR;
            end
         end

         case (new_command.cmd_type)
            DDR_CMD_TYPE_NOP : cmd_nop();
            DDR_CMD_TYPE_DES : cmd_des();
            DDR_CMD_TYPE_ZQC : cmd_zqc();
            DDR_CMD_TYPE_ERROR : cmd_unknown();
            DDR_CMD_TYPE_ACTIVATE : cmd_set_activate();
            DDR_CMD_TYPE_PRECHARGE : cmd_precharge(new_command.chip_id, new_command.bank_group, new_command.bank, mem_a_wire[10]);
            DDR_CMD_TYPE_WRITE : cmd_write();
            DDR_CMD_TYPE_READ : cmd_read();
            DDR_CMD_TYPE_MRS : cmd_mrs();
            DDR_CMD_TYPE_REFRESH : cmd_refresh(new_command.chip_id);
         endcase

         if(CHECK_VIOLATIONS)
            check_violations();

      end else if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3") begin
         if (mem_ck_diff && clock_stable) begin
            new_command.bank = mem_a[9:7];
            new_command.bank_group = 0;
            new_command.chip_id = 0;
            new_command.word_count = 0;
            new_command.burst_length = 8;

            mem_a_posedge = mem_a;

            new_command.opcode = {mem_cs_n, mem_a[0], mem_a[1], mem_a[2], mem_a[3]};

            casex (new_command.opcode)
               LPDDR3_OPCODE_PRECHARGE : new_command.cmd_type = DDR_CMD_TYPE_PRECHARGE;
               LPDDR3_OPCODE_ACTIVATE : new_command.cmd_type = DDR_CMD_TYPE_ACTIVATE;
               LPDDR3_OPCODE_WRITE : new_command.cmd_type = DDR_CMD_TYPE_WRITE;
               LPDDR3_OPCODE_READ : new_command.cmd_type = DDR_CMD_TYPE_READ;
               LPDDR3_OPCODE_MRW: new_command.cmd_type = DDR_CMD_TYPE_MRW;
               LPDDR3_OPCODE_MRR: new_command.cmd_type = DDR_CMD_TYPE_MRR;
               LPDDR3_OPCODE_REFRESH : new_command.cmd_type = DDR_CMD_TYPE_REFRESH;
               LPDDR3_OPCODE_NOP : new_command.cmd_type = DDR_CMD_TYPE_NOP;
               LPDDR3_OPCODE_DES : new_command.cmd_type = DDR_CMD_TYPE_DES;
               default : new_command.cmd_type = DDR_CMD_TYPE_ERROR;
            endcase

            if (REFRESH_BURST_VALIDATION) begin
               if (new_command.cmd_type == DDR_CMD_TYPE_REFRESH) begin
                  if (!refresh_burst_active) begin
                     refresh_burst_active = 1;
                     refresh_executed_count = 1;
                     refresh_required_time = mem_ck_time - last_refresh_time;
                     $display("[%0t] [DWR=%0d%0d%0d]:  Time since last refresh %0t ps", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_required_time);
                     last_refresh_time = mem_ck_time;
                  end else begin
                     refresh_executed_count = refresh_executed_count + 1;
                  end
               end else if (new_command.cmd_type == DDR_CMD_TYPE_NOP || new_command.cmd_type == DDR_CMD_TYPE_DES) begin
               end else begin
                  if (refresh_burst_active) begin
                     refresh_burst_active = 0;
                     if (refresh_executed_count >= FULL_BURST_REFRESH_COUNT)
                        refresh_debt = -(STD_REFRESH_INTERVAL_PS * 9);
                     else
                        refresh_debt = refresh_debt + (refresh_required_time - (STD_REFRESH_INTERVAL_PS * refresh_executed_count));

                     if (refresh_debt > STD_REFRESH_INTERVAL_PS * 9) begin
                        $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: REFRESH interval has exceeded allowable buffer! %0d refreshes executed. Debt: %0t ps",
                         $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_executed_count, refresh_debt);
                        $finish(1);
                     end else begin
                        $display("[%0t] [DWR=%0d%0d%0d]:  REFRESH burst complete! %0d refreshes executed. Buffer: %0d ps",
                        $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX, refresh_executed_count, refresh_debt);
                     end
                  end
               end
            end
         end else if (!mem_ck_diff && clock_stable) begin
            mem_a_negedge = mem_a;

            if ((new_command.cmd_type == DDR_CMD_TYPE_READ) || (new_command.cmd_type == DDR_CMD_TYPE_WRITE)) begin
               new_command.address = {8'b0, mem_a_negedge[9:1], mem_a_posedge[6:5], 1'b0};
            end else if (new_command.cmd_type == DDR_CMD_TYPE_ACTIVATE) begin
               new_command.address = {5'b0, mem_a_negedge[9:8], mem_a_posedge[6:2], mem_a_negedge[7:0]};
            end else begin
               new_command.address = {mem_a_negedge, mem_a_posedge};
            end

               case (new_command.cmd_type)
                  DDR_CMD_TYPE_NOP : cmd_nop();
                  DDR_CMD_TYPE_DES : cmd_des();
                  DDR_CMD_TYPE_ERROR : cmd_unknown();
                  DDR_CMD_TYPE_ACTIVATE : cmd_set_activate();
                  DDR_CMD_TYPE_PRECHARGE : cmd_precharge(new_command.chip_id, new_command.bank_group, new_command.bank, mem_a_negedge[4]);
                  DDR_CMD_TYPE_WRITE : cmd_write();
                  DDR_CMD_TYPE_READ : cmd_read();
                  DDR_CMD_TYPE_MRW : cmd_mrw();
                  DDR_CMD_TYPE_MRR : cmd_mrr();
                  DDR_CMD_TYPE_REFRESH : cmd_refresh(new_command.chip_id);
               endcase

            if (CHECK_VIOLATIONS)
               check_violations();
         end
      end


      if (read_command_pipeline[0]) begin
         if (read_command_queue.size() == 0) begin
           $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: READ command queue empty but READ commands expected!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
           $stop(1);
         end
      end

      if (write_command_pipeline[0]) begin
         if (write_command_queue.size() == 0) begin
            $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: WRITE command queue empty but WRITE commands expected!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
            $stop(1);
         end
      end

      if (active_command.cmd_type != DDR_CMD_TYPE_NOP) begin
         if (active_command.word_count == active_command.burst_length) begin
            active_command.cmd_type = DDR_CMD_TYPE_NOP;
         end
      end


      if (active_command.cmd_type == DDR_CMD_TYPE_NOP) begin

         if (read_command_pipeline[0]) begin
            active_command.cmd_type = read_command_queue.pop_front();
            active_command.word_count = read_word_count_queue.pop_front();
            active_command.burst_length = read_burst_length_queue.pop_front();
            active_command.address = read_address_queue.pop_front();
            active_command.bank = read_bank_queue.pop_front();
            active_command.bank_group = read_bank_group_queue.pop_front();
            active_command.chip_id = read_chip_id_queue.pop_front();

            if (active_command.cmd_type != DDR_CMD_TYPE_READ) begin
               $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: Expected READ command not in queue!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
               $stop(1);
            end

         end
         else if (write_command_pipeline[0]) begin
            active_command.cmd_type = write_command_queue.pop_front();
            active_command.word_count = write_word_count_queue.pop_front();
            active_command.burst_length = write_burst_length_queue.pop_front();
            active_command.address = write_address_queue.pop_front();
            active_command.bank = write_bank_queue.pop_front();
            active_command.bank_group = write_bank_group_queue.pop_front();
            active_command.chip_id = write_chip_id_queue.pop_front();

            if (active_command.cmd_type != DDR_CMD_TYPE_WRITE) begin
               $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: Expected WRITE command not in queue!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
               $stop(1);
            end
         end
         else begin
            if (read_command_pipeline[0] || write_command_pipeline[0]) begin
               $display("[%0t] [DWR=%0d%0d%0d]:  Internal Error: Active command but read/write pipeline also active!", $time, MEM_DEPTH_IDX, MEM_WIDTH_IDX, MEM_RANK_IDX);
               $stop(1);
            end
         end
      end

      if (precharge_command_pipeline[0]) begin
         precharge_command.cmd_type = precharge_command_queue.pop_front();
         precharge_command.bank = precharge_bank_queue.pop_front();
         precharge_command.bank_group = precharge_bank_group_queue.pop_front();
         precharge_command.chip_id = precharge_chip_id_queue.pop_front();
         cmd_precharge(precharge_command.chip_id, precharge_command.bank_group, precharge_command.bank, 1'b0);
      end

      if (activate_command_pipeline[0]) begin
         activate_command.cmd_type = activate_command_queue.pop_front();
         activate_command.address = activate_row_queue.pop_front();
         activate_command.bank = activate_bank_queue.pop_front();
         activate_command.bank_group = activate_bank_group_queue.pop_front();
         activate_command.chip_id = activate_chip_id_queue.pop_front();
         cmd_activate(activate_command.chip_id, activate_command.bank_group, activate_command.bank, activate_command.address);
      end

      mem_dq_en = 1'b0;
      mem_dqs_en = 1'b0;
      mem_dqs_preamble_no_toggle = 1'b0;
      mem_dqs_preamble_toggle = 1'b0;
      mem_dqs_pod_pullup = 1'b0;
      if (active_command.cmd_type == DDR_CMD_TYPE_WRITE) begin
         integer mem_ck_dqs_diff;
         integer dqs;
         logic [PORT_MEM_DQ_WIDTH - 1:0]   mem_dq_write;
         #(MEM_DQS_TO_CLK_CAPTURE_DELAY);
         mem_dq_write = '0;
         for (dqs = 0; dqs < PORT_MEM_DQS_WIDTH; dqs = dqs + 1) begin

            if (mem_ck_time > mem_dqs_time[dqs]) begin
               mem_ck_dqs_diff = -(mem_ck_time - mem_dqs_time[dqs]);
            end
            else begin
               mem_ck_dqs_diff = mem_dqs_time[dqs] - mem_ck_time;
            end

            if (mem_ck_dqs_diff >= -(MEM_CLK_TO_DQS_CAPTURE_DELAY)) begin
               mem_dq_write = mem_dq_write | (mem_dq_captured & ({MEM_DQS_GROUP_SIZE{1'b1}} << (dqs*MEM_DQS_GROUP_SIZE)));
            end
            else begin
               $display("[%0t] %s Write: mem_ck=%0t mem_dqs=%0t delta=%0d min=%0d",
               $time, mem_ck_dqs_diff >= -(MEM_CLK_TO_DQS_CAPTURE_DELAY) ? "GOOD" : "BAD",
               mem_ck_time, mem_dqs_time[dqs], mem_ck_dqs_diff, -(MEM_CLK_TO_DQS_CAPTURE_DELAY));
               mem_dq_write = mem_dq_write | ({MEM_DQS_GROUP_SIZE{1'bx}} << (dqs*MEM_DQS_GROUP_SIZE));
            end

         end

         write_memory(active_command, mem_dq_write, full_mask, full_dbi_n_in);
         active_command.word_count = active_command.word_count+1;
      end
      else if (active_command.cmd_type == DDR_CMD_TYPE_READ) begin
         if (rdbi_en) begin
            read_memory(active_command, mem_dq_from_mem, dbi_n);
            mem_dq_int = mem_dq_from_mem ^ ~full_dbi_n;
         end else
            read_memory(active_command, mem_dq_int, dbi_n);
         mem_dq_en = 1'b1;
         mem_dqs_en = 1'b1;
         active_command.word_count = active_command.word_count+1;
      end

      if (PROTOCOL_ENUM == "PROTOCOL_DDR4" && !read_preamble_training_mode) begin
         if (!mem_dqs_en) begin
            if ((read_preamble_2ck_mode && read_command_pipeline[5]) || (!read_preamble_2ck_mode && read_command_pipeline[3])) begin
               mem_dqs_en = 1'b1;
               mem_dqs_pod_pullup = 1'b1;
               mem_dqs_preamble_no_toggle = 1'b0;
               mem_dqs_preamble_toggle = 1'b0;
            end else if (read_preamble_2ck_mode && (read_command_pipeline[4] || read_command_pipeline[3])) begin
               mem_dqs_en = 1'b1;
               mem_dqs_pod_pullup = 1'b0;
               mem_dqs_preamble_no_toggle = 1'b1;
               mem_dqs_preamble_toggle = 1'b0;
            end else if (read_command_pipeline[2] || read_command_pipeline[1]) begin
               mem_dqs_en = 1'b1;
               mem_dqs_pod_pullup = 1'b0;
               mem_dqs_preamble_no_toggle = 1'b0;
               mem_dqs_preamble_toggle = 1'b1;
            end
         end
      end else begin
         if (!mem_dqs_en && (read_command_pipeline[2] | read_command_pipeline[1])) begin
            mem_dqs_en = 1'b1;
            mem_dqs_pod_pullup = 1'b0;
            mem_dqs_preamble_no_toggle = 1'b1;
            mem_dqs_preamble_toggle = 1'b0;
         end
      end
   end

   int capture_ca_negedge = 0;
   always @(posedge mem_ck_ca) begin
      if (PROTOCOL_ENUM == "PROTOCOL_LPDDR3" && lpddr3_ca_training != CA_TRAINING_OFF && mem_cs_n == 1'b0) begin
         if (lpddr3_ca_training == CA_TRAINING_MR41) begin
            mem_dq_ca_map[0] <= mem_a[0];
            mem_dq_ca_map[2] <= mem_a[1];
            mem_dq_ca_map[4] <= mem_a[2];
            mem_dq_ca_map[6] <= mem_a[3];
            mem_dq_ca_map[8] <= mem_a[5];
            mem_dq_ca_map[10] <= mem_a[6];
            mem_dq_ca_map[12] <= mem_a[7];
            mem_dq_ca_map[14] <= mem_a[8];
         end else if (lpddr3_ca_training == CA_TRAINING_MR48) begin
            mem_dq_ca_map[0] <= mem_a[4];
            mem_dq_ca_map[8] <= mem_a[9];
         end

         capture_ca_negedge = 1;
      end
   end

   always @(negedge mem_ck_ca) begin
      if (capture_ca_negedge) begin
         if (lpddr3_ca_training == CA_TRAINING_MR41) begin
            mem_dq_ca_map[1] <= mem_a[0];
            mem_dq_ca_map[3] <= mem_a[1];
            mem_dq_ca_map[5] <= mem_a[2];
            mem_dq_ca_map[7] <= mem_a[3];
            mem_dq_ca_map[9] <= mem_a[5];
            mem_dq_ca_map[11] <= mem_a[6];
            mem_dq_ca_map[13] <= mem_a[7];
            mem_dq_ca_map[15] <= mem_a[8];
         end else if (lpddr3_ca_training == CA_TRAINING_MR48) begin
            mem_dq_ca_map[1] <= mem_a[4];
            mem_dq_ca_map[9] <= mem_a[9];
         end

         capture_ca_negedge = 0;
      end
   end

   generate
     genvar dm_count;
     for (dm_count = 0; dm_count < PORT_MEM_DQS_WIDTH; dm_count = dm_count + 1) begin: dm_mapping
         assign full_mask [(dm_count + 1) * MEM_DQS_GROUP_SIZE - 1 : dm_count * MEM_DQS_GROUP_SIZE] = {MEM_DQS_GROUP_SIZE{mem_dm_captured[dm_count]}};
     end
     genvar dbi_n_count_in;
     for (dbi_n_count_in = 0; dbi_n_count_in < PORT_MEM_DQS_WIDTH; dbi_n_count_in = dbi_n_count_in + 1) begin: dbi_n_mapping
         assign full_dbi_n_in [(dbi_n_count_in + 1) * MEM_DQS_GROUP_SIZE - 1 : dbi_n_count_in * MEM_DQS_GROUP_SIZE] = {MEM_DQS_GROUP_SIZE{mem_dbi_n[dbi_n_count_in]}};
     end
   endgenerate

   assign #1 mem_dqs_shifted = mem_dqs;
   assign #1 mem_dqs_n_shifted = mem_dqs_n;
   assign #2 mem_dqs_n_shifted_2 = mem_dqs_n;

   generate
    genvar dqs;
    for (dqs = 0; dqs < PORT_MEM_DQS_WIDTH; dqs = dqs + 1) begin
      always @(posedge mem_dqs_shifted[dqs] or posedge mem_dqs_n_shifted[dqs]) begin
         if (mem_dqs_shifted[dqs] === 1'b1 || mem_dqs_n_shifted[dqs] === 1'b1) begin
            mem_dqs_time[dqs] <= $time;
            mem_dq_captured[((dqs+1)*MEM_DQS_GROUP_SIZE)-1:dqs*MEM_DQS_GROUP_SIZE] <= mem_dq[((dqs+1)*MEM_DQS_GROUP_SIZE)-1:dqs*MEM_DQS_GROUP_SIZE];

            if (PROTOCOL_ENUM == "PROTOCOL_DDR4") begin
               if (wdbi_en || dm_n_en) begin
                  mem_dm_captured[dqs] <= mem_dbi_n[dqs];
               end else begin
                  mem_dm_captured[dqs] <= 1'b1;
               end
            end else begin
               if (PORT_MEM_DM_WIDTH == PORT_MEM_DQS_WIDTH) begin
                  mem_dm_captured[dqs] <= mem_dm[dqs];
               end else begin
                  mem_dm_captured[dqs] <= 1'b0;
               end
            end

            if (mem_dqs_n_shifted_2[dqs] === 'z || mem_dqs_n_shifted_2_prev[dqs] === 'z) begin
              mem_dq_captured[((dqs+1)*MEM_DQS_GROUP_SIZE)-1:dqs*MEM_DQS_GROUP_SIZE] <= 'z;
              mem_dm_captured[dqs] <= 'z;
            end
            mem_dqs_n_shifted_2_prev[dqs] <= mem_dqs_n_shifted_2[dqs];
         end else begin
            mem_dq_captured[((dqs+1)*MEM_DQS_GROUP_SIZE)-1:dqs*MEM_DQS_GROUP_SIZE] <= 'x;
            mem_dm_captured[dqs] <= 'x;
         end
      end
      always @(posedge mem_dqs_shifted[dqs]) begin
          mem_ck_sampled_by_dqs[((dqs+1)*MEM_DQS_GROUP_SIZE)-1:dqs*MEM_DQS_GROUP_SIZE] <= {MEM_DQS_GROUP_SIZE{mem_ck_diff}};
      end
    end
   endgenerate

   assign mem_dq    = (lpddr3_ca_training == CA_TRAINING_OFF) ? (wlevel_en ? mem_ck_sampled_by_dqs : (mem_dq_en ? mem_dq_int : 'z)) : mem_dq_ca_map;
   assign mem_dbi_n = rdbi_en ? (mem_dq_en ? dbi_n : 'z) : 'z;
   assign mem_dqs   = (!mem_dqs_en)                                                      ? 'z :
                      (mem_dqs_pod_pullup)                                               ? '1 :
                      (mem_dqs_preamble_toggle)                                          ? {PORT_MEM_DQS_WIDTH{mem_ck_diff}} :
                      (mem_dqs_preamble_no_toggle && read_preamble_training_mode)        ? '0 :
                      (mem_dqs_preamble_no_toggle && PROTOCOL_ENUM == "PROTOCOL_DDR4")   ? '1 :
                      (mem_dqs_preamble_no_toggle && PROTOCOL_ENUM != "PROTOCOL_DDR4")   ? '0 :
                                                                                           {PORT_MEM_DQS_WIDTH{mem_ck_diff}};
   assign mem_dqs_n = (!mem_dqs_en)                                                      ? 'z :
                      (mem_dqs_pod_pullup)                                               ? '1 :
                      (mem_dqs_preamble_toggle)                                          ? {PORT_MEM_DQS_WIDTH{~mem_ck_diff}} :
                      (mem_dqs_preamble_no_toggle && read_preamble_training_mode)        ? '1 :
                      (mem_dqs_preamble_no_toggle && PROTOCOL_ENUM == "PROTOCOL_DDR4")   ? '0 :
                      (mem_dqs_preamble_no_toggle && PROTOCOL_ENUM != "PROTOCOL_DDR4")   ? '1 :
                                                                                           {PORT_MEM_DQS_WIDTH{~mem_ck_diff}};

   assign mem_alert_n = parity_alert_n_pipeline[0];

// synthesis translate_on

endmodule
