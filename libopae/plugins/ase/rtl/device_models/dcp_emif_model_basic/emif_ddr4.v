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
	output wire         ddr_avmm_waitrequest,
	output wire [DDR_DATA_WIDTH-1:0] ddr_avmm_readdata,
	output wire         ddr_avmm_readdatavalid,
	input  wire [BURST_W-1:0]   ddr_avmm_burstcount,
	input  wire [DDR_DATA_WIDTH-1:0] ddr_avmm_writedata,
	input  wire [DDR_ADDR_WIDTH-1:0]  ddr_avmm_address,
	input  wire         ddr_avmm_write,
	input  wire         ddr_avmm_read,
	input  wire [NUM_SYMBOLS-1:0]  ddr_avmm_byteenable,
	output wire         ddr_avmm_clk_clk,

	input  wire         ddr_global_reset_reset_sink_reset_n,
	input  wire         ddr_pll_ref_clk_clock_sink_clk
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
logic [DDR_DATA_WIDTH-1:0] memory[*];

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
Response read_response_queue_slave[$];

assign ddr_avmm_clk_clk = ddr_pll_ref_clk_clock_sink_clk;

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
	.AV_MAX_PENDING_READS       (0),
	.AV_MAX_PENDING_WRITES      (0),
	.AV_FIX_READ_LATENCY        (0),
	.AV_READ_WAIT_TIME          (1),
	.AV_WRITE_WAIT_TIME         (0),
	.REGISTER_WAITREQUEST       (0),
	.AV_REGISTERINCOMINGSIGNALS (0),
	.AV_WAITREQUEST_ALLOWANCE   (1),
	.VHDL_ID                    (0),
	.PRINT_HELLO                (0)
)  avs_bfm_inst (
	.clk                  	(ddr_pll_ref_clk_clock_sink_clk),
	.reset                	(~ddr_global_reset_reset_sink_reset_n),

	.avs_writedata        	(ddr_avmm_writedata),
	.avs_readdata         	(ddr_avmm_readdata),
	.avs_address          	(ddr_avmm_address),
	.avs_waitrequest      	(ddr_avmm_waitrequest),
	.avs_write            	(ddr_avmm_write && !ddr_avmm_waitrequest),
	.avs_read             	(ddr_avmm_read && !ddr_avmm_waitrequest),
	.avs_byteenable       	(ddr_avmm_byteenable),
	.avs_readdatavalid    	(ddr_avmm_readdatavalid),
	.avs_burstcount       	(ddr_avmm_burstcount)
);


function automatic Command get_command_from_slave();
	Command cmd;

	avs_bfm_inst.pop_command();
	cmd.burstcount          = avs_bfm_inst.get_command_burst_count();
	cmd.addr                = avs_bfm_inst.get_command_address();

	if (avs_bfm_inst.get_command_request() == REQ_WRITE) begin
		cmd.trans = WRITE;
		for(int i = 0; i < cmd.burstcount; i++) begin
			cmd.data[i]       =avs_bfm_inst.get_command_data(i);
			cmd.byteenable[i] =avs_bfm_inst.get_command_byte_enable(i);
		end
	end else begin
		cmd.trans = READ;
		for(int i = 0; i < cmd.burstcount; i++) begin
			cmd.byteenable[i] =avs_bfm_inst.get_command_byte_enable(i);
		end
	end

	return cmd;
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

function automatic Response memory_response (Command cmd);
	Response rsp;
	
	rsp.burstcount = (cmd.trans == READ) ? cmd.burstcount : 1;
	for(int idx = 0; idx < cmd.burstcount; idx++) begin
		if(cmd.trans == READ) begin
			rsp.data[idx] = memory[cmd.addr+idx] & get_mask(cmd.byteenable[0]);
			rsp.latency[idx] = $urandom_range(0,MAX_LATENCY); // set a random memory response latency
		end
	end
	return rsp;
endfunction

// Simple waitrequest emulation: assert waitrequest 80% of the time
always @(posedge avs_bfm_inst.clk) begin
	avs_bfm_inst.set_waitrequest(($urandom_range(0, 100) > 80));
end

always @(avs_bfm_inst.signal_command_received) begin
	Command     actual_cmd, exp_cmd;
	Response    rsp;
	logic [DDR_DATA_WIDTH-1:0] mask;

	actual_cmd = get_command_from_slave();

	// set read response
	if (actual_cmd.trans == READ) begin
		rsp = memory_response(actual_cmd);
		configure_and_push_response_to_slave(rsp);
		read_response_queue_slave.push_back(rsp);
	end

	if (actual_cmd.trans == WRITE) begin
		for(int idx = 0; idx < actual_cmd.burstcount; idx++) begin
			if(memory.exists(actual_cmd.addr+idx)) begin
				mask = get_mask(actual_cmd.byteenable[idx]);
				memory[actual_cmd.addr+idx] = ((memory[actual_cmd.addr+idx] & ~mask) |
					                       (actual_cmd.data[idx] & mask));
			end
			else begin
				memory[actual_cmd.addr+idx] = actual_cmd.data[idx];
			end
		end
	end
end

int pending_read_cycles_slave = 0;
always @(posedge avs_bfm_inst.clk) begin
	if (pending_read_cycles_slave > 0) begin
		pending_read_cycles_slave--;
	end
end

task automatic configure_and_push_response_to_slave(
	Response rsp
);
	int read_response_latency;

	avs_bfm_inst.set_response_request(REQ_READ);
	avs_bfm_inst.set_response_burst_size(rsp.burstcount);
	for (int i = 0; i < rsp.burstcount; i++) begin
		avs_bfm_inst.set_response_data(rsp.data[i], i);

		if (i == 0) begin
			avs_bfm_inst.set_response_latency(rsp.latency[i] + pending_read_cycles_slave, i);
			read_response_latency = rsp.latency[i];
		end else begin
			avs_bfm_inst.set_response_latency(rsp.latency[i], i);
			read_response_latency = rsp.latency[i] + read_response_latency;
		end

	end
	avs_bfm_inst.push_response();
	pending_read_cycles_slave = pending_read_cycles_slave + read_response_latency + rsp.burstcount + 2;
endtask

endmodule
