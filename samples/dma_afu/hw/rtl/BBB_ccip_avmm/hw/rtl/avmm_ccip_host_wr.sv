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
	parameter ENABLE_INTR = 0,
  parameter AVMM_BURST_FIFO_MLAB_ENABLE = 1  // 1 to force burst FIFO to be synthesized in MLABs
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
	output logic [1:0] avmm_write_response,
	output logic avmm_write_responsevalid,

	// ---------------------------IF signals between CCI and AFU  --------------------------------
	input c1TxAlmFull,
	input t_if_ccip_c1_Rx c1rx,

	//write request
	output t_if_ccip_c1_Tx c1tx
);
	t_ccip_mdata tx_mdata;
	//write request
	t_if_ccip_c1_Tx c1tx_next;

	t_ccip_clLen burst_encoded;

  
  /* Logic to primarily handle Avalon bursts that are not supported by CCIP.  If you Avalon master issues bursts of 2 then it should ensure the
     address of each burst is a multiple of 2CL (128 bytes).  If your Avalon master issues bursts of 4 then it should ensure the address
     of each burst is a mulitple of 4CL (256 bytes).  If your doesn't follow these rules or issues bursts of 3 beats then they will be
     chopped into bursts of 1CL degrading the CCI-P performance.     
  */
	reg [1:0] burst_counter;
	wire burst_counter_enable;
	wire load_burst_counter;
	wire write_sop;
  reg [41:0] address_counter;  // 64-byte addresses
  wire unaligned_burst;        // set when incoming burst is not aligned to CCI-P burst alignment rules
  reg additional_unaligned_beats;  // set for additional beats of an unaligned burst.  This can be asserted for 2 to 4 beats

	logic avcmd_ready;

	t_ccip_c1_ReqMemHdr ccip_intr_req_hdr;
	t_ccip_c1_ReqMemHdr ccip_mem_req_hdr;
	t_ccip_c1_ReqMemHdr ccip_fence_req_hdr;

  logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] irq_d;
  logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] irq_pending;
  logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] set_irq_pending;
  logic [CCIP_AVMM_NUM_INTERRUPT_LINES-1:0] clear_irq_pending;
  logic [CCIP_AVMM_REQUESTOR_ID_BITS-1:0] ccip_pending_irq_id;
  logic ccip_pending_irq_dly;

	logic ccip_write_fence_request;
	logic ccip_write_fence_complete;
	logic set_ccip_write_fence_complete;
	logic clear_ccip_write_fence_complete;
	logic ccip_write_fence_dly;

  // logic to keep track of outstanding Avalon write bursts and coalesce CCIP responses into Avalon write responses
  logic [CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0] burst_fifo_input;
  logic [CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0] burst_fifo_output;
  logic burst_fifo_wrreq;
  logic burst_fifo_rdreq;
  logic burst_fifo_empty;
  logic burst_fifo_full;
  logic [8:0] response_counter;  // number of 1CL sizes responses in flight (worst case is 64 bursts of 4CL in flight)
  logic increment_response_counter;  // increments based on cl_num + 1 of the responses returning
  logic decrement_response_counter;  // decrements based on the Avalon burst size in first-in-first-out order
  logic decrement_response_counter_d1;  // pipelined version will be used for the Avalon writeresponsevalid signal
  

	/* When burst_counter is set to 0 that represents the start of a burst.  Whether the burst is aligned or not 
     the address increments by 1 and the burst_counter decrements by 1 for each beat of the burst.  */
	always @ (posedge clk)
	begin
	  if (reset)
	  begin
      burst_counter <= 2'b00;
      address_counter <= 2'b00;
	  end
	  else if (load_burst_counter == 1'b1)
		begin
		  burst_counter <= 2'b11 & (avmm_burstcount - 1'b1);  // maximum Avalon burst length is 4 but after subtracting 1 it's a 2-bit value
		  address_counter <= avmm_address[47:6] + 1'b1;  // need to +1 because this counter value is only used on beats 2-4
		end
		else if (burst_counter_enable == 1'b1)
		begin
		  burst_counter <= burst_counter - 1'b1;
		  address_counter <= address_counter + 1'b1;
		end
	end
  
  /*  unaligned_burst is used to determine if the incoming Avalon burst is not compatible with the CCI-P spec.  When this signal asserts
      at the start of an Avalon burst it will it is not garanteed that the Avalon burst signal will be held constant so if the incoming burst
      was 2, 3, or 4 then the signal additional_unaligned_beats will be used to capture the state of unaligned_burst for the rest of the beats.
  
      unaligned_burst asserts in either of the three cases (in this order in the RTL below):
      1)  Incoming Avalon burst is 4 but the address is not divisable by 256 (4CL)   -or-
      2)  Incoming Avalon burst is 2 but the address is not divisable by 128 (2CL)   -or-
      3)  Incoming Avalon burst is 3 which is not supported by CCI-P at all
      
      In all three cases the burst will get chopped into single 1CL bursts when issued to CCI-P.  
  */
  assign unaligned_burst = ((burst_counter == 2'b00) & (avmm_write == 1'b1) & (avcmd_ready == 1'b1) & (ccip_write_fence_dly == 1'b0)) &
                           (((avmm_burstcount == 3'b100) & (avmm_address[7:6] != 2'b00)) |
                           ((avmm_burstcount == 3'b010) & (avmm_address[6] != 1'b0)) |
                           (avmm_burstcount == 3'b011));
  
  /* When an incoming Avalon burst is not aligned to 2CL or 4CL (or is 3), additional_unaligned_beats will assert until the Avalon burst completes.
     This signal will be used to ensure we keep issuing 1CL bursts to the CCI-P interface while asserting sop.  After the last beat
     of an unaligned burst is sent to CCI-P this signal will be reset and we'll repeat these steps again for the next burst if it's unaligned or has a length of 3 beats.
     Since a buffer transfer that starts unaligned will have all the bursts within the transfer unaligned it is recommend that the Avalon
     write master get back into 4CL alignment otherwise the link efficiency will drop due to all the bursts of 1CL posted to the CCI-P interface.
  */
  always @ (posedge clk)
  begin
    if (reset)
      additional_unaligned_beats <= 1'b0;
    else if (unaligned_burst == 1'b1)  // unaligned bursts are always at least 2 beats
      additional_unaligned_beats <= 1'b1;
    else if ((additional_unaligned_beats == 1'b1) & (burst_counter == 2'b01) & (avcmd_ready == 1'b1))
      additional_unaligned_beats <= 1'b0;
  end
  
  /* If an Avalon burst of 1, 2, or 4 arrives and the address is aligned to 1CL, 2CL or 4CL then encode it and pass it to the CCI-P interface directly.
     Avalon bursts of 2 or 4 that are not aligned, or an Avalon burst of 3 is broken up into 1CL bursts to CCI-P.  
  */
  always @ (avmm_burstcount or unaligned_burst or additional_unaligned_beats)
	begin
	  case ( {(unaligned_burst | additional_unaligned_beats), avmm_burstcount})
      4'b0010:  burst_encoded = eCL_LEN_2;
      4'b0100:  burst_encoded = eCL_LEN_4;
      default:  burst_encoded = eCL_LEN_1;
	  endcase
	end
  
  // In the case of an unaligned burst we need to keep write_sop asserted because we will be issuing 1 beat bursts
	assign write_sop = (burst_counter == 2'b00) | (additional_unaligned_beats == 1'b1);
	assign load_burst_counter = (burst_counter == 2'b00) & (avmm_write == 1'b1) & (avcmd_ready == 1'b1) & (burst_fifo_full == 1'b0) &
                              (ccip_pending_irq_dly == 1'b0) & (ccip_write_fence_dly == 1'b0);
	assign burst_counter_enable = (burst_counter != 2'b00) & (avmm_write == 1'b1) & (avmm_waitrequest == 1'b0);

	//write request
	assign ccip_mem_req_hdr.rsvd2 = '0;
	assign ccip_mem_req_hdr.vc_sel = eVC_VH0;
	assign ccip_mem_req_hdr.sop = write_sop;
	assign ccip_mem_req_hdr.rsvd1 = '0;
	assign ccip_mem_req_hdr.cl_len = burst_encoded;
	assign ccip_mem_req_hdr.req_type = eREQ_WRLINE_I;
	assign ccip_mem_req_hdr.rsvd0 = '0;
  assign ccip_mem_req_hdr.address = (burst_counter == 2'b00)? avmm_address[47:6] : address_counter;
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


	//interupt/write fence/write request.  Priority is interrupts (highest) --> write fence --> write (lowest)
	assign c1tx_next.hdr = ccip_pending_irq_dly? ccip_intr_req_hdr :
				                 ccip_write_fence_dly? ccip_fence_req_hdr : ccip_mem_req_hdr;

  wire avcmd_ready_next = ~c1TxAlmFull;
	/* When a write fence arrives this logic must backpressure immediately so that data accompanying the write fence is not lost.
     Since only a finite number of write bursts can be tracked need to backpressure when the burst FIFO is full */
	assign avmm_waitrequest = ~avcmd_ready | ccip_pending_irq_dly | ccip_write_fence_dly | burst_fifo_full;
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
					tx_mdata <= tx_mdata + 1'b1;
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



/***********************************************************************************************/
/*                                    Interrupt Logic                                          */
/***********************************************************************************************/

  // copy of the incoming IRQ to use for edge detection
  always @ (posedge clk)
  begin
    if (reset)
      irq_d <= {CCIP_AVMM_NUM_INTERRUPT_LINES{1'b0}};
    else
      irq_d <= irq;    
  end
  
genvar j;   
generate
  for (j=0; j<CCIP_AVMM_NUM_INTERRUPT_LINES; j++) 
  begin 
  
    always @ (posedge clk)
    begin
      if (reset)
        irq_pending[j] <= 1'b0;
      else
      begin
        case ({set_irq_pending[j], clear_irq_pending[j]})
          2'b01:  irq_pending[j] <= 1'b0;
          2'b10:  irq_pending[j] <= 1'b1;
          2'b11:  irq_pending[j] <= 1'b1;  // this would only happen if the input IRQ pulses at the same time command reg slice accepts the IRQ
          default:  irq_pending[j] <= irq_pending[j];
        endcase
      end
    end
    
    // rising edge detect always sets the appropriate irq_pending bit
    assign set_irq_pending[j] = (irq[j] == 1'b1) & (irq_d[j] == 1'b0);
    // clear pending register when command reg slice accepts IRQ
    assign clear_irq_pending[j] = (c1tx_next.hdr.req_type == eREQ_INTR) & (ccip_pending_irq_id == j) & ccip_pending_irq_dly;  
    
  end
endgenerate



  // lowest order bit in the IRQ bus has highest priority
  assign ccip_pending_irq_id = (irq_pending[0] == 1'b1)? 2'b00 :
                               (irq_pending[1] == 1'b1)? 2'b01 :
                               (irq_pending[2] == 1'b1)? 2'b10 : 2'b11;
  
  // need to make sure we don't attempt to start an interrupt in the middle of a multiline burst
  assign ccip_pending_irq_dly = (burst_counter == 2'b00) & (c1TxAlmFull == 1'b0) & (irq_pending[0] | irq_pending[1] | irq_pending[2] |irq_pending[3]);

/***********************************************************************************************/
/*                                End of Interrupt Logic                                       */
/***********************************************************************************************/






/***********************************************************************************************/
/*                                   Write Fence Logic                                         */
/***********************************************************************************************/
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
  assign clear_ccip_write_fence_complete = ccip_write_fence_complete && avcmd_ready;
	// once the write fence is sent we immediately let the data that arrived with it to be sent to host memory
	assign ccip_write_fence_dly = ccip_write_fence_request && ~ccip_write_fence_complete;

/***********************************************************************************************/
/*                              End of Write Fence Logic                                       */
/***********************************************************************************************/





/***********************************************************************************************/
/*                                  Write Response Logic                                       */
/***********************************************************************************************/

  // need to make sure once this FIFO is full the slave port backpressures
  scfifo avmm_burst_fifo_inst (
		.data(burst_fifo_input),
		.q(burst_fifo_output),
		.sclr(reset),
		.clock(clk),
		.wrreq(burst_fifo_wrreq),
		.rdreq(burst_fifo_rdreq),
		.aclr (),
		.almost_empty (),
		.almost_full (),
		.eccstatus (),
		.empty (burst_fifo_empty),
		.full (burst_fifo_full),
		.usedw ()
	);
	defparam
      avmm_burst_fifo_inst.add_ram_output_register  = "ON",
      avmm_burst_fifo_inst.enable_ecc  = "FALSE",
      avmm_burst_fifo_inst.lpm_numwords  = 64,
      avmm_burst_fifo_inst.lpm_showahead  = "ON",
      avmm_burst_fifo_inst.lpm_type  = "scfifo",
      avmm_burst_fifo_inst.lpm_width  = CCIP_AVMM_REQUESTOR_BURST_WIDTH,
      avmm_burst_fifo_inst.lpm_widthu  = 6,  // can only track 
      avmm_burst_fifo_inst.overflow_checking  = "OFF",
      avmm_burst_fifo_inst.underflow_checking  = "OFF",
      avmm_burst_fifo_inst.ram_block_type = (AVMM_BURST_FIFO_MLAB_ENABLE)? "MLAB" : "AUTO",
      avmm_burst_fifo_inst.use_eab  = "ON";

  assign burst_fifo_input = avmm_burstcount;
  assign burst_fifo_wrreq = load_burst_counter;
  assign burst_fifo_rdreq = decrement_response_counter;


  // this counter assumes that write responses are always packed by the MPF
  always @ (posedge clk)
  begin
    if (reset)
    begin
      response_counter <= 9'h000;
    end
    else
    begin
      case ({increment_response_counter, decrement_response_counter})
        2'b01:  response_counter <= response_counter - burst_fifo_output;
        2'b10:  response_counter <= response_counter + c1rx.hdr.cl_num + 1'b1;  // responses are 0, 1, and 3 so need to add 1 for Avalon format
        2'b11:  response_counter <= response_counter + c1rx.hdr.cl_num + 1'b1 - burst_fifo_output;  // counting up and down
        default:  response_counter <= response_counter;  // no new write bursts or responses
      endcase
    end  
  end

  /* Increment response counter any time a write response returns.  Since the MPF is in front of this logic
     responses are always garanteed to be packed.  Unpacked responses are not supported but would be trivial
     to support.
    
     Decrement response counter any time enough 1CL response beats have been accumulated that we can respond to
     the incoming write bursts in the order that they arrived.  Decrementing the counter also pops the burst FIFO.  */
  assign increment_response_counter = c1rx.rspValid & (c1rx.hdr.resp_type == eRSP_WRLINE);
  assign decrement_response_counter = (burst_fifo_empty == 1'b0) & (response_counter >= burst_fifo_output);
  
  
  always @ (posedge clk)
  begin
    if (reset)
      decrement_response_counter_d1 <= 1'b0;
    else
      decrement_response_counter_d1 <= decrement_response_counter;
  end
  
  // each Avalon burst must be responded to once.  After all writes have been responded to, response_counter should be 0
  assign avmm_write_response = 2'b00;  // Avalon response 'OK'
  assign avmm_write_responsevalid = decrement_response_counter_d1;
  
/***********************************************************************************************/
/*                               End of Write Response Logic                                   */
/***********************************************************************************************/
  
endmodule
