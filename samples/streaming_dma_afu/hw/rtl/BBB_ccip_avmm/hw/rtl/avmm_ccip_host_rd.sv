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
  
	logic avcmd_ready;
  
	t_ccip_clLen burst_encoded;

  /* Logic to handle Avalon bursts that are not supported by CCIP.  If you Avalon master issues bursts of 2 then it should ensure the
     address of each burst is a multiple of 2CL (128 bytes).  If your Avalon master issues bursts of 4 then it should ensure the address
     of each burst is a mulitple of 4CL (256 bytes).  If your doesn't follow these rules or issues bursts of 3 beats then they will be
     chopped into bursts of 1CL degrading the CCI-P performance.     
  */
	reg [1:0] burst_counter;
	wire burst_counter_enable;
	wire load_burst_counter;
  reg [41:0] address_counter;  // 64-byte addresses
  wire unaligned_burst;        // set when incoming burst is not aligned to CCI-P burst alignment rules
  reg additional_unaligned_beats;  // set for additional beats of an unaligned burst.  This can be asserted for 2 to 4 beats
  
	/* When burst_counter is set to 0 that represents the start of a burst.  Whether the burst is aligned or not 
     the address increments by 1 and the burst_counter decrements by 1 for each beat of the burst.  */
  always @ (posedge clk)
	begin
	  if (reset == 1'b1)
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
  assign unaligned_burst = ((burst_counter == 2'b00) & (avmm_read == 1'b1) & (avcmd_ready == 1'b1)) &
                           (((avmm_burstcount == 3'b100) & (avmm_address[7:6] != 2'b00)) |
                           ((avmm_burstcount == 3'b010) & (avmm_address[6] != 1'b0)) |
                           (avmm_burstcount == 3'b011));

                           
  /* When an incoming Avalon burst is not aligned to 2CL or 4CL (or is 3), additional_unaligned_beats will assert until the Avalon burst completes.
     This signal will be used to ensure we keep issuing 1CL bursts to the CCI-P interface.  After the last beat of an unaligned burst is 
     sent to CCI-P this signal will be reset and we'll repeat these steps again for the next burst if it's unaligned or has a length of 3 beats.
     Since a buffer transfer that starts unaligned will have all the bursts within the transfer unaligned it is recommend that the Avalon
     read master get back into 4CL alignment otherwise the link efficiency will drop due to all the bursts of 1CL posted to the CCI-P interface.
  */
  always @ (posedge clk)
  begin
    if (reset == 1'b1)
      additional_unaligned_beats <= 1'b0;
    else if (unaligned_burst == 1'b1)
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

  // only load the burst counter if the incoming Avalon burst is 2 or 4 and unaligned or if the Avalon burst is 3. 
  assign load_burst_counter = (unaligned_burst == 1'b1) & (avcmd_ready == 1'b1);
	assign burst_counter_enable = (burst_counter != 2'b00) & (avcmd_ready == 1'b1);
  

	//read request, this block requires CCIP read re-ordering to be enabled in the MPF to ensure in-order read responses
	assign c0tx_next.hdr.vc_sel = eVC_VH0;
	assign c0tx_next.hdr.rsvd1 = '0;
	assign c0tx_next.hdr.cl_len = burst_encoded;
	assign c0tx_next.hdr.req_type = eREQ_RDLINE_I;
  assign c0tx_next.hdr.rsvd0 = '0;
	assign c0tx_next.hdr.address = (burst_counter == 2'b00)? avmm_address[47:6] : address_counter;
	assign c0tx_next.hdr.mdata = rx_mdata;
	assign c0tx_next.valid = reset ? 1'b0 : (avmm_read | additional_unaligned_beats);

	wire avcmd_ready_next = ~c0TxAlmFull;
  // need to throttle when dealing with an unaligned burst or an Avalon burst of 3 beats
	assign avmm_waitrequest = ~avcmd_ready | additional_unaligned_beats;

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
					rx_mdata <= rx_mdata + 1'b1;
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
