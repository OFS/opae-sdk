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

`timescale 1 ps / 1 ps

import verbosity_pkg::*;
import avalon_mm_pkg::*;

// emif_ddr4 implements two banks per instance
`define NUM_SLAVES 2

//---------------------------------------------------
// Constants
//---------------------------------------------------
localparam BURST_W                  = 7;
localparam MAX_BURST                = 64;
localparam MAX_LATENCY              = 2;

localparam MAX_COMMAND_IDLE         = 5;
localparam MAX_COMMAND_BACKPRESSURE = 2;
localparam MAX_DATA_IDLE            = 3;

module emif_ddr4 #(
	parameter DDR_ADDR_WIDTH = 26,
	parameter DDR_DATA_WIDTH = 512,
	parameter SYMBOL_WIDTH = 8,
	parameter NUM_SYMBOLS = (DDR_DATA_WIDTH + SYMBOL_WIDTH - 1) / SYMBOL_WIDTH
) (
	output wire         ddr4a_avmm_waitrequest,
	output wire [DDR_DATA_WIDTH-1:0] ddr4a_avmm_readdata,
	output wire         ddr4a_avmm_readdatavalid,
	input  wire [BURST_W-1:0]   ddr4a_avmm_burstcount,
	input  wire [DDR_DATA_WIDTH-1:0] ddr4a_avmm_writedata,
	input  wire [DDR_ADDR_WIDTH-1:0]  ddr4a_avmm_address,
	input  wire         ddr4a_avmm_write,
	input  wire         ddr4a_avmm_read,
	input  wire [NUM_SYMBOLS-1:0]  ddr4a_avmm_byteenable,
	output wire         ddr4a_avmm_clk_clk,

	input  wire         ddr4a_global_reset_reset_sink_reset_n,
	input  wire         ddr4a_pll_ref_clk_clock_sink_clk,

	output wire         ddr4b_avmm_waitrequest,
	output wire [DDR_DATA_WIDTH-1:0] ddr4b_avmm_readdata,
	output wire         ddr4b_avmm_readdatavalid,
	input  wire [BURST_W-1:0]   ddr4b_avmm_burstcount,
	input  wire [DDR_DATA_WIDTH-1:0] ddr4b_avmm_writedata,
	input  wire [DDR_ADDR_WIDTH-1:0]  ddr4b_avmm_address,
	input  wire         ddr4b_avmm_write,
	input  wire         ddr4b_avmm_read,
	input  wire [NUM_SYMBOLS-1:0]  ddr4b_avmm_byteenable,
	output wire         ddr4b_avmm_clk_clk
);

//---------------------------------------------------
// Data structures
//---------------------------------------------------
typedef logic [BURST_W-1:0]      Burstcount;

typedef enum bit
{
   WRITE = 0,
   READ  = 1
} Transaction;

typedef enum bit
{
   NOBURST = 0,
   BURST   = 1
} Burstmode;

// model memory banks as associative arrays
logic [DDR_DATA_WIDTH-1:0] memory[`NUM_SLAVES][*];

typedef struct
{
   Transaction                  trans;
   Burstcount                   burstcount;
   logic [DDR_ADDR_WIDTH-1:0]   addr;
   logic [DDR_DATA_WIDTH-1:0]   data       [MAX_BURST-1:0];
   logic [NUM_SYMBOLS-1:0]      byteenable [MAX_BURST-1:0];
   bit [31:0]                   cmd_delay;
   bit [31:0]                   data_idles [MAX_BURST-1:0];
} Command;

typedef struct
{
   Burstcount                    burstcount;
   logic [DDR_DATA_WIDTH-1:0]    data     [MAX_BURST-1:0];
   bit [31:0]                    latency  [MAX_BURST-1:0];
} Response;

// slave response queue
Response read_response_queue_slave[`NUM_SLAVES][$];

// ddr user clock frequency = 266.666.. Mhz
// We shift the clock because altera_avalon_mm_slave_bfm shifts
// its monitor_request() task #1, which leads to glitches in
// waitrequest and causes random errors in simulation.
assign #10 ddr4a_avmm_clk_clk = ddr4a_pll_ref_clk_clock_sink_clk;
assign #10 ddr4b_avmm_clk_clk = ddr4a_pll_ref_clk_clock_sink_clk;

altera_avalon_mm_slave_bfm #(
	.AV_ADDRESS_W               (DDR_ADDR_WIDTH),
	.AV_NUMSYMBOLS              (NUM_SYMBOLS),
	.AV_BURSTCOUNT_W            (BURST_W),
	.AV_READRESPONSE_W          (1),
	.AV_WRITERESPONSE_W         (1),
	.USE_BEGIN_TRANSFER         (0),
	.USE_BEGIN_BURST_TRANSFER   (0),
	.USE_WAIT_REQUEST           (1),
	.USE_TRANSACTIONID          (0),
	.USE_WRITERESPONSE          (1),
	.USE_READRESPONSE           (1),
	.USE_CLKEN                  (0),
	.AV_BURST_LINEWRAP          (0),
	.AV_BURST_BNDR_ONLY         (0),
	.AV_MAX_PENDING_READS       (9),
	.AV_MAX_PENDING_WRITES      (0),
	.AV_FIX_READ_LATENCY        (0),
	.AV_READ_WAIT_TIME          (1),
	.AV_WRITE_WAIT_TIME         (0),
	.REGISTER_WAITREQUEST       (0),
	.AV_REGISTERINCOMINGSIGNALS (0),
	.VHDL_ID                    (0),
	.PRINT_HELLO                (0)
)  avs_bfm_inst_ddra (
	.clk                  	(ddr4a_pll_ref_clk_clock_sink_clk),
	.reset                	(~ddr4a_global_reset_reset_sink_reset_n),

	.avs_writedata        	(ddr4a_avmm_writedata),
	.avs_readdata         	(ddr4a_avmm_readdata),
	.avs_address          	(ddr4a_avmm_address),
	.avs_waitrequest      	(ddr4a_avmm_waitrequest),
	.avs_write            	(ddr4a_avmm_write),
	.avs_read             	(ddr4a_avmm_read),
	.avs_byteenable       	(ddr4a_avmm_byteenable),
	.avs_readdatavalid    	(ddr4a_avmm_readdatavalid),
	.avs_burstcount       	(ddr4a_avmm_burstcount)
);

altera_avalon_mm_slave_bfm #(
	.AV_ADDRESS_W               (DDR_ADDR_WIDTH),
	.AV_NUMSYMBOLS              (NUM_SYMBOLS),
	.AV_BURSTCOUNT_W            (BURST_W),
	.AV_READRESPONSE_W          (1),
	.AV_WRITERESPONSE_W         (1),
	.USE_BEGIN_TRANSFER         (0),
	.USE_BEGIN_BURST_TRANSFER   (0),
	.USE_WAIT_REQUEST           (1),
	.USE_TRANSACTIONID          (0),
	.USE_WRITERESPONSE          (1),
	.USE_READRESPONSE           (1),
	.USE_CLKEN                  (0),
	.AV_BURST_LINEWRAP          (0),
	.AV_BURST_BNDR_ONLY         (0),
	.AV_MAX_PENDING_READS       (9),
	.AV_MAX_PENDING_WRITES      (0),
	.AV_FIX_READ_LATENCY        (0),
	.AV_READ_WAIT_TIME          (1),
	.AV_WRITE_WAIT_TIME         (0),
	.REGISTER_WAITREQUEST       (0),
	.AV_REGISTERINCOMINGSIGNALS (0),
	.VHDL_ID                    (0),
	.PRINT_HELLO                (0)
)  avs_bfm_inst_ddrb (
	.clk                  	(ddr4a_pll_ref_clk_clock_sink_clk),
	.reset                	(~ddr4a_global_reset_reset_sink_reset_n),

	.avs_writedata        	(ddr4b_avmm_writedata),
	.avs_readdata         	(ddr4b_avmm_readdata),
	.avs_address          	(ddr4b_avmm_address),
	.avs_waitrequest      	(ddr4b_avmm_waitrequest),
	.avs_write            	(ddr4b_avmm_write),
	.avs_read             	(ddr4b_avmm_read),
	.avs_byteenable       	(ddr4b_avmm_byteenable),
	.avs_readdatavalid    	(ddr4b_avmm_readdatavalid),
	.avs_burstcount       	(ddr4b_avmm_burstcount)
);

// Macros
`define SLAVE0 avs_bfm_inst_ddra
`define SLAVE1 avs_bfm_inst_ddrb

`define MACRO_PENDING_READ_CYCLES(SLAVE_ID) \
	int pending_read_cycles_slave_``SLAVE_ID = 0; \
	always @(posedge `SLAVE``SLAVE_ID.clk) begin \
		if (pending_read_cycles_slave_``SLAVE_ID > 0) begin \
			pending_read_cycles_slave_``SLAVE_ID--; \
		end \
	end

`define MACRO_GET_COMMAND_FROM_SLAVE(SLAVE_ID) \
	function automatic Command get_command_from_slave_``SLAVE_ID (); \
\
	Command cmd; \
\
	`SLAVE``SLAVE_ID.pop_command(); \
	cmd.burstcount          = `SLAVE``SLAVE_ID.get_command_burst_count(); \
	cmd.addr                = `SLAVE``SLAVE_ID.get_command_address(); \
\
	if (`SLAVE``SLAVE_ID.get_command_request() == REQ_WRITE) begin \
		cmd.trans = WRITE; \
		for(int i = 0; i < cmd.burstcount; i++) begin \
			cmd.data[i]       =`SLAVE``SLAVE_ID.get_command_data(i); \
			cmd.byteenable[i] =`SLAVE``SLAVE_ID.get_command_byte_enable(i); \
		end \
	end else begin \
		cmd.trans = READ; \
		for(int i = 0; i < cmd.burstcount; i++) begin \
			cmd.byteenable[i] =`SLAVE``SLAVE_ID.get_command_byte_enable(i); \
		end \
	end \
\
	return cmd; \
	endfunction

// Functions
function automatic Response create_response (
	Burstcount burstcount
);
	Response rsp;

	rsp.burstcount       = burstcount;
	for (int i = 0;i < burstcount; i++) begin
		rsp.data[i]       = $random;
		rsp.latency[i]    = $urandom_range(0, MAX_DATA_IDLE);
	end

	return rsp;
endfunction

// Expand data enable to a data mask
function automatic logic [DDR_DATA_WIDTH-1:0] get_mask (logic [NUM_SYMBOLS-1:0] byteenable);
	logic [DDR_DATA_WIDTH-1:0] mask;
	for(int i=0; i<NUM_SYMBOLS; i++) begin
		mask[i*SYMBOL_WIDTH +: SYMBOL_WIDTH] =
			byteenable[i]? {SYMBOL_WIDTH{1'b1}} : {SYMBOL_WIDTH{1'b0}};
	end
	return mask;
endfunction

function automatic Response memory_response (Command cmd, int SLAVE_ID);
	Response rsp;
	
	rsp.burstcount = (cmd.trans == READ) ? cmd.burstcount : 1;
	for(int idx = 0; idx < cmd.burstcount; idx++) begin
		if(cmd.trans == READ) begin
			rsp.data[idx] = memory[SLAVE_ID][cmd.addr+idx] & get_mask(cmd.byteenable[0]);
			rsp.latency[idx] = $urandom_range(0,MAX_LATENCY); // set a random memory response latency
		end
	end
	return rsp;
endfunction

`define MACRO_SLAVE_THREAD(SLAVE_ID) \
	always @(`SLAVE``SLAVE_ID.signal_command_received) begin \
\
	Command     actual_cmd, exp_cmd; \
	Response    rsp; \
	logic [DDR_DATA_WIDTH-1:0] mask; \
\
	automatic int backpressure_cycles; \
\
	// set random backpressure cycles for next command \
	for (int i = 0; i < MAX_BURST; i++) begin \
		backpressure_cycles = $urandom_range(0, MAX_COMMAND_BACKPRESSURE); \
		`SLAVE``SLAVE_ID.set_interface_wait_time(backpressure_cycles, i); \
	end \
\
	actual_cmd = get_command_from_slave_``SLAVE_ID(); \
\
	// set read response \
	if (actual_cmd.trans == READ) begin \
		rsp = memory_response(actual_cmd, SLAVE_ID); \
		configure_and_push_response_to_slave_``SLAVE_ID(rsp); \
		read_response_queue_slave[``SLAVE_ID].push_back(rsp); \
	end \
\
	if (actual_cmd.trans == WRITE) begin \
		for(int idx = 0; idx < actual_cmd.burstcount; idx++) begin\
			if(memory[SLAVE_ID].exists(actual_cmd.addr+idx)) begin\
				mask = get_mask(actual_cmd.byteenable[idx]); \
				memory[SLAVE_ID][actual_cmd.addr+idx] = ((memory[SLAVE_ID][actual_cmd.addr+idx] & ~mask) | \
					                                     (actual_cmd.data[idx] & mask));\
			end\
			else begin\
				memory[SLAVE_ID][actual_cmd.addr+idx] = actual_cmd.data[idx];\
			end\
		end \
	end \
end

`define MACRO_CONFIGURE_AND_PUSH_RESPONSE_TO_SLAVE(SLAVE_ID) \
task automatic configure_and_push_response_to_slave_``SLAVE_ID ( \
	Response rsp \
); \
\
	int read_response_latency; \
\
	`SLAVE``SLAVE_ID.set_response_request(REQ_READ); \
	`SLAVE``SLAVE_ID.set_response_burst_size(rsp.burstcount); \
	for (int i = 0; i < rsp.burstcount; i++) begin \
		`SLAVE``SLAVE_ID.set_response_data(rsp.data[i], i); \
\
		if (i == 0) begin \
			`SLAVE``SLAVE_ID.set_response_latency(rsp.latency[i] + pending_read_cycles_slave_``SLAVE_ID, i); \
			read_response_latency = rsp.latency[i]; \
		end else begin \
			`SLAVE``SLAVE_ID.set_response_latency(rsp.latency[i], i); \
			read_response_latency = rsp.latency[i] + read_response_latency; \
		end \
\
	end \
	`SLAVE``SLAVE_ID.push_response(); \
	pending_read_cycles_slave_``SLAVE_ID = pending_read_cycles_slave_``SLAVE_ID + read_response_latency + rsp.burstcount + 2; \
endtask

// slave 0
`MACRO_SLAVE_THREAD(0)
`MACRO_GET_COMMAND_FROM_SLAVE(0)
`MACRO_PENDING_READ_CYCLES(0)
`MACRO_CONFIGURE_AND_PUSH_RESPONSE_TO_SLAVE(0)

// slave 1
`MACRO_SLAVE_THREAD(1)
`MACRO_GET_COMMAND_FROM_SLAVE(1)
`MACRO_PENDING_READ_CYCLES(1)
`MACRO_CONFIGURE_AND_PUSH_RESPONSE_TO_SLAVE(1)

endmodule
