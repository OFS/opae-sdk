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

module avmm_ccip_host_wr #(
	parameter ENABLE_INTR = 0
	)

	(
	//clock/reset
	input clk,
	input reset,

	//interrupts
	input [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] irq,

	//avmm master
	output logic		avmm_waitrequest,
	input 	[CCIP_AVMM_REQUESTOR_DATA_WIDTH-1:0]	avmm_writedata,
	input 	[CCIP_AVMM_REQUESTOR_WR_ADDR_WIDTH-1:0]	avmm_address,
	input 		avmm_write,
	input 	[CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0]	avmm_burstcount,

	// ---------------------------IF signals between CCI and AFU  --------------------------------
	input c1TxAlmFull,
	//for write response.  don't need for now.  not using avmm write response
	//input t_if_ccip_c1_Rx      c1rx,

	//write request
	output t_if_ccip_c1_Tx c1tx
);
	t_ccip_mdata tx_mdata;
	//write request
	t_if_ccip_c1_Tx c1tx_next;

	t_ccip_clLen burst_encoded;

	always @ (avmm_burstcount)
	begin
	  case (avmm_burstcount)
		3'b010:  burst_encoded = eCL_LEN_2;
		3'b100:  burst_encoded = eCL_LEN_4;
		default:  burst_encoded = eCL_LEN_1;
	  endcase
	end


	reg [1:0] burst_counter;
	wire burst_counter_enable;
	wire load_burst_counter;
	wire write_sop;
	reg [1:0] address_counter;

	logic avcmd_ready;

	t_ccip_c1_ReqMemHdr ccip_intr_req_hdr;
	t_ccip_c1_ReqMemHdr ccip_mem_req_hdr;
	t_ccip_c1_ReqMemHdr ccip_fence_req_hdr;

	logic ccip_pending_irq;
	logic ccip_pending_irq_dly;
	logic [CCIP_AVMM_REQUESTOR_ID_BITS-1:0] ccip_pending_irq_id;

	logic ccip_write_fence_request;
	logic ccip_write_fence_complete;
	logic set_ccip_write_fence_complete;
	logic clear_ccip_write_fence_complete;
	logic ccip_write_fence_dly;

	/* Timing counter for signalling the write channel SOP.
	   The incoming bursts are values of 0, 1, or 3.  The steady
	   state of the counter is 0 and when 0 we load the counter when
	   a new write burst arrives. If a burst of 1 or 3 arrives then
	   for each beat coming out of the command channel decrements the
	   counter by 1.  Every time the burst counter is set to 0 the
	   SOP is asserted.  For back to back bursts the counter hits 0
	   exactly at the same time the next burst arrives, and if there is
	   a gap between bursts the counter still hits 0 and SOP will be
	   asserted but the valid will remain deasserted until the next
	   burst arrives. The two address LSBs must increment during a burst
	   so on a 2nd, 3rd, or 4th beat the address_counter[1:0] will be
	   used for the address LSB for write commands. */
	always @ (posedge clk or posedge reset)
	begin
	  if (reset == 1'b1)
	  begin
		burst_counter <= 2'b00;
		address_counter <= 2'b00;
	  end
	  else
	  begin
		if (load_burst_counter == 1'b1)
		begin
		  burst_counter <= burst_encoded;
		  address_counter <= avmm_address[7:6] + 1'b1;  // need to +1 because this counter is only used on beats 2-4
		end
		else if (burst_counter_enable == 1'b1)
		begin
		  burst_counter <= burst_counter - 1'b1;
		  address_counter <= address_counter + 1'b1;
		end
	  end
	end

	assign write_sop = (burst_counter == 2'b00);
	assign load_burst_counter = (burst_counter == 2'b00) & (avmm_write == 1'b1) & (avcmd_ready == 1'b1) & (ccip_write_fence_dly == 1'b0);
	assign burst_counter_enable = (burst_counter != 2'b00) & (avmm_write == 1'b1) & (avcmd_ready == 1'b1);

	//write request
	assign ccip_mem_req_hdr.rsvd2 = '0;
	assign ccip_mem_req_hdr.vc_sel = eVC_VH0;
	assign ccip_mem_req_hdr.sop = write_sop;
	assign ccip_mem_req_hdr.rsvd1 = '0;
	assign ccip_mem_req_hdr.cl_len = burst_encoded;
	assign ccip_mem_req_hdr.req_type = eREQ_WRLINE_I;
	assign ccip_mem_req_hdr.rsvd0 = '0;
	assign ccip_mem_req_hdr.address = {avmm_address[47:8], ((write_sop == 1'b1)? avmm_address[7:6] : address_counter)};
	assign ccip_mem_req_hdr.mdata = tx_mdata;
	assign c1tx_next.data = avmm_writedata;

	//intr request
	assign ccip_intr_req_hdr.rsvd2 = '0;
	assign ccip_intr_req_hdr.vc_sel = eVC_VH0;
	assign ccip_intr_req_hdr.sop = '0;
	assign ccip_intr_req_hdr.rsvd1 = '0;
	assign ccip_intr_req_hdr.cl_len = eCL_LEN_1;
	assign ccip_intr_req_hdr.req_type = eREQ_INTR;
	assign ccip_intr_req_hdr.rsvd0 = '0;
	assign ccip_intr_req_hdr.address = '0;
	assign ccip_intr_req_hdr.mdata[CCIP_MDATA_WIDTH-1:CCIP_AVMM_REQUESTOR_ID_BITS] = '0;
	assign ccip_intr_req_hdr.mdata[CCIP_AVMM_REQUESTOR_ID_BITS-1:0] = ccip_pending_irq_id;

	//write fence request
	assign ccip_fence_req_hdr.rsvd2 = '0;
	assign ccip_fence_req_hdr.vc_sel = eVC_VH0;
	assign ccip_fence_req_hdr.sop = '0;
	assign ccip_fence_req_hdr.rsvd1 = '0;
	assign ccip_fence_req_hdr.cl_len = eCL_LEN_1;
	assign ccip_fence_req_hdr.req_type = eREQ_WRFENCE;
	assign ccip_fence_req_hdr.rsvd0 = '0;
	assign ccip_fence_req_hdr.address = '0;
	assign ccip_fence_req_hdr.mdata = tx_mdata;


	//interupt/write fence/write request.  Priority is interrupts (highest) --> write fence --> read/write (lowest)
	assign c1tx_next.hdr = ccip_pending_irq_dly ? ccip_intr_req_hdr :
				ccip_write_fence_dly? ccip_fence_req_hdr : ccip_mem_req_hdr;

	//while there are any pending IRQs no new write fences, reads, or writes are allowed through
	wire avcmd_ready_next = ~c1TxAlmFull && ~ccip_pending_irq;
	// when a write fence arrives this logic must backpressure immediately so that data accompanying the write fence is not lost
	assign avmm_waitrequest = ~avcmd_ready | ccip_write_fence_dly;
	assign c1tx_next.valid = reset ? 1'b0 : avmm_write | ccip_pending_irq_dly | ccip_write_fence_dly;

	always @(posedge clk) begin
		if (reset) begin // global reset
			//wait request
			avcmd_ready <= 1'b0;

			//mdata counter
			tx_mdata <= '0;
		end
		else begin
			//this can be registered because it is an almost full signal
			//will driver avmm wait request
			avcmd_ready <= avcmd_ready_next;

			if(avcmd_ready | ccip_pending_irq_dly | ccip_write_fence_dly) begin
				//write request
				c1tx.valid <= c1tx_next.valid;

				//mdata counter
				if(c1tx_next.valid)
					tx_mdata <= tx_mdata + 1;
			end
			else begin
				//write request
				c1tx.valid <= 1'b0;

				//mdata counter
				tx_mdata <= tx_mdata;
			end
		end
	end

	always @(posedge clk) begin
		//write request
		c1tx.hdr <= c1tx_next.hdr;
		c1tx.data <= c1tx_next.data;
	end

	//interrupts
	//
	//this block captures queues up N number of interrupt lines and sends the
	//them one by one to the ccip c1tx request line
	//higher interupt lines are given priority
	//
	//handshaking between code above is done via signals
	//	ccip_pending_irq
	//  ccip_pending_irq_dly
	//  ccip_pending_irq_id
	//
	genvar i;
	generate if(ENABLE_INTR == 1) begin
		logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] prev_irq_state;
		logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] irq_pending;
		logic [CCIP_AVMM_REQUESTOR_ID_BITS-1:0] ccip_irq_id_sent;
		logic ccip_irq_was_sent;

		assign ccip_irq_id_sent = ccip_intr_req_hdr.mdata[CCIP_AVMM_REQUESTOR_ID_BITS-1:0];
		assign ccip_irq_was_sent = (c1tx_next.hdr.req_type == eREQ_INTR) && ccip_pending_irq_dly;

		logic ccip_pending_irq_next;
		logic [CCIP_AVMM_REQUESTOR_ID_BITS-1:0] ccip_pending_irq_id_next;
		always_comb begin
			ccip_pending_irq_next = '0;
			ccip_pending_irq_id_next = '0;
			for (integer j=0; j<CCIP_AVMM_NUM_INTERRUPT_LINES; j++) begin
				if(irq_pending[j] && ~ccip_irq_was_sent) begin
					ccip_pending_irq_next = 1'b1;
					ccip_pending_irq_id_next = j;
				end
			end
		end

		always @(posedge clk) begin
			if(reset) begin
				ccip_pending_irq <= 1'b0;
				ccip_pending_irq_dly <= 1'b0;
				ccip_pending_irq_id <= '0;
			end
			else begin
				ccip_pending_irq_dly <= ccip_pending_irq && ~c1TxAlmFull && ~ccip_irq_was_sent;
				ccip_pending_irq <= ccip_pending_irq_next;
				ccip_pending_irq_id <= ccip_pending_irq_id_next;
			end
		end

		for (i=0; i<CCIP_AVMM_NUM_INTERRUPT_LINES; i++) begin : intr_lines
			always @(posedge clk) begin
				if(reset) begin
					prev_irq_state[i] <= '0;
					irq_pending[i] <= '0;
				end
				else begin
					prev_irq_state[i] <= irq[i];
					//only set pending signal if there is a transition from low to high
					if(irq[i] && ~prev_irq_state[i] && ~irq_pending[i])
						irq_pending[i] <= 1'b1;

					if(ccip_irq_was_sent && ccip_irq_id_sent == i)
						irq_pending[i] <= 1'b0;
				end
			end
		end
	end
	else begin
		assign ccip_pending_irq = 1'b0;
		assign ccip_pending_irq_dly = 1'b0;
		assign ccip_pending_irq_id = '0;
	end
	endgenerate


	/* When a write occurs with Avalon address[48] set ccip_write_fence_request is asserted.
	The logic will issue a write fence followed by writing the incoming data within the host
	48-bit address space (i.e. address MSB ignored).  Reads with Avalon address[48] set will
	be treated as plain reads to the host space.  The only transaction that can take a higher
	priority than a write fence is an interrupt which the write fence logic will allow to be
	transferred to TX C1 first.  This logic does not wait for the fence response to return so
	the host use query for the data that is sent immediately after the fence to determine when
	all previous writes have arrived in host memory.
	*/
	always @ (posedge clk) begin
		if (reset) begin
			ccip_write_fence_complete <= 1'b0;
		end
		else if (set_ccip_write_fence_complete) begin
			ccip_write_fence_complete <= 1'b1;
		end
		else if (clear_ccip_write_fence_complete) begin
			ccip_write_fence_complete <= 1'b0;
		end
	end

	// a write to the write fence mirrored host address space has arrived
	assign ccip_write_fence_request = avmm_address[48] & avmm_write;
	// Need write fence to yield to IRQ traffic.  The write fence goes out even if
	// almost full is asserted, since there will always be space for it.
	assign set_ccip_write_fence_complete = ccip_write_fence_request && ~ccip_pending_irq_dly;
	// the write fence completes the cycle after the write fence command has been sent to the next queue so complete is 1 cycle
	assign clear_ccip_write_fence_complete = ccip_write_fence_complete && avcmd_ready_next;
	// once the write fence is sent we immediately let the data that arrived with it to be sent to host memory
	assign ccip_write_fence_dly = ccip_write_fence_request && ~ccip_write_fence_complete;

endmodule
