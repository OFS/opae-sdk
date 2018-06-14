// ***************************************************************************
// Copyright (c) 2017, Intel Corporation
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither the name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// ***************************************************************************

import ccip_if_pkg::*;
import ccip_avmm_pkg::*;

module ccip_avmm_mmio #(
	parameter MMIO_BYPASS_ADDRESS = 0,
	parameter MMIO_BYPASS_SIZE = 0
	)
	
	(
	input clk,
	input	reset,

	input		avmm_waitrequest,
	input	[CCIP_AVMM_MMIO_DATA_WIDTH-1:0]	avmm_readdata,
	input		avmm_readdatavalid,
	output logic 	[CCIP_AVMM_MMIO_DATA_WIDTH-1:0]	avmm_writedata,
	output logic 	[CCIP_AVMM_MMIO_ADDR_WIDTH-1:0]	avmm_address,
	output logic 		avmm_write,
	output logic 		avmm_read,
	output logic 	[(CCIP_AVMM_MMIO_DATA_WIDTH/8)-1:0]	avmm_byteenable,
	
	// ---------------------------IF signals between CCI and AFU  --------------------------------
	//input	t_if_ccip_Rx    cp2af_sRxPort,
	input t_if_ccip_c0_Rx c0rx,
	//output	t_if_ccip_Tx	af2cp_sTxPort
	output t_if_ccip_c2_Tx c2tx
);
	localparam TID_FIFO_WIDTH = CCIP_TID_WIDTH+1; //+1 bit to indicate 32/64 bit request


	//
	logic [CCIP_AVMM_MMIO_DATA_WIDTH-1:0] mmio_rsp_data;
	logic mmio_rsp_valid;
	logic mmio_rsp_ready;
	
	t_ccip_avmm_mmio_cmd mmio_cmd_data;
	logic mmio_cmd_valid;
	logic mmio_cmd_ready;
	
	// cast c0 header into ReqMmioHdr
	t_ccip_c0_ReqMmioHdr mmioHdr;
	assign mmioHdr = t_ccip_c0_ReqMmioHdr'(c0rx.hdr);
	wire mmio32_req = (mmioHdr.length == 2'b00);
	wire mmio32_highword_req = mmio32_req & mmioHdr.address[0];
	
	logic tid_fifo_wrreq;
	reg tid_fifo_rdreq;
	logic [TID_FIFO_WIDTH-1:0] tid_fifo_input;
	wire [TID_FIFO_WIDTH-1:0] tid_fifo_output;
	reg [63:0] rd_rsp_data_reg;
	reg [63:0] rd_rsp_data_reg2;
	reg rd_rsp_valid_reg;
	
	wire fifo_mmio32_highword_req = tid_fifo_output[TID_FIFO_WIDTH-1];
	
	//need to avoid region that MPF responds to
	wire mmio_address_valid = (MMIO_BYPASS_SIZE == 0) ? 1'b1 : 
		!(mmioHdr.address >= (MMIO_BYPASS_ADDRESS/4) && mmioHdr.address < ((MMIO_BYPASS_ADDRESS/4)+MMIO_BYPASS_SIZE/4));
	
	scfifo  tid_fifo_inst (
		.data(tid_fifo_input),
		.q(tid_fifo_output),
		.sclr(reset),
		.clock(clk),
		.wrreq(tid_fifo_wrreq),
		.rdreq(tid_fifo_rdreq),
		.aclr (),
		.almost_empty (),
		.almost_full (),
		.eccstatus (),
		.empty (),
		.full (),
		.usedw ()
	);
	defparam
		tid_fifo_inst.add_ram_output_register  = "ON",
		tid_fifo_inst.enable_ecc  = "FALSE",
		tid_fifo_inst.intended_device_family  = "Arria 10",
		tid_fifo_inst.lpm_numwords  = 64,
		tid_fifo_inst.lpm_showahead  = "OFF",
		tid_fifo_inst.lpm_type  = "scfifo",
		tid_fifo_inst.lpm_width  = TID_FIFO_WIDTH,
		tid_fifo_inst.lpm_widthu  = 6,
		tid_fifo_inst.overflow_checking  = "ON",
		tid_fifo_inst.underflow_checking  = "ON",
		tid_fifo_inst.use_eab  = "ON";

	always_ff @(posedge clk)
	begin
		mmio_rsp_ready <= reset ? 1'b0 : 1'b1;
		
		//MMIO request (read or write)
		mmio_cmd_valid <= reset ? 1'b0 : (c0rx.mmioRdValid || c0rx.mmioWrValid) && mmio_address_valid;
		mmio_cmd_data.addr <= {mmioHdr.address, 2'b00};
		mmio_cmd_data.is_32bit <= mmio32_req;
		mmio_cmd_data.is_read <= c0rx.mmioRdValid && mmio_address_valid;

		//MMIO write request
		mmio_cmd_data.write_data[31:0] <= c0rx.data[31:0];
		mmio_cmd_data.write_data[63:32] <= mmio32_highword_req ? c0rx.data[31:0] : c0rx.data[63:32];
		
		//MMIO read requests
		tid_fifo_wrreq <= reset ? 1'b0 : (c0rx.mmioRdValid && mmio_address_valid);
		tid_fifo_input <= {mmio32_highword_req, mmioHdr.tid}; // copy TID

		//MMIO read response
		tid_fifo_rdreq <= reset ? 1'b0 : mmio_rsp_valid;
		rd_rsp_valid_reg <= reset ? 1'b0 : tid_fifo_rdreq;
		rd_rsp_data_reg <= mmio_rsp_data;
		rd_rsp_data_reg2 <= rd_rsp_data_reg;
		c2tx.mmioRdValid <= reset ? 1'b0 : rd_rsp_valid_reg; // post response
		c2tx.hdr.tid <= tid_fifo_output[CCIP_TID_WIDTH-1:0];
		c2tx.data[31:0] <= fifo_mmio32_highword_req ? rd_rsp_data_reg2[63:32] : rd_rsp_data_reg2[31:0];
		c2tx.data[63:32] <= rd_rsp_data_reg2[63:32];
	end
	
	//handle avmm signals
	assign avmm_byteenable = mmio_cmd_data.is_32bit ? 
		(mmio_cmd_data.addr[2] ? 8'b11110000 : 8'b00001111) : 8'b11111111;

	//read/write request
	assign avmm_address = mmio_cmd_data.addr;
	assign avmm_writedata = mmio_cmd_data.write_data;

	assign mmio_cmd_ready = !reset & !avmm_waitrequest;	
	wire avmm_ready = !reset & (!avmm_waitrequest && mmio_cmd_valid);
	assign avmm_write = avmm_ready & !mmio_cmd_data.is_read;
	assign avmm_read = avmm_ready & mmio_cmd_data.is_read;

	//handle read response
	assign mmio_rsp_data = avmm_readdata;
	assign mmio_rsp_valid = reset ? 1'b0 : avmm_readdatavalid;

endmodule
