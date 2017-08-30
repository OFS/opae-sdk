/* ****************************************************************************
 * Copyright(c) 2011-2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * **************************************************************************
 *
 * Module Info: In-order transaction channel
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * *********************************************************************************
 * SR 4.1.x - SR 5.0.0-prealpha implementation
 * ---------------------------------------------------------------------------------
 * - Transactions are stored when request comes from AFU
 *   - This is a normal DPI-C call to C functions
 * - When a response is received, the response is queued in normal format
 *
 * THIS COMPONENT
 * - simply re-orders requests and sends them out
 * - May not necessarily be synthesizable
 *
 * OPERATION:
 * - {meta_in, data_in} is validated with write_en signal
 *   - An empty slot is found, a random delay is computed based on pre-known parameters
 *   - The state machine is kicked off.
 *
 * GENERICS:
 * - NUM_WAIT_STATIONS : Number of transactions in latency buffer
 * - FIFO_FULL_THRESH : FIFO full threshold
 * - FIFO_DEPTH_BASE2 : FIFO depth radix
 *
 */

import ase_pkg::*;

`include "platform.vh"

module inorder_wrf_channel
  #(
    parameter string DEBUG_LOGNAME = "channel.log",
    parameter int    NUM_WAIT_STATIONS = 8,
    parameter int    NUM_STATIONS_FULL_THRESH = 3,
    parameter int    COUNT_WIDTH = 8,
    parameter int    VISIBLE_DEPTH_BASE2 = 8,
    parameter int    VISIBLE_FULL_THRESH = 220,
    parameter int    LATBUF_MAX_TXN = 4,
    parameter int    WRITE_CHANNEL = 0
    )
   (
    input logic 		       clk,
    input logic 		       rst,
    input logic 		       finish_trigger,
    // Transaction in
    input 			       TxHdr_t hdr_in,
    input logic [CCIP_DATA_WIDTH-1:0]  data_in,
    input logic 		       write_en,
    // Transaction out
    output 			       TxHdr_t txhdr_out,
    output 			       RxHdr_t rxhdr_out,
    output logic [CCIP_DATA_WIDTH-1:0] data_out,
    output logic 		       valid_out,
    input logic 		       read_en,
    // Status signals
    output logic 		       empty,
    output logic 		       almfull,
    output logic 		       full,
    output logic 		       overflow_error,
    // Status inputs to hazard detector logic (dummy ports, never used --- since everything is in-order)
    output 			       ase_haz_pkt hazpkt_in,
    output 			       ase_haz_pkt hazpkt_out
    );

   // Logger function
`ifdef ASE_DEBUG
   // Internal logger fd
   int 				       log_fd;
   initial begin
      log_fd = $fopen( DEBUG_LOGNAME, "w");
      $fwrite(log_fd, "Logger for %m transactions\n");
   end
`endif


   // Visible depth
   localparam VISIBLE_DEPTH = 2**VISIBLE_DEPTH_BASE2;

   // Internal signals
   logic [LATBUF_TID_WIDTH-1:0] 	  tid_in;
   logic [LATBUF_TID_WIDTH-1:0] 	  tid_out;

   logic 				  outfifo_almfull;

   // FIFO widths
   localparam INFIFO_WIDTH   = LATBUF_TID_WIDTH + CCIP_TX_HDR_WIDTH + CCIP_DATA_WIDTH;
   localparam OUTFIFO_WIDTH  = LATBUF_TID_WIDTH + CCIP_RX_HDR_WIDTH + CCIP_TX_HDR_WIDTH + CCIP_DATA_WIDTH;

   // Internal channel management
   logic [INFIFO_WIDTH-1:0] infifo[$:VISIBLE_DEPTH-1];
   logic [OUTFIFO_WIDTH-1:0] outfifo[$:VISIBLE_DEPTH-1];

   // FIFO counts
   int 								      infifo_cnt;
   int 								      outfifo_cnt;

   always @(posedge clk) begin : cnt_proc
      infifo_cnt      <= infifo.size();
      outfifo_cnt     <= outfifo.size();
   end

   /*
    * Tracking ID generator
    */
   always @(posedge clk) begin : tid_proc
      if (rst)
	tid_in	<= {LATBUF_TID_WIDTH{1'b0}};
      else if (write_en)
	tid_in	<= tid_in + 1;
   end

   // Full signal
   always @(posedge clk) begin : full_proc
      if (rst) begin
	 full <= 0;
      end
      else if (infifo_cnt == VISIBLE_DEPTH-1) begin
	 full <= 1;
      end
      else begin
	 full <= 0;
      end
   end

   // Overflow check
   always @(posedge clk) begin
      if (rst) begin
	 overflow_error <= 0;
      end
      else if ((infifo_cnt == VISIBLE_DEPTH-1) && write_en) begin
	 overflow_error <= 1;
`ifdef ASE_DEBUG
	 $fwrite(log_fd, "%d | ** Overflow Error detected **\n", $time);
`endif
      end
   end


   /*
    * Infifo, request staging
    */
   always @(posedge clk) begin : infifo_push
      if (write_en) begin
`ifdef ASE_DEBUG
	 $fwrite(log_fd, "%d | ENTER : %s assigned tid=%x\n", $time, return_txhdr(hdr_in), tid_in);
	 if (hdr_in.reqtype == ASE_WRFENCE) begin
	    $fwrite (log_fd, "%d | WrFence inserted in channel\n", $time);
	 end
`endif
	 infifo.push_back({ tid_in, data_in, logic_cast_TxHdr_t'(hdr_in) });
      end
   end


   // Almfull signal
   always @(posedge clk) begin : almfull_proc
      if (rst) begin
	 almfull <= 1;
      end
      else if (infifo_cnt > VISIBLE_FULL_THRESH ) begin
	 almfull <= 1;
      end
      else begin
	 almfull <= 0;
      end
   end

   // Almfull tracking
   logic almfull_q;
   always @(posedge clk) begin
      almfull_q <= almfull;
   end

   // If Full toggles, log the event
`ifdef ASE_DEBUG
   always @(posedge clk) begin
      if (almfull_q != almfull) begin
	 $fwrite(log_fd, "%d | Module full toggled from %b to %b\n", $time, almfull_q, almfull);
      end
   end
`endif


   /*
    * Processing logic
    */
   logic [CCIP_DATA_WIDTH-1:0]   infifo_data_out;
   logic [LATBUF_TID_WIDTH-1:0]  infifo_tid_out;
   logic [CCIP_TX_HDR_WIDTH-1:0] infifo_hdr_out_vec;
   TxHdr_t                       infifo_hdr_out;
   logic 			 infifo_vld_out;

   ccip_vc_t 		         vc_arb;
   logic [1:0] 			 curr_vc_index = 2'b0;
   ccip_vc_t [0:3] sel_vc_array = {VC_VL0, VC_VH0, VC_VL0, VC_VH1};

   logic 			 outfifo_empty;
   logic [CCIP_TX_HDR_WIDTH-1:0] txhdr_out_vec;
   logic [CCIP_RX_HDR_WIDTH-1:0] rxhdr_out_vec;
   ccip_vc_t                  last_vc;
      ccip_len_t                 base_len;
      ccip_vc_t                  base_vc;

   // Function: infifo_to_outfifo
   // function automatic void infifo_to_outfifo(int init);
   task automatic infifo_to_outfifo(int init);
      // int 			 mcl_txn_iter;
      RxHdr_t                    infifo_rxhdr;
      logic [41:0] 		 base_addr;
      begin
	 if (init) begin
	    vc_arb = ccip_vc_t'(VC_VL0);
	    curr_vc_index = 2'b00;
	 end
	 else if ((infifo.size() != 0) && ~outfifo_almfull)  begin
	    // ----- Read from infifo ---- //
	    {infifo_tid_out, infifo_data_out, infifo_hdr_out_vec} = infifo.pop_front();
	    infifo_hdr_out = TxHdr_t'(infifo_hdr_out_vec);
	    // ----- If VA is set, select a channel ----- //
	    if ((infifo_hdr_out.vc == VC_VA) && (isReadRequest(infifo_hdr_out) || (isWriteRequest(infifo_hdr_out) && infifo_hdr_out.sop))) begin
   	       infifo_hdr_out.vc = sel_vc_array[curr_vc_index];
	       curr_vc_index = curr_vc_index + 1;
	       last_vc = infifo_hdr_out.vc;
	    end
	    else if (isWriteRequest(infifo_hdr_out) && ~infifo_hdr_out.sop) begin
	       infifo_hdr_out.vc = last_vc;	       
	    end
	    // ---- Address capture ---- //
	    base_addr = infifo_hdr_out.addr;
	    // ---- MCL packet management --- //
	    if (isWriteRequest(infifo_hdr_out)) begin
	       if (infifo_hdr_out.sop) begin
		  base_vc = infifo_hdr_out.vc;
		  base_len = infifo_hdr_out.len;		  
	       end
	       else begin
		  infifo_hdr_out.vc = base_vc;
		  infifo_hdr_out.len = base_len;		  
	       end
	    end
	    /////////////// Prepare RxHdr ////////////////////
	    infifo_rxhdr         = RxHdr_t'(0);
	    infifo_rxhdr.vc_used = infifo_hdr_out.vc;
	    infifo_rxhdr.hitmiss = 0;
	    infifo_rxhdr.mdata   = infifo_hdr_out.mdata;
	    if (isReadRequest(infifo_hdr_out)) begin
	       infifo_rxhdr.resptype = ASE_RD_RSP;
	    end
	    else if (isWriteRequest(infifo_hdr_out)) begin
	       infifo_rxhdr.resptype = ASE_WR_RSP;
	    end
	    else if (isWrFenceRequest(infifo_hdr_out)) begin
	       infifo_rxhdr.resptype = ASE_WRFENCE_RSP;
	    end
`ifdef ASE_ENABLE_INTR_FEATURE
	    else if (isIntrRequest(infifo_hdr_out)) begin
	       infifo_rxhdr.resptype = ASE_INTR_RSP;
	    end
`endif
	    if (isWriteRequest(infifo_hdr_out)) begin
	       if (isVHxRequest(infifo_hdr_out)) begin
		  infifo_rxhdr.format = 1;
		  infifo_rxhdr.clnum  = base_len;
		  // mcl_txn_iter = mcl_txn_iter + 1;
	       end
	       else begin
		  infifo_rxhdr.format = 0;
		  infifo_rxhdr.clnum  = infifo_hdr_out.len;
	       end
	    end
`ifdef ASE_DEBUG
	    $fwrite(log_fd, "%d | tid=%x assigned to channel %s, %s %s\n", $time, infifo_tid_out, ase_channel_type(infifo_hdr_out.vc), return_txhdr(infifo_hdr_out), return_rxhdr(infifo_rxhdr));
`endif
	    ///////////// Packing & CLNUM control ///////////
	    if (isReadRequest(infifo_hdr_out)) begin
	       infifo_rxhdr.format = 0;
	       for (int cl_i = 0; cl_i <= int'(infifo_hdr_out.len); cl_i = cl_i + 1) begin
		  infifo_rxhdr.clnum = ccip_len_t'(cl_i);
		  infifo_hdr_out.addr = base_addr + cl_i[1:0];
		  outfifo.push_back({infifo_tid_out, infifo_data_out, logic_cast_RxHdr_t'(infifo_rxhdr), logic_cast_TxHdr_t'(infifo_hdr_out)} );
	       end
	    end
	    else begin
	       outfifo.push_back({infifo_tid_out, infifo_data_out, logic_cast_RxHdr_t'(infifo_rxhdr), logic_cast_TxHdr_t'(infifo_hdr_out)});
	    end

	 end // else: !if(init)
      end
      // endfunction // infifo_to_outfifo
   endtask // infifo_to_outfifo

   // Glue process
   always @(posedge clk) begin
      if (rst) begin
	 infifo_to_outfifo(1);
      end
      else if (infifo_cnt != 0) begin
	 infifo_to_outfifo(0);
      end
   end


   /*
    * Output Staging
    */
   // Outfifo Full/Empty
   assign outfifo_almfull  = (outfifo_cnt > VISIBLE_FULL_THRESH) ? 1 : 0;

   always @(*) begin
      if (outfifo_cnt == 0)
	outfifo_empty <= 1;
      else
	outfifo_empty <= 0;
   end

   assign empty = outfifo_empty;

   // HDR out
   assign txhdr_out = TxHdr_t'(txhdr_out_vec);
   assign rxhdr_out = RxHdr_t'(rxhdr_out_vec);

   // Output pop process
   always @(posedge clk) begin : read_out_proc
      if (rst) begin
	 valid_out <= 0;
      end
      else if (read_en && (outfifo.size() != 0)) begin
	 { tid_out, data_out, rxhdr_out_vec, txhdr_out_vec } <= outfifo.pop_front();
	 valid_out         <= 1;
      end
      else begin
	 valid_out         <= 0;
      end
   end

   // Log output pop
`ifdef ASE_DEBUG
   always @(posedge clk) begin
      if (valid_out) begin
	 $fwrite(log_fd, "%d | EXIT => tid=%x with %s %s \n", $time, tid_out, return_txhdr(txhdr_out), return_rxhdr(rxhdr_out) );
      end
   end
`endif


   /*
    * Transaction IN-OUT checker
    * Sniffs dropped transactions, unexpected mdata, vc or mcl responses
    */
`ifdef ASE_DEBUG
   TxHdr_t     check_hdr_array[*];
   int         check_vld_array[*];

   // Check and delete from array
   function automatic void check_delete_from_array(longint key);
      begin
	 if (check_hdr_array.exists(key)) begin
	    check_hdr_array.delete(key);
	    check_vld_array.delete(key);
	 end
	 else begin
	    `BEGIN_RED_FONTCOLOR;
	    $display(" ** HASH ERROR ** %x key was not found ", key);
	    $fwrite(log_fd, " ** HASH ERROR ** %x key was not found ", key);
	    `END_RED_FONTCOLOR;
	 end
      end
   endfunction

   // Update & self-ccheck process
   always @(posedge clk) begin
      // Push to channel
      if (write_en) begin
   	 if (WRITE_CHANNEL == 0) begin
   	    for (int ii = 0; ii <= hdr_in.len ; ii = ii + 1) begin
	       check_hdr_array [tid_in] <= hdr_in;
	       check_vld_array [tid_in] <= hdr_in.len + 1;
   	    end
   	 end
   	 else if (WRITE_CHANNEL == 1) begin
	    check_hdr_array [tid_in] <= hdr_in;
	    check_vld_array [tid_in] <= 1;
   	 end
      end
      // Pop from channel
      if (valid_out) begin
	 check_vld_array[tid_out] = check_vld_array[tid_out] - 1;
	 if (check_vld_array[tid_out] == 0) begin
	    check_delete_from_array( tid_out );
	 end
	 // *** VC checks here ***
	 if ((check_hdr_array[tid_out].vc != VC_VA) && (rxhdr_out.vc_used != check_hdr_array[tid_out].vc)) begin
	    `BEGIN_RED_FONTCOLOR;
	    $display("** ERROR **: VC was assigned incorrectly");
	    `END_RED_FONTCOLOR;
	    start_simkill_countdown();
	 end
	 // ** MDATA checks here ***
	 if (rxhdr_out.mdata != check_hdr_array[tid_out].mdata) begin
	    `BEGIN_RED_FONTCOLOR;
	    $display("** ERROR **: MDATA was assigned incorrectly");
	    `END_RED_FONTCOLOR;
	    start_simkill_countdown();
	 end
      end
   end
`endif


endmodule // inorder_wrf_channel
