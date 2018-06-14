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

module avmm_ccip_host_rd (
	//clock/reset
	input clk,
	input reset,

	//avmm master
	output logic		avmm_waitrequest,
	output logic	[CCIP_AVMM_REQUESTOR_DATA_WIDTH-1:0]	avmm_readdata,
	output logic		avmm_readdatavalid,
	input 	[CCIP_AVMM_REQUESTOR_RD_ADDR_WIDTH-1:0]	avmm_address,
	input 		avmm_read,
	input 	[CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0]	avmm_burstcount,

	// ---------------------------IF signals between CCI and AFU  --------------------------------
	input c0TxAlmFull,
	//for read response
	input t_if_ccip_c0_Rx      c0rx,

	//read request
	output t_if_ccip_c0_Tx c0tx
);
	t_ccip_mdata rx_mdata;
	//read request
	t_if_ccip_c0_Tx c0tx_next;

	t_ccip_clLen burst_encoded;

	always @ (avmm_burstcount)
	begin
	  case (avmm_burstcount)
		3'b010:  burst_encoded = eCL_LEN_2;
		3'b100:  burst_encoded = eCL_LEN_4;
		default:  burst_encoded = eCL_LEN_1;
	  endcase
	end

	logic avcmd_ready;

	//read request, this block requires CCIP read re-ordering to be enabled in the MPF to ensure in-order read responses
	assign c0tx_next.hdr.vc_sel = eVC_VH0;
	assign c0tx_next.hdr.rsvd1 = '0;
	assign c0tx_next.hdr.cl_len = burst_encoded;
	assign c0tx_next.hdr.req_type = eREQ_RDLINE_I;
	assign c0tx_next.hdr.address = avmm_address[47:6];
	assign c0tx_next.hdr.mdata = rx_mdata;
	assign c0tx_next.valid = reset ? 1'b0 : avmm_read;

	wire avcmd_ready_next = ~c0TxAlmFull;
	assign avmm_waitrequest = ~avcmd_ready;

	always @(posedge clk) begin
		if (reset) begin // global reset
			//wait request
			avcmd_ready <= 1'b0;

			//mdata counter
			rx_mdata <= '0;
		end
		else begin
			//this can be registered because it is an almost full signal
			//will driver avmm wait request
			avcmd_ready <= avcmd_ready_next;

			if(avcmd_ready) begin
				//read request
				c0tx.valid <= c0tx_next.valid;

				//mdata counter
				if(c0tx_next.valid)
					rx_mdata <= rx_mdata + 1;
			end
			else begin
				//read request
				c0tx.valid <= 1'b0;

				//mdata counter
				rx_mdata <= rx_mdata;
			end
		end
	end

	always @(posedge clk) begin
		//read response
		avmm_readdata <= c0rx.data;
		avmm_readdatavalid <= reset ? 1'b0 : c0rx.rspValid &
			(c0rx.hdr.resp_type == eRSP_RDLINE);

		//read request
		c0tx.hdr <= c0tx_next.hdr;
	end

endmodule
