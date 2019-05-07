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
 * Module Info: Latency modeling scoreboard system
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * *********************************************************************************
 * SR-5.0.0-alpha onwards implementation
 * ---------------------------------------------------------------------------------
 *
 *                     TBD cachesim link
 *                            ||
 *                       /---------------------------------\
 *             |-->vl0-->|          |           |          |
 *             |         | assign   |   wait    | multi-CL |
 * -->infifo-->|-->vh0-->| delayed  | stattions | breakout |-->outfifo-->
 *             |         | action   |           |          |
 *             |-->vh1-->|          |           |          |
 *                       \---------------------------------/
 *
 * - Input FIFO stages requests and asserts AlmostFull signal             |
 *   -- Feeds 2 high-lat and 1 low-lat lanes                              |
 * - If VA                                                                | Request
 *   -- Round robin between VH and VL lanes                               | Order
 *   -- Response channels (VC_USED) is assigned here                      | Maintain
 * - Assignment of waits is done and pushed to wait stations              |
 * - When ready to pop from wait stations                                 |---------
 *   -- If multi-CL is observed, request is broken out to multiple-single |
 *   -- If single line is observed, it is passed through                  |
 *   -- Unit generates RxHdr output to send response back to AFU          | Request
 * - If fence is observed with:                                           | order
 *   -- VA      : All channels fenced                                     | changed
 *   -- VL0/VHx : Requested channel is fenced                             |
 *
 * *********************************************************************************
 * SR 4.1.x - SR 5.0.0-prealpha implementation
 * ---------------------------------------------------------------------------------
 * - Transactions are stored when request comes from AFU
 * - Random number generator chooses a delay component between MIN_DELAY & MAX_DELAY
 * - When a request's "time has come", it gets called by cci_emulator
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

module outoforder_wrf_channel
  #(
    parameter string DEBUG_LOGNAME = "channel.log",
    parameter int    NUM_WAIT_STATIONS = 8,
    parameter int    NUM_STATIONS_FULL_THRESH = 3,
    parameter int    COUNT_WIDTH = 8,
    parameter int    VISIBLE_DEPTH_BASE2 = 8,
    parameter int    VISIBLE_FULL_THRESH = 32,
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
    // Status inputs to hazard detector logic
    output 			       ase_haz_pkt hazpkt_in,
    output 			       ase_haz_pkt hazpkt_out
    );


   /*
    * FUNCTION: get_random_from_range
    */
   function int get_random_from_range(int low,
				      int high);
      int rand_out;

      // rand_out = abs_val($random() % (high + 1 - low) + low);
      rand_out = $urandom_range(low, high);
      return rand_out;
   endfunction


   /*
    * Optional tracking log - enabled by ASE_DEBUG
    */
`ifdef ASE_PROFILE
   // Distribution histogram counters
   int histogram_stats[`ASE_MAX_LATENCY];
   initial begin
      // Initialize histogram counters
      for(int ii=0; ii<`ASE_MAX_LATENCY; ii = ii + 1) begin
	 histogram_stats[ii] = 0;
      end
   end
`endif

`ifdef ASE_DEBUG
   // Internal logger fd
   int 				       log_fd;
   initial begin
      log_fd = $fopen( DEBUG_LOGNAME, "w");
      $fwrite(log_fd, "Logger for %m transactions\n");
   end
`endif

   // Set random seed
   // initial begin
   //    // $srandom(cfg.ase_seed);
   //    // $urandom(cfg.ase_seed);
   // end

   localparam FIFO_WIDTH          = LATBUF_TID_WIDTH + CCIP_TX_HDR_WIDTH + CCIP_DATA_WIDTH;
   localparam OUTFIFO_WIDTH       = LATBUF_TID_WIDTH + CCIP_RX_HDR_WIDTH + CCIP_TX_HDR_WIDTH + CCIP_DATA_WIDTH;

   localparam LATBUF_SLOT_INVALID = 255;

   // Visible depth
   localparam VISIBLE_DEPTH = 2**VISIBLE_DEPTH_BASE2;

   // Internal FIFOs are invisible FIFOs inside channel
   localparam INTERNAL_FIFO_DEPTH          = 64;
   localparam INTERNAL_FIFO_ALMFULL_THRESH = 8;

   // Internal signals
   logic [LATBUF_TID_WIDTH-1:0] 	  tid_in;
   logic [LATBUF_TID_WIDTH-1:0] 	  tid_out;

   // Infifo
   logic [FIFO_WIDTH-1:0] 		  infifo[$:VISIBLE_DEPTH-1];

   // Lanes
   logic [FIFO_WIDTH-1:0] 		  vl0_array[$:INTERNAL_FIFO_DEPTH-1];
   logic [FIFO_WIDTH-1:0] 		  vh0_array[$:INTERNAL_FIFO_DEPTH-1];
   logic [FIFO_WIDTH-1:0] 		  vh1_array[$:INTERNAL_FIFO_DEPTH-1];


   /*
    * Wrfence response mechanism
    * --------------------------
    *
    * - When Wrfence is observed on infifo, it is applied to
    *   required channel
    * - A response packet is formed and staged
    * THIS ENSURES WRFENCE RESPONSES ARE RETURNED IN ORDER RECEIVED
    * - A compare-wait engine waits pops wrfence_rsp_array, and
    *   waits until wrfence_flag is seen on records_t:interface
    * - Then response is placed on outfifo
    *
    * These are used only when WRITE_CHANNEL == 1.
    */
   // Wrfence response staging
   logic [(LATBUF_TID_WIDTH+CCIP_RX_HDR_WIDTH+CCIP_TX_HDR_WIDTH-1):0] wrfence_rsp_array[$];
   // Wrfence assert/deassert/status/compare
   logic 							      wrfence_rspvalid;
   logic [LATBUF_TID_WIDTH-1:0]					      wrfence_rsptid;
   RxHdr_t							      wrfence_rsphdr;
   TxHdr_t							      wrfence_reqhdr;
   logic 							      vl0_wrfence_deassert;
   logic 							      vh0_wrfence_deassert;
   logic 							      vh1_wrfence_deassert;

   // Outfifo
   logic [OUTFIFO_WIDTH-1:0] 					      outfifo[$:VISIBLE_DEPTH-1];

   // FIFO counts
   int 								      infifo_cnt;
   int 								      infifo_len_pushed;
   int 								      infifo_len_popped;
   int 								      vl0_array_cnt;
   int 								      vh0_array_cnt;
   int 								      vh1_array_cnt;
   int 								      outfifo_cnt;
   int 								      wrfence_rsp_cnt;

   logic 							      vl0_array_full;
   logic 							      vh0_array_full;
   logic 							      vh1_array_full;

   logic 							      vl0_array_empty;
   logic 							      vh0_array_empty;
   logic 							      vh1_array_empty;

   logic 							      outfifo_empty;
   logic 							      outfifo_almfull;

   logic [2:0] 							      vc_push;

   logic 							      some_lane_full;

   always @(*) begin : lane_fullcheck_comb
      some_lane_full <= vl0_array_full | vh0_array_full | vh1_array_full;
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

   // Counts/fill level
   always_ff @(posedge clk) begin : cnt_proc
      if (rst) begin
	 infifo_cnt <= 0;
      end
      else begin
	 if (WRITE_CHANNEL == 1) begin
	    // Writes can just use the number of entries in the FIFO since there is
	    // a message for every beat in a multi-line write.
	    infifo_cnt <= infifo.size();
	 end
	 else begin
	    // For reads, track the number of lines in flight and not just the number
	    // of requests in flight.
	    infifo_cnt <= infifo_cnt + infifo_len_pushed - infifo_len_popped;
	 end
      end

      vl0_array_cnt   <= vl0_array.size();
      vh0_array_cnt   <= vh0_array.size();
      vh1_array_cnt   <= vh1_array.size();
      outfifo_cnt     <= outfifo.size();
      wrfence_rsp_cnt <= wrfence_rsp_array.size();
   end

   assign vl0_array_full  = (vl0_array_cnt > INTERNAL_FIFO_ALMFULL_THRESH) ? 1 : 0;
   assign vh0_array_full  = (vh0_array_cnt > INTERNAL_FIFO_ALMFULL_THRESH) ? 1 : 0;
   assign vh1_array_full  = (vh1_array_cnt > INTERNAL_FIFO_ALMFULL_THRESH) ? 1 : 0;

   assign vl0_array_empty = (vl0_array_cnt == 0) ? 1 : 0;
   assign vh0_array_empty = (vh0_array_cnt == 0) ? 1 : 0;
   assign vh1_array_empty = (vh1_array_cnt == 0) ? 1 : 0;

   // Almfull signal
   always @(posedge clk) begin : almfull_proc
      if (rst) begin
	 almfull <= 1;
      end
      else if (infifo_cnt > VISIBLE_FULL_THRESH) begin
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
    * Scoreboard logic
    */
   // Enumerate states
   typedef enum {LatSc_Disabled,
		 LatSc_Countdown,
		 LatSc_DoneReady,
		 LatSc_RecordPopped} latsc_fsmState;

   // Transaction storage
   typedef struct
		  {
		     TxHdr_t                        hdr  [0:LATBUF_MAX_TXN-1]; // in
		     logic [CCIP_DATA_WIDTH-1:0]    data [0:LATBUF_MAX_TXN-1]; // in
		     logic [LATBUF_TID_WIDTH-1:0]   tid  [0:LATBUF_MAX_TXN-1]; // in
		     logic [LATBUF_TIMER_WIDTH-1:0] ctr_out;       // out
		     int 			    num_items;     // in
		     // logic 			    record_valid;  // out
		     logic 			    record_ready;  // out
		     // logic 			    record_push;   // in
		     logic 			    record_pop;    // in
		     latsc_fsmState                 state;         // out
		  } transact_t;

   // Array of stored transactions
   transact_t records[NUM_WAIT_STATIONS] ;

   logic [0:NUM_WAIT_STATIONS-1] 		    record_vl0_flag_arr;
   logic [0:NUM_WAIT_STATIONS-1] 		    record_vh0_flag_arr;
   logic [0:NUM_WAIT_STATIONS-1] 		    record_vh1_flag_arr;

   // logic [0:NUM_WAIT_STATIONS-1] 		    record_pop_arr;
   // logic [0:NUM_WAIT_STATIONS-1] 		    record_push_arr;

   /*
    * Busy state management
    */
   logic [0:NUM_WAIT_STATIONS-1] 		    latbuf_busy_state;

   // Set Record as busy
   function void set_latbuf_busy(int slot);
      begin
	 latbuf_busy_state[slot] = 1;
      end
   endfunction

   // Unset record as available
   function void unset_latbuf_available(int slot);
      begin
	 latbuf_busy_state[slot] = 0;
      end
   endfunction


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

   assign infifo_len_pushed = (write_en ? int'(hdr_in.len) + 1 : 0);


   // Pop infifo, arbitrate between lanes
   logic [CCIP_DATA_WIDTH-1:0]   infifo_data_out;
   logic [LATBUF_TID_WIDTH-1:0]  infifo_tid_out;
   logic [CCIP_TX_HDR_WIDTH-1:0] infifo_hdr_out_vec;
   TxHdr_t                       infifo_hdr_out;
   logic 			 infifo_vld_out;

   ccip_vc_t 		  vc_rd_arb;
   ccip_vc_t 		  vc_wr_arb;

   ccip_vc_t [0:3] sel_vc_array = {VC_VL0, VC_VH0, VC_VL0, VC_VH1};
   logic [1:0] 			 curr_vc_index = 2'b0;


   /*
    * Read MCL select VC
    */
   function automatic void select_vc_read(int init, ref TxHdr_t hdr);
      begin
   	 if (init) begin
   	    vc_rd_arb = ccip_vc_t'(VC_VL0);
   	 end
   	 else begin
   	    if (hdr.vc == VC_VA) begin
   	       case ({vl0_array_full, vh0_array_full, vh1_array_full})
   	       	 3'b000:
   		   begin
   		      hdr.vc = sel_vc_array[curr_vc_index];
   		      curr_vc_index = curr_vc_index + 1;
   		   end
   	       	 3'b001: hdr.vc = VC_VL0;
   	       	 3'b010: hdr.vc = VC_VH1;
   	       	 3'b011: hdr.vc = VC_VL0;
   	       	 3'b100: hdr.vc = VC_VH0;
   	       	 3'b101: hdr.vc = VC_VH0;
   	       	 3'b110: hdr.vc = VC_VH1;
   	       endcase
   	       vc_rd_arb = ccip_vc_t'(hdr.vc);
   	    end
	 end
      end
   endfunction

   /*
    * Write MCL select VC
    */
   function automatic void select_vc_write(int init, ref TxHdr_t hdr);
      begin
   	 if (init) begin
   	    vc_wr_arb = ccip_vc_t'(VC_VL0);
   	 end
	 else if (isIntrRequest(hdr)) begin
	    hdr.vc = VC_VH0;
	    vc_wr_arb = ccip_vc_t'(VC_VH0);
	 end
   	 else if (hdr.sop && (hdr.vc == VC_VA) && isWriteRequest(hdr)) begin
   	    case ({vl0_array_full, vh0_array_full, vh1_array_full})
   	      3'b000:
   		begin
   		   hdr.vc = sel_vc_array[curr_vc_index];
   		   curr_vc_index = curr_vc_index + 1;
   		end
   	      3'b001: hdr.vc = VC_VL0;
   	      3'b010: hdr.vc = VC_VH1;
   	      3'b011: hdr.vc = VC_VL0;
   	      3'b100: hdr.vc = VC_VH0;
   	      3'b101: hdr.vc = VC_VH0;
   	      3'b110: hdr.vc = VC_VH1;
   	    endcase
   	    vc_wr_arb = ccip_vc_t'(hdr.vc);
	 end // if (hdr.sop && (hdr.vc == VC_VA))
	 else if (hdr.sop && (hdr.vc != VC_VA)) begin
   	    vc_wr_arb = ccip_vc_t'(hdr.vc);
	 end
	 else if (~hdr.sop) begin
	    hdr.vc = vc_wr_arb;
	 end
      end
   endfunction

   // Write fence response generator
   function automatic logic [CCIP_RX_HDR_WIDTH-1:0] prepare_wrfence_response(TxHdr_t wrfence);
      RxHdr_t wrfence_rsp;
      logic [CCIP_RX_HDR_WIDTH-1:0] wrfence_rsp_vec;
      begin
	 // Precast
	 wrfence_rsp = RxHdr_t'(0);
	 // response
	 wrfence_rsp.vc_used  = wrfence.vc;
	 wrfence_rsp.resptype = ASE_WRFENCE_RSP;
	 wrfence_rsp.mdata    = wrfence.mdata;
	 // Cast back and return
	 wrfence_rsp_vec = logic_cast_RxHdr_t'(wrfence_rsp);
	 return wrfence_rsp_vec;
      end
   endfunction


   /*
    * INFIFO->VC_sel
    * -----------------------------------------
    * - Read infifo contents
    * - If WrFence (either channel)
    *   = Block required channel(s)
    *   = Stage WrFence response in wrfence_rsp_array
    * - Else !wrfence
    *   = Select VC
    *   = Stage into response array
    */

   task automatic infifo_fetch_state ();
      {infifo_tid_out, infifo_data_out, infifo_hdr_out_vec} = infifo[0];
      infifo_hdr_out = TxHdr_t'(infifo_hdr_out_vec);
   endtask

   // ================================================================== //
   // Read CHANNEL infifo_to_vc_put
   // ================================================================== //
   task automatic READ_infifo_to_vc_push ();
      logic [PHYSCLADDR_WIDTH-1:0] c0tx_vl0_addr_base;
      TxHdr_t                      vl0_hdr;
      begin
	 infifo.pop_front();
	 select_vc_read (0, infifo_hdr_out);

	 case (infifo_hdr_out.vc)
	   VC_VL0:
	     begin
		vc_push = 3'b100;
		for(int ii = 0; ii <= infifo_hdr_out.len; ii = ii + 1) begin
		   vl0_hdr = infifo_hdr_out;
		   if (ii == 0) begin
		      c0tx_vl0_addr_base = infifo_hdr_out.addr;
		   end
		   vl0_hdr.addr = infifo_hdr_out.addr + ii;
		   vl0_hdr.len = ccip_len_t'(ii);
		   vl0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(vl0_hdr)});
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc(VL0) : tid=%x %s sent to VL0\n", $time, infifo_tid_out, return_txhdr(vl0_hdr) );
 `endif
		end // for (int ii = 0; ii <= infifo_hdr_out.len; ii = ii + 1)
	     end

	   VC_VH0:
	     begin
		vc_push = 3'b010;
		vh0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
 `ifdef ASE_DEBUG
		$fwrite(log_fd, "%d | infifo_to_vc(VH0) : tid=%x %s sent to VH0\n", $time, infifo_tid_out, return_txhdr(infifo_hdr_out));
 `endif
	     end

	   VC_VH1:
	     begin
		vc_push = 3'b001;
		vh1_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
 `ifdef ASE_DEBUG
		$fwrite(log_fd, "%d | infifo_to_vc(VH1) : tid=%x %s sent to VH1\n", $time, infifo_tid_out, return_txhdr(infifo_hdr_out));
 `endif
	     end
	 endcase
      end
   endtask

   // ================================================================== //
   // Write CHANNEL infifo_to_vc_put
   // ================================================================== //
   task automatic WRITE_infifo_to_vc_push ();
      logic [PHYSCLADDR_WIDTH-1:0] c0tx_vl0_addr_base;
      TxHdr_t                      vl0_hdr;
      begin
	 infifo.pop_front();

	 // ----------------------------------------------------------------------- //
	 // If Write fence is observed
	 // ----------------------------------------------------------------------- //
	 if (infifo_hdr_out.reqtype == ASE_WRFENCE) begin
	    vc_push = 3'b111;
	    case (infifo_hdr_out.vc)
	      // If VA, fence all channels, and stage one coalesced response
	      VC_VA:
		begin
		   // Fence activatd
		   vl0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   vh0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   vh1_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   // Wrfence response
		   wrfence_rsp_array.push_back( {infifo_tid_out, prepare_wrfence_response(infifo_hdr_out), logic_cast_TxHdr_t'(infifo_hdr_out) } );
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc: WrFence of tid=%x inserted into VA\n", $time, infifo_tid_out);
 `endif
		end

	      // If single channel fence, stage requisite response
	      VC_VL0:
		begin
		   vl0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   wrfence_rsp_array.push_back( {infifo_tid_out, prepare_wrfence_response(infifo_hdr_out), logic_cast_TxHdr_t'(infifo_hdr_out) } );
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc: WrFence of tid=%x inserted into VL0\n", $time, infifo_tid_out);
 `endif
		end

	      VC_VH0:
		begin
		   vh0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   wrfence_rsp_array.push_back( {infifo_tid_out, prepare_wrfence_response(infifo_hdr_out), logic_cast_TxHdr_t'(infifo_hdr_out) } );
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc: WrFence of tid=%x inserted into VH0\n", $time, infifo_tid_out);
 `endif
		end

	      VC_VH1:
		begin
		   vh1_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
		   wrfence_rsp_array.push_back( {infifo_tid_out, prepare_wrfence_response(infifo_hdr_out), logic_cast_TxHdr_t'(infifo_hdr_out) } );
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc: WrFence of tid=%x inserted into VH1\n", $time, infifo_tid_out);
 `endif
		end

	    endcase
	 end // if (infifo_hdr_out.reqtype == ASE_WRFENCE)
	 // ----------------------------------------------------------------------- //
	 // Any non-WRFence transaction
	 // ----------------------------------------------------------------------- //
	 else begin
	    select_vc_write (0, infifo_hdr_out);
	    // No fence
	    case (infifo_hdr_out.vc)
	      VC_VL0:
		begin
		   vc_push = 3'b100;
		   vl0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc(VL0) : tid=%x %s sent to VL0\n", $time, infifo_tid_out, return_txhdr(infifo_hdr_out) );
 `endif
		end

	      VC_VH0:
		begin
		   vc_push = 3'b010;
		   vh0_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc(VH0) : tid=%x %s sent to VH0\n", $time, infifo_tid_out, return_txhdr(infifo_hdr_out));
 `endif
		end

	      VC_VH1:
		begin
		   vc_push = 3'b001;
		   vh1_array.push_back({infifo_tid_out, infifo_data_out, logic_cast_TxHdr_t'(infifo_hdr_out)});
 `ifdef ASE_DEBUG
		   $fwrite(log_fd, "%d | infifo_to_vc(VH1) : tid=%x %s sent to VH1\n", $time, infifo_tid_out, return_txhdr(infifo_hdr_out));
 `endif
		end
	    endcase
	 end // else: !if(infifo_hdr_out.reqtype == ASE_WRFENCE)
      end
   endtask


   /*
    * Virtual channel select and push
    */
   generate
      // ================================================================== //
      // Read CHANNEL infifo_to_vc_put
      // ================================================================== //
      if (WRITE_CHANNEL == 0) begin
	 always @(posedge clk) begin : vc_selector_proc
	    infifo_fetch_state();

	    if (rst) begin
	       vc_push <= 3'b000;
	       select_vc_read (1, infifo_hdr_out);
	       infifo_vld_out     <= 0;
	       infifo_len_popped  <= 0;
	    end
	    else if (~some_lane_full && (infifo.size() != 0)) begin
	       READ_infifo_to_vc_push();
	       infifo_vld_out     <= 1;
	       infifo_len_popped  <= int'(infifo_hdr_out.len) + 1;
	    end
	    else begin
	       vc_push <= 3'b000;
	       infifo_vld_out     <= 0;
	       infifo_len_popped  <= 0;
	    end
	 end
      end
      // ================================================================== //
      // Write CHANNEL infifo_to_vc_put
      // ================================================================== //
      else if (WRITE_CHANNEL == 1) begin
	 always @(posedge clk) begin : vc_selector_proc
	    infifo_fetch_state();

	    if (rst) begin
	       vc_push <= 3'b000;
	       select_vc_write (1, infifo_hdr_out);
	       infifo_vld_out     <= 0;
	       infifo_len_popped  <= 0;
	    end
	    else if ((~some_lane_full || ~infifo_hdr_out.sop) &&
		     (infifo.size() != 0) &&
		     // Don't start popping a multi-line packet until the full packet
		     // is buffered in infifo.	Failure to do this leads to possible
		     // deadlocks in the later state machines.
		     ((infifo.size() > infifo_hdr_out.len) || ~infifo_hdr_out.sop))
	    begin
	       WRITE_infifo_to_vc_push();
	       infifo_vld_out     <= 1;
	       infifo_len_popped  <= int'(infifo_hdr_out.len) + 1;
	    end
	    else begin
	       vc_push <= 3'b000;
	       infifo_vld_out     <= 0;
	       infifo_len_popped  <= 0;
	    end
	 end
      end
   endgenerate


   // Lane pop and latency scoreboard push
   logic vl0_wrfence_flag;
   logic vh0_wrfence_flag;
   logic vh1_wrfence_flag;

   logic glbl_wrfence_pop_status;

   logic [LATBUF_TID_WIDTH-1:0] vl0_wrfence_tid;
   logic [LATBUF_TID_WIDTH-1:0] vh0_wrfence_tid;
   logic [LATBUF_TID_WIDTH-1:0] vh1_wrfence_tid;

   int 				latbuf_push_ptr;
   int 				latbuf_pop_ptr;

   int 				vl0_records_cnt ;
   int 				vh0_records_cnt ;
   int 				vh1_records_cnt;

   int 				latbuf_cnt;
   logic 			latbuf_full;
   logic 			latbuf_almfull;
   logic 			latbuf_empty;

   // Count used latbuf
   assign vl0_records_cnt = $countones(record_vl0_flag_arr);
   assign vh0_records_cnt = $countones(record_vh0_flag_arr);
   assign vh1_records_cnt = $countones(record_vh1_flag_arr);

   // Total count
   always @(posedge clk) begin : latbuf_cnt_proc
      latbuf_cnt <= vl0_records_cnt + vh0_records_cnt + vh1_records_cnt;
   end

   // Latbuf status signals
   always @(*) begin : latbuf_empty_comb
      if (latbuf_cnt == 0)
	latbuf_empty <= 1;
      else
	latbuf_empty <= 0;
   end

   always @(*) begin : latbuf_full_comb
      if (latbuf_cnt == NUM_WAIT_STATIONS)
	latbuf_full <= 1;
      else
	latbuf_full <= 0;
   end

   // Initial assertion
   initial begin
      if (NUM_WAIT_STATIONS-5 <= 0 ) begin
	 $display("** ERROR ** => (NUM_WAIT_STATIONS-5) must not be less than 0... operation cannot continue -- EXIT !");
	 start_simkill_countdown();
      end
   end

   // push_ptr selector
   function automatic integer find_next_push_slot();
      int 				     find_iter;
      int 				     ret_free_slot;
      begin
   	 for(find_iter = latbuf_push_ptr;
	     find_iter < latbuf_push_ptr + NUM_WAIT_STATIONS;
	     find_iter = find_iter + 1) begin
	    ret_free_slot = find_iter % NUM_WAIT_STATIONS;
   	    // if ( (records[ret_free_slot].state == LatSc_Disabled) && ~records[ret_free_slot].record_valid ) begin
   	    if ( (records[ret_free_slot].state == LatSc_Disabled) && ~latbuf_busy_state[ret_free_slot] ) begin
   	       return ret_free_slot;
   	    end
   	 end
	 return LATBUF_SLOT_INVALID;
      end
   endfunction

   // MCL Write in progress
   logic mcl_write_in_progress;
   int 	 mcl_txn_iter;
   int 	 record_len;


   /*
    * Latbuf assignment process
    * - Read and update record in latency scoreboard
    */
   assign latbuf_almfull =
      ((latbuf_cnt > NUM_STATIONS_FULL_THRESH) && ~mcl_write_in_progress) ? 1 : 0;

   // Read channel VC->LATBUF glue
   function automatic void READ_get_vc_put_latbuf (ref logic [FIFO_WIDTH-1:0] array[$:INTERNAL_FIFO_DEPTH-1]);
      logic [CCIP_TX_HDR_WIDTH-1:0] array_hdr;
      logic [CCIP_DATA_WIDTH-1:0]   array_data;
      logic [LATBUF_TID_WIDTH-1:0]  array_tid;
      TxHdr_t                                                                hdr;
      int 			    ptr;
      begin
	 hazpkt_in.valid = 0;
	 // Find a pointer to use
	 ptr = find_next_push_slot();
	 latbuf_push_ptr = ptr;
 	 // ------------------------------------------------------ //
	 // If slot is legal only then proceed
	 // ------------------------------------------------------ //
	 if (ptr != LATBUF_SLOT_INVALID) begin
	    set_latbuf_busy(ptr);
	    {array_tid, array_data, array_hdr} = array.pop_front();
	    hdr = TxHdr_t'(array_hdr);
	    record_len = int'(hdr.len);
	    records[ptr].hdr[0]       = hdr;
	    records[ptr].data[0]      = array_data;
	    records[ptr].tid[0]       = array_tid;
	    // records[ptr].record_push  = 1;
	    // records[ptr].record_valid = 1;
	    records[ptr].num_items    = int'(ASE_1CL);
	    mcl_write_in_progress     = 0;
 `ifdef ASE_DEBUG
	    $fwrite(log_fd, "%d | latbuf_push : tid=%x %s sent to record[%02d][0]\n", $time, array_tid, return_txhdr(hdr), ptr);
 `endif
	    hazpkt_in.hdr     = hdr;
	    hazpkt_in.tid     = array_tid;
	    hazpkt_in.valid   = 1;
	 end // if (ptr != LATBUF_SLOT_INVALID)
 `ifdef ASE_DEBUG
	 else begin
	    $fwrite(log_fd, "%d | latbuf_push : Returned slot_num = %d .. UNUSED\n", $time, LATBUF_SLOT_INVALID);
	 end
 `endif
      end
   endfunction


   // Write channel VC->LATBUF glue
   function automatic void WRITE_get_vc_put_latbuf (ref logic [FIFO_WIDTH-1:0] array[$:INTERNAL_FIFO_DEPTH-1],
						    ref logic 			     wrfence_flag,
						    ref logic [LATBUF_TID_WIDTH-1:0] wrfence_tid
						    );
      logic [CCIP_TX_HDR_WIDTH-1:0] 						     array_hdr;
      logic [CCIP_DATA_WIDTH-1:0] 						     array_data;
      logic [LATBUF_TID_WIDTH-1:0] 						     array_tid;
      TxHdr_t                                                                 hdr;
      int 									     ptr;
      begin
	 hazpkt_in.valid = 0;
	 // Find a pointer to use
	 if (~mcl_write_in_progress) begin
	    ptr = find_next_push_slot();
	    latbuf_push_ptr = ptr;
	 end
	 else begin
	    ptr = latbuf_push_ptr;
	 end
	 // ------------------------------------------------------ //
	 // If slot is legal only then proceed
	 // ------------------------------------------------------ //
	 if (ptr != LATBUF_SLOT_INVALID) begin
	    {array_tid, array_data, array_hdr} = array.pop_front();
	    hdr = TxHdr_t'(array_hdr);
	    // Record base length
	    if (hdr.sop || ~isWriteRequest(hdr)) begin
	       mcl_txn_iter = 0;
	       record_len = int'(hdr.len);
	    end
	    // ------------------------------------------------------ //
	    // If Transaction is a Wrfence
	    // ------------------------------------------------------ //
	    if (hdr.reqtype == ASE_WRFENCE) begin
	       hazpkt_in.valid = 0;
	       wrfence_flag = 1;
	       wrfence_tid  = array_tid;
 `ifdef ASE_DEBUG
	       $fwrite(log_fd, "%d | latbuf_push : saw Wrfence on tid=%x on channel %s\n", $time, array_tid, ase_channel_type(hdr.vc));
 `endif
	    end
	    // ------------------------------------------------------ //
	    // If Transaction is a WRITE
	    // ------------------------------------------------------ //
	    else if (isWriteRequest(hdr) || isIntrRequest(hdr)) begin
	       // ------------------------------------------------------ //
	       // If a VHx transaction
	       // ------------------------------------------------------ //
	       if (isVHxRequest(hdr)) begin
		  set_latbuf_busy(ptr);
		  records[ptr].hdr[mcl_txn_iter]  = hdr;
		  records[ptr].data[mcl_txn_iter] = array_data;
		  records[ptr].tid[mcl_txn_iter]  = array_tid;
		  // records[ptr].record_push     = 1;
		  // records[ptr].record_valid    = 1;
		  records[ptr].num_items       = record_len;
		  if (mcl_txn_iter != record_len)
		    mcl_write_in_progress      = 1;
		  else
		    mcl_write_in_progress      = 0;
 `ifdef ASE_DEBUG
		  $fwrite(log_fd, "%d | latbuf_push : tid=%x %s sent to record[%02d][%02d]\n", $time, array_tid, return_txhdr(hdr), ptr, mcl_txn_iter);
 `endif
		  mcl_txn_iter = mcl_txn_iter + 1;
		  hazpkt_in.hdr   = hdr;
		  hazpkt_in.tid   = array_tid;
		  hazpkt_in.valid = isWriteRequest(hdr);
	       end // if (isVHxRequest(hdr))
	       // ------------------------------------------------------ //
	       // If a VL0 transaction
	       // ------------------------------------------------------ //
	       else begin
		  set_latbuf_busy(ptr);
		  records[ptr].hdr[0]       = hdr;
		  records[ptr].data[0]      = array_data;
		  records[ptr].tid[0]       = array_tid;
		  // records[ptr].record_push  = 1;
		  // records[ptr].record_valid = 1;
		  records[ptr].num_items    = int'(ASE_1CL);
		  mcl_write_in_progress     = 0;
 `ifdef ASE_DEBUG
		  $fwrite(log_fd, "%d | latbuf_push : tid=%x sent to record[%02d][0]\n", $time, array_tid, ptr);
 `endif
		  hazpkt_in.hdr   = hdr;
		  hazpkt_in.tid   = array_tid;
		  hazpkt_in.valid = isWriteRequest(hdr);
	       end // else: !if(isVHxRequest(hdr))
	    end // if (isWriteRequest(hdr))
	 end // if (ptr != LATBUF_SLOT_INVALID)
 `ifdef ASE_DEBUG
	 else begin
	    $fwrite(log_fd, "%d | latbuf_push : Returned slot_num = %d .. UNUSED\n", $time, LATBUF_SLOT_INVALID);
	 end // else: !if(ptr != LATBUF_SLOT_INVALID)
 `endif
      end
   endfunction


   // --------------------------------------------------------- //
   // States
   // --------------------------------------------------------- //
   typedef enum {Select_VL0, Select_VH0, Select_VH1} lssel_state;
   lssel_state vc_pop;

   generate
      // ====================================================================== //
      // READ CHANNEL
      // ====================================================================== //
      if (WRITE_CHANNEL == 0) begin
	 always @(posedge clk) begin : READ_latbuf_push_proc
	    if (rst) begin
   	       vc_pop <= Select_VL0;
	       vl0_wrfence_flag <= 0;
	       vh0_wrfence_flag <= 0;
	       vh1_wrfence_flag <= 0;
	       hazpkt_in.valid <= 0;
	       mcl_write_in_progress <= 0;
	       for(int ii = 0 ; ii < NUM_WAIT_STATIONS ; ii = ii + 1) begin
		  unset_latbuf_available(ii);
		  // records[ii].record_push <= 0;
		  // records[ii].record_valid <= 0;
	       end
	    end
	    else begin
	       // If input arrays are available
   	       case (vc_pop)
   		 Select_VL0:
   		   begin
		      if (~vl0_array_empty && ~latbuf_almfull) begin
			 hazpkt_in.valid <= 1;
			 READ_get_vc_put_latbuf(vl0_array);
		      end
		      else begin
			 hazpkt_in.valid <= 0;
		      end
   		      vc_pop <= Select_VH0;
   		   end

   		 Select_VH0:
   		   begin
		      if (~vh0_array_empty && ~latbuf_almfull) begin
			 hazpkt_in.valid <= 1;
			 READ_get_vc_put_latbuf(vh0_array);
		      end
		      else begin
			 hazpkt_in.valid <= 0;
		      end
   		      vc_pop <= Select_VH1;
   		   end

   		 Select_VH1:
   		   begin
		      if (~vh1_array_empty && ~latbuf_almfull) begin
			 hazpkt_in.valid <= 1;
			 READ_get_vc_put_latbuf(vh1_array);
		      end
		      else begin
			 hazpkt_in.valid <= 0;
		      end
   		      vc_pop <= Select_VL0;
   		   end

   		 default:
   		   begin
		      hazpkt_in.valid <= 0;
   		      vc_pop <= Select_VL0;
   		   end

   	       endcase // case (vc_pop)
	       // -------------------------------------------------- //
	       // Release latbuf_used & record_push
	       // -------------------------------------------------- //
	       for(int ii = 0 ; ii < NUM_WAIT_STATIONS ; ii = ii + 1) begin
		  if (records[ii].state == LatSc_Countdown) begin
		     unset_latbuf_available(ii);
		     // records[ii].record_push <= 0;
		     // records[ii].record_valid <= 0;
		  end
	       end
	    end
	 end
      end
      // ====================================================================== //
      // WRITE CHANNEL
      // ====================================================================== //
      else if (WRITE_CHANNEL == 1) begin
	 always @(posedge clk) begin : WRITE_latbuf_push_proc
	    if (rst) begin
	       hazpkt_in.valid <= 0;
   	       vc_pop <= Select_VL0;
	       vl0_wrfence_flag <= 0;
	       vh0_wrfence_flag <= 0;
	       vh1_wrfence_flag <= 0;
	       mcl_write_in_progress <= 0;
	       for(int ii = 0 ; ii < NUM_WAIT_STATIONS ; ii = ii + 1) begin
		  unset_latbuf_available(ii);
		  // records[ii].record_push <= 0;
		  // records[ii].record_valid <= 0;
	       end
	    end
	    else begin
 	       // hazpkt_in.valid <= 0;
	       // If input arrays are available
   	       case (vc_pop)
   		 Select_VL0:
   		   begin
		      // hazpkt_in.valid <= 0;
		      if (~vl0_wrfence_flag && ~vl0_array_empty && ~latbuf_almfull) begin
			 // hazpkt_in.valid <= 1;
			 WRITE_get_vc_put_latbuf(vl0_array, vl0_wrfence_flag, vl0_wrfence_tid );
		      end
		      else begin
			 hazpkt_in.valid <= 0;
		      end
		      if (~mcl_write_in_progress) begin
			 vc_pop <= Select_VH0;
		      end
   		   end

   		 Select_VH0:
   		   begin
		      // hazpkt_in.valid <= 0;
		      if (~vh0_wrfence_flag && ~vh0_array_empty && ~latbuf_almfull) begin
			 // hazpkt_in.valid <= 1;
			 WRITE_get_vc_put_latbuf(vh0_array, vh0_wrfence_flag, vh0_wrfence_tid );
		      end
		      else begin
		      	 hazpkt_in.valid <= 0;
		      end
		      if (~mcl_write_in_progress) begin
   			 vc_pop <= Select_VH1;
		      end
   		   end

   		 Select_VH1:
   		   begin
		      // hazpkt_in.valid <= 0;
		      if (~vh1_wrfence_flag && ~vh1_array_empty && ~latbuf_almfull) begin
			 // hazpkt_in.valid <= 1;
			 WRITE_get_vc_put_latbuf(vh1_array, vh1_wrfence_flag, vh1_wrfence_tid );
		      end
		      else begin
		      	 hazpkt_in.valid <= 0;
		      end
		      if (~mcl_write_in_progress) begin
   			 vc_pop <= Select_VL0;
		      end
   		   end

   		 default:
   		   begin
		      // hazpkt_in.valid <= 0;
   		      vc_pop <= Select_VL0;
   		   end

   	       endcase // case (vc_pop)
	       // ------------------------------------------------------------- //
	       // WrFence assertion logic (assertions in WRITE mode only)
	       // ------------------------------------------------------------- //
	       // If a VL0 fence is set, wait till downstream gets cleared
	       if (vl0_wrfence_flag && (vl0_records_cnt == 0) && vl0_wrfence_deassert) begin
		  hazpkt_in.valid <= 0;
		  vl0_wrfence_flag <= 0;
`ifdef ASE_DEBUG
		  $fwrite(log_fd, "%d | VL0 write fence popped\n", $time);
`endif
	       end
	       // If a VH0 fence is set, wait till downstream gets cleared
	       if (vh0_wrfence_flag && (vh0_records_cnt == 0) && vh0_wrfence_deassert) begin
		  hazpkt_in.valid <= 0;
		  vh0_wrfence_flag <= 0;
`ifdef ASE_DEBUG
		  $fwrite(log_fd, "%d | VH0 write fence popped\n", $time);
`endif
	       end
	       // If a VH0 fence is set, wait till downstream gets cleared
	       if (vh1_wrfence_flag && (vh1_records_cnt == 0) && vh1_wrfence_deassert) begin
		  hazpkt_in.valid <= 0;
		  vh1_wrfence_flag <= 0;
`ifdef ASE_DEBUG
		  $fwrite(log_fd, "%d | VH1 write fence popped\n", $time);
`endif
	       end
	       // -------------------------------------------------- //
	       // Release latbuf_used & record_push
	       // -------------------------------------------------- //
	       for(int ii = 0 ; ii < NUM_WAIT_STATIONS ; ii = ii + 1) begin
		  if (records[ii].state == LatSc_Countdown) begin
		     unset_latbuf_available(ii);
		     // records[ii].record_push <= 0;
		     // records[ii].record_valid <= 0;
		  end
	       end
	    end // else: !if(rst)
	 end // block: latbuf_push_proc
      end
   endgenerate

   // Print process (DEBUG ONLY)
`ifdef ASE_DEBUG
   always @(posedge clk) begin
      if (hazpkt_in.valid) begin
	 $fwrite(log_fd, "%d | hazpkt_in => tid = %x, hdr=%s\n", $time, hazpkt_in.tid, return_txhdr(hazpkt_in.hdr));
      end
   end
`endif

   /*
    * Latency scoreboard
    */
   // Get delay function
   function int get_delay(input TxHdr_t hdr);
      int delay;
      begin
	 case (hdr.vc)
	   VC_VL0:
	     begin
		delay = get_random_from_range(`RDWR_VL_LATRANGE);
	     end
	   VC_VH0:
	     begin
		delay = get_random_from_range(`RDWR_VH_LATRANGE);
	     end
	   VC_VH1:
	     begin
		delay = get_random_from_range(`RDWR_VH_LATRANGE);
	     end
	   VC_VA:
	     begin
		delay = 100;
`ifdef ASE_DEBUG
		$fwrite(log_fd, "%d | *ERROR* => get_delay() must not get VC_VA", $time);
`endif
	     end
	 endcase // case (hdr.vc)
`ifdef ASE_PROFILE
	 histogram_stats[delay] = histogram_stats[delay] + 1;
`endif
	 return delay;
      end
   endfunction


   // Wait station logic
   genvar 				     ii;
   generate
      for ( ii = 0; ii < NUM_WAIT_STATIONS; ii = ii + 1) begin : gen_latsc
	 // Record process
	 always @(posedge clk) begin : record_proc
	    if (rst) begin
	       records[ii].ctr_out      <= 0;
	       records[ii].record_ready <= 0;
	       record_vl0_flag_arr[ii] <= 0;
	       record_vh0_flag_arr[ii] <= 0;
	       record_vh1_flag_arr[ii] <= 0;
	    end
	    else begin
	       case (records[ii].state)
		 LatSc_Disabled:
		   begin
		      records[ii].record_ready  <= 0;
		      //if (records[ii].record_push) begin
		      // if (records[ii].record_valid) begin
		      if (latbuf_busy_state[ii]) begin
			 records[ii].ctr_out      <= get_delay(records[ii].hdr[0]);
			 records[ii].state        <= LatSc_Countdown;
			 if (records[ii].hdr[0].vc == VC_VL0) begin
	 		    record_vl0_flag_arr[ii] <= 1;
			 end
			 else if (records[ii].hdr[0].vc == VC_VH0) begin
	 		    record_vh0_flag_arr[ii] <= 1;
			 end
			 else if (records[ii].hdr[0].vc == VC_VH1) begin
	 		    record_vh1_flag_arr[ii] <= 1;
			 end
		      end
		      else begin
			 records[ii].ctr_out      <= 0;
			 records[ii].state        <= LatSc_Disabled;
		      end
		   end

		 LatSc_Countdown:
		   begin
		      records[ii].ctr_out      <= records[ii].ctr_out - 1;
		      if (records[ii].ctr_out == 0) begin
			 records[ii].record_ready <= 1;
			 records[ii].state        <= LatSc_DoneReady;
		      end
		      else begin
			 records[ii].record_ready <= 0;
			 records[ii].state        <= LatSc_Countdown;
		      end
		   end

		 LatSc_DoneReady:
		   begin
		      records[ii].ctr_out      <= 0;
		      if (records[ii].record_pop) begin
			 records[ii].record_ready <= 0;
			 records[ii].state        <= LatSc_RecordPopped;
		      end
		      else begin
			 records[ii].record_ready <= 1;
			 records[ii].state        <= LatSc_DoneReady;
		      end
		   end

		 LatSc_RecordPopped:
		   begin
	 	      record_vl0_flag_arr[ii] <= 0;
	 	      record_vh0_flag_arr[ii] <= 0;
	 	      record_vh1_flag_arr[ii] <= 0;
		      records[ii].record_ready <= 0;
		      records[ii].ctr_out      <= 0;
		      if (~records[ii].record_pop) begin
			 records[ii].state        <= LatSc_Disabled;
		      end
		      else begin
			 records[ii].state        <= LatSc_RecordPopped;
		      end
		   end

		 default:
		   begin
		      records[ii].record_ready <= 0;
		      records[ii].ctr_out      <= 0;
		      records[ii].state        <= LatSc_Disabled;
		   end
	       endcase
	    end
	 end

      end
   endgenerate


   // Find a transaction to release to output stage
   function integer find_next_pop_slot();
      int ret_pop_slot;
      int pop_iter;
      int sel_slot;
      begin
	 for(pop_iter = latbuf_pop_ptr; pop_iter < latbuf_pop_ptr + NUM_WAIT_STATIONS ; pop_iter = pop_iter + 1) begin
	    sel_slot = pop_iter % NUM_WAIT_STATIONS;
	    if (records[sel_slot].record_ready) begin
	       return sel_slot;
	    end
	 end
	 return LATBUF_SLOT_INVALID;
      end
   endfunction


   // Status of unroll (readouts)
   logic [CCIP_RX_HDR_WIDTH-1:0] rxhdr_out_vec;
   logic [CCIP_TX_HDR_WIDTH-1:0] txhdr_out_vec;


   /*
    * get_latbuf_unroll_put_outfifo : Get a record from latbuf, unroll and stage in outfifo
    * --------------------------------------------------------------------------------------
    *   VirtChannel       Read/Write         MCL       Action
    * -------------------------------------------------------------
    *       VL0              Read             0        Passthru
    *       VL0              Read             1        Passthru
    *       VL0              Write            0        Passthru
    *       VL0              Write            1        Passthru
    *       VHx              Read             0        Unroll, outfifo
    *       VHx              Read             1        Unroll, outfifo
    *       VHx              Write            0        iterate, outfifo
    *       VHx              Write            1        iterate, outfifo
    *
    */
   // Read channel latbuf -> outfifo
   task automatic READ_get_latbuf_unroll_put_outfifo(ref logic [OUTFIFO_WIDTH-1:0] array[$:VISIBLE_DEPTH-1] );
      TxHdr_t                     base_hdr;
      logic [CCIP_DATA_WIDTH-1:0] data;
      int 			  ptr;
      TxHdr_t                     txhdr;
      RxHdr_t                     rxhdr;
      logic [LATBUF_TID_WIDTH-1:0] tid;
      int 			   line_i;
      logic [41:0] 		   base_addr;
      logic [15:0] 		   base_mdata;
      ccip_vc_t                   base_vc;
      ccip_len_t                  base_len;
      int 			   loop_max;
      begin
	 // unroll_active  = 0;
	 ptr            = find_next_pop_slot();
	 latbuf_pop_ptr = ptr;
	 if (ptr != LATBUF_SLOT_INVALID) begin
	    // unroll_active = 1;
	    base_hdr      = records[ptr].hdr[0];
	    base_addr     = records[ptr].hdr[0].addr;
	    base_mdata    = records[ptr].hdr[0].mdata;
	    base_vc       = records[ptr].hdr[0].vc;
	    base_len      = records[ptr].hdr[0].len;
	    loop_max      = records[ptr].num_items + 1;
	    // --------------------------------------------------------- //
  // VL0 request (broken out by infifo_to_vc)
	    // --------------------------------------------------------- //
	    if (isVL0Request(base_hdr)) begin
	       txhdr         = records[ptr].hdr[0];
	       tid           = records[ptr].tid[0];
	       data          = records[ptr].data[0];
	       // --------------- RxHdr ------------------ //
	       rxhdr         = RxHdr_t'(0);
	       rxhdr.vc_used = txhdr.vc;
	       rxhdr.hitmiss = 0;
	       rxhdr.mdata   = base_mdata;
	       rxhdr.clnum   = txhdr.len;
	       rxhdr.format  = 0;
	       rxhdr.resptype = ASE_RD_RSP;
	       array.push_back({ tid, data, logic_cast_RxHdr_t'(rxhdr), logic_cast_TxHdr_t'(txhdr) });
 `ifdef ASE_DEBUG
	       $fwrite(log_fd, "%d | record[%02d][0] size %1d with tid=%x unrolled %s %s \n", $time, ptr, loop_max, tid, return_txhdr(txhdr), return_rxhdr(rxhdr) );
 `endif
	    end
	    // ---------------------------------------------- //
	    // VHx MCL Read Request, unroll the request
	    // ---------------------------------------------- //
	    else if (isVHxRequest(base_hdr)) begin // && isReadRequest(base_hdr)) begin
	       txhdr            = base_hdr;
	       tid              = records[ptr].tid[0];
	       data             = {CCIP_DATA_WIDTH{1'b0}};
	       for(int jj = 0; jj <= txhdr.len; jj = jj + 1) begin
		  txhdr.addr       = base_addr + jj;
		  // --------------- RxHdr ------------------ //
		  rxhdr            = RxHdr_t'(0);
		  rxhdr.hitmiss    = 0;
		  rxhdr.mdata      = base_mdata;
		  rxhdr.clnum      = ccip_len_t'(jj);
		  rxhdr.vc_used    = base_vc;
		  rxhdr.resptype   = ASE_RD_RSP;
		  array.push_back({ tid, data, logic_cast_RxHdr_t'(rxhdr), logic_cast_TxHdr_t'(txhdr) });
 `ifdef ASE_DEBUG
	    	  $fwrite(log_fd, "%d | record[%02d][%02d] size %1d with tid=%x unrolled %s %s \n", $time, ptr, jj, loop_max, tid, return_txhdr(txhdr), return_rxhdr(rxhdr) );
 `endif
	       end // for (int jj = 0; jj <= txhdr.len; jj = jj + 1)
	    end // if (isVHxRequest(base_hdr))
	    // ----------------------------------------------------- //
	    // Pop record and deactivate unroll
	    // ----------------------------------------------------- //
	    // unroll_active = 0;
	    records[ptr].record_pop = 1;
	    @(posedge clk);
	 end // if (ptr != LATBUF_SLOT_INVALID)
      end
   endtask


   // Write channel latbuf -> outfifo
   task automatic WRITE_get_latbuf_unroll_put_outfifo(ref logic [OUTFIFO_WIDTH-1:0] array[$:VISIBLE_DEPTH-1] );
      TxHdr_t                     base_hdr;
      logic [CCIP_DATA_WIDTH-1:0] data;
      int 			  ptr;
      TxHdr_t                     txhdr;
      RxHdr_t                     rxhdr;
      logic [LATBUF_TID_WIDTH-1:0] tid;
      int 			   line_i;
      logic [41:0] 		   base_addr;
      logic [15:0] 		   base_mdata;
      ccip_vc_t                   base_vc;
      ccip_len_t                  base_len;
      int 			   loop_max;
      begin
	 // unroll_active  = 0;
	 ptr            = find_next_pop_slot();
	 latbuf_pop_ptr = ptr;
	 if (ptr != LATBUF_SLOT_INVALID) begin
	    // unroll_active = 1;
	    base_hdr      = records[ptr].hdr[0];
	    base_addr     = records[ptr].hdr[0].addr;
	    base_mdata    = records[ptr].hdr[0].mdata;
	    base_vc       = records[ptr].hdr[0].vc;
	    base_len      = records[ptr].hdr[0].len;
	    loop_max      = records[ptr].num_items + 1;
	    // --------------------------------------------------------- //
  // VL0 request (broken out by infifo_to_vc)
	    // --------------------------------------------------------- //
	    if (isVL0Request(base_hdr)) begin
	       txhdr         = records[ptr].hdr[0];
	       tid           = records[ptr].tid[0];
	       data          = records[ptr].data[0];
	       // --------------- RxHdr ------------------ //
	       rxhdr         = RxHdr_t'(0);
	       rxhdr.vc_used = txhdr.vc;
	       rxhdr.hitmiss = 0;
	       rxhdr.mdata   = base_mdata;
	       rxhdr.clnum   = txhdr.len;
	       rxhdr.format  = 0;

	       rxhdr.resptype = ASE_WR_RSP;
 `ifdef ASE_ENABLE_INTR_FEATURE
	       if (isIntrRequest(base_hdr)) begin
		  rxhdr.resptype = ASE_INTR_RSP;
	       end
 `endif

	       array.push_back({ tid, data, logic_cast_RxHdr_t'(rxhdr), logic_cast_TxHdr_t'(txhdr) });
 `ifdef ASE_DEBUG
	       $fwrite(log_fd, "%d | record[%02d][0] size %1d with tid=%x unrolled %s %s \n", $time, ptr, loop_max, tid, return_txhdr(txhdr), return_rxhdr(rxhdr) );
 `endif
	    end
	    // ---------------------------------------------- //
	    // VHx Write request
	    // ---------------------------------------------- //
	    else if (isVHxRequest(base_hdr)) begin
	       for (int rec_i = 0; rec_i < loop_max ; rec_i = rec_i + 1) begin
		  txhdr            = records[ptr].hdr[rec_i];
		  tid              = records[ptr].tid[rec_i];
		  data             = records[ptr].data[rec_i];
		  // --------------- RxHdr ------------------ //
		  rxhdr            = RxHdr_t'(0);
		  rxhdr.clnum      = base_len;
		  rxhdr.mdata      = base_mdata;
		  rxhdr.vc_used    = base_vc;
		  rxhdr.format     = 1;

		  rxhdr.resptype = ASE_WR_RSP;
 `ifdef ASE_ENABLE_INTR_FEATURE
		  if (isIntrRequest(base_hdr)) begin
		     rxhdr.resptype = ASE_INTR_RSP;
		  end
 `endif

		  array.push_back({ tid, data, logic_cast_RxHdr_t'(rxhdr), logic_cast_TxHdr_t'(txhdr) });
 `ifdef ASE_DEBUG
	    	  $fwrite(log_fd, "%d | record[%02d][0] size %1d with tid=%x unrolled %s %s \n", $time, ptr, loop_max, tid, return_txhdr(txhdr), return_rxhdr(rxhdr) );
 `endif
	       end
	    end
	    // ----------------------------------------------------- //
	    // Pop record and deactivate unroll
	    // ----------------------------------------------------- //
	    // unroll_active = 0;
	    records[ptr].record_pop = 1;
	    @(posedge clk);
	 end // if (ptr != LATBUF_SLOT_INVALID)
      end
   endtask


   // Wrfence response monitor
   always @(posedge clk) begin : wrfence_rsp_monitor
      if (rst) begin
	 wrfence_rspvalid <= 0;
      end
      else if (~wrfence_rspvalid & (wrfence_rsp_cnt != 0)) begin
	 wrfence_rspvalid <= 1;
	 {wrfence_rsptid, wrfence_rsphdr, wrfence_reqhdr } = wrfence_rsp_array.pop_front();
      end
      else if (wrfence_rspvalid & (vl0_wrfence_deassert|vh0_wrfence_deassert|vh1_wrfence_deassert) ) begin
	 wrfence_rspvalid <= 0;
      end
   end

   logic [2:0] latbuf_pop_proc_status;


   /*
    * Latbuf -> outfifo process
    */
   generate
      // ====================================================================== //
      // READ CHANNEL
      // ====================================================================== //
      if (WRITE_CHANNEL == 0) begin
	 always @(posedge clk) begin : READ_latbuf_pop_proc
	    if (rst) begin
	       vl0_wrfence_deassert <= 0;
	       vh0_wrfence_deassert <= 0;
	       vh1_wrfence_deassert <= 0;
	       latbuf_pop_proc_status	<= 3'b000;
	       glbl_wrfence_pop_status <= 0;
	       // unroll_active        <= 0;
	    end
	    // empty outfifo on normal transactions
	    else if (~outfifo_almfull && ~latbuf_empty ) begin
	       READ_get_latbuf_unroll_put_outfifo(outfifo);
	       latbuf_pop_proc_status	<= 3'b110;
	    end
	    // Else
	    else begin
	       latbuf_pop_proc_status  <= 3'b000;
	    end
	    // ------------------------------------------------------------------- //-
	    // Book keeping
	    // -------------------------------------------------------------------- //
	    for(int ready_i = 0; ready_i < NUM_WAIT_STATIONS ; ready_i = ready_i + 1) begin
	       if (rst) begin
		  records[ready_i].record_pop <= 0;
	       end
	       else if ( (records[ready_i].state == LatSc_RecordPopped) ||
			 (records[ready_i].state == LatSc_Disabled) ) begin
		  records[ready_i].record_pop <= 0;
	       end
	    end
	 end
      end
      // ====================================================================== //
      // Write CHANNEL
      // ====================================================================== //
      else if (WRITE_CHANNEL == 1) begin
	 always @(posedge clk) begin : WRITE_latbuf_pop_proc
	    if (rst) begin
	       vl0_wrfence_deassert <= 0;
	       vh0_wrfence_deassert <= 0;
	       vh1_wrfence_deassert <= 0;
	       latbuf_pop_proc_status	<= 3'b000;
	       glbl_wrfence_pop_status <= 0;
	       // unroll_active        <= 0;
	    end
	    // empty outfifo on normal transactions
	    else if (~outfifo_almfull && ~latbuf_empty ) begin
	       WRITE_get_latbuf_unroll_put_outfifo(outfifo);
	       vl0_wrfence_deassert <= 0;
	       vh0_wrfence_deassert <= 0;
	       vh1_wrfence_deassert <= 0;
	       latbuf_pop_proc_status	<= 3'b110;
	       glbl_wrfence_pop_status <= 0;
	    end
	    // Pop write fence
	    else if (wrfence_rspvalid && (vl0_wrfence_flag|vh0_wrfence_flag|vh1_wrfence_flag) && ~glbl_wrfence_pop_status) begin
	       case (wrfence_rsphdr.vc_used)
		 VC_VA :
		   begin
		      if ( (wrfence_rsptid == vl0_wrfence_tid) &&
			   (wrfence_rsptid == vh0_wrfence_tid) &&
			   (wrfence_rsptid == vh1_wrfence_tid) &&
			   vl0_wrfence_flag &&
			   vh0_wrfence_flag &&
			   vh1_wrfence_flag ) begin
			 vl0_wrfence_deassert <= 1;
			 vh0_wrfence_deassert <= 1;
			 vh1_wrfence_deassert <= 1;
			 latbuf_pop_proc_status	<= 3'b100;
			 glbl_wrfence_pop_status <= 1;
			 outfifo.push_back({wrfence_rsptid, {CCIP_DATA_WIDTH{1'b0}}, logic_cast_RxHdr_t'(wrfence_rsphdr), logic_cast_TxHdr_t'(wrfence_reqhdr) });
		      end
		   end

		 VC_VL0:
		   begin
		      if ((wrfence_rsptid == vl0_wrfence_tid) && vl0_wrfence_flag) begin
			 vl0_wrfence_deassert <= 1;
			 vh0_wrfence_deassert <= 0;
			 vh1_wrfence_deassert <= 0;
			 latbuf_pop_proc_status	<= 3'b101;
			 glbl_wrfence_pop_status <= 1;
			 outfifo.push_back({wrfence_rsptid, {CCIP_DATA_WIDTH{1'b0}}, logic_cast_RxHdr_t'(wrfence_rsphdr), logic_cast_TxHdr_t'(wrfence_reqhdr) });
		      end
		   end

		 VC_VH0:
		   begin
		      if ((wrfence_rsptid == vh0_wrfence_tid) && vh0_wrfence_flag ) begin
			 vl0_wrfence_deassert <= 0;
			 vh0_wrfence_deassert <= 1;
			 vh1_wrfence_deassert <= 0;
			 latbuf_pop_proc_status	<= 3'b110;
			 glbl_wrfence_pop_status <= 1;
			 outfifo.push_back({wrfence_rsptid, {CCIP_DATA_WIDTH{1'b0}}, logic_cast_RxHdr_t'(wrfence_rsphdr), logic_cast_TxHdr_t'(wrfence_reqhdr) });
		      end
		   end

		 VC_VH1:
		   begin
		      if ((wrfence_rsptid == vh1_wrfence_tid) && vh1_wrfence_flag ) begin
			 vl0_wrfence_deassert <= 0;
			 vh0_wrfence_deassert <= 0;
			 vh1_wrfence_deassert <= 1;
			 latbuf_pop_proc_status	<= 3'b111;
			 glbl_wrfence_pop_status <= 1;
			 outfifo.push_back({wrfence_rsptid, {CCIP_DATA_WIDTH{1'b0}}, logic_cast_RxHdr_t'(wrfence_rsphdr), logic_cast_TxHdr_t'(wrfence_reqhdr) });
		      end
		   end
	       endcase
	    end
	    else begin
	       vl0_wrfence_deassert <= 0;
	       vh0_wrfence_deassert <= 0;
	       vh1_wrfence_deassert <= 0;
	       latbuf_pop_proc_status	<= 3'b000;
	       glbl_wrfence_pop_status <= 0;
	       // unroll_active           <= 0;
	    end
	    // ------------------------------------------------------------------- //-
	    // Book keeping
	    // -------------------------------------------------------------------- //
	    for(int ready_i = 0; ready_i < NUM_WAIT_STATIONS ; ready_i = ready_i + 1) begin
	       if (rst) begin
		  records[ready_i].record_pop <= 0;
	       end
	       else if ( (records[ready_i].state == LatSc_RecordPopped) ||
			 (records[ready_i].state == LatSc_Disabled) ) begin
		  records[ready_i].record_pop <= 0;
	       end
	    end
	 end // block: latbuf_pop_proc
	 // ====================================================================== //
      end
   endgenerate

   // Outfifo Full/Empty
   assign outfifo_almfull  = (outfifo_cnt > VISIBLE_FULL_THRESH) ? 1 : 0;

   always @(*) begin
      if (outfifo_cnt == 0)
	outfifo_empty <= 1;
      else
	outfifo_empty <= 0;
   end

   assign empty = outfifo_empty;

   assign txhdr_out = TxHdr_t'(txhdr_out_vec);
   assign rxhdr_out = RxHdr_t'(rxhdr_out_vec);


   //////////////////////////////////////////////////////////////////////
   // Read guard
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


   /*
    * Hazard-OUT interface assignment
    */
   generate
      // -------------------------------------- //
      // Read channel configuration
      // -------------------------------------- //
      if (WRITE_CHANNEL == 0) begin
	 always @(posedge clk) begin
	    if (valid_out) begin
	       hazpkt_out.valid <= valid_out;
	       hazpkt_out.hdr   <= txhdr_out;
	       hazpkt_out.tid   <= tid_out;
	    end
	    else begin
	       hazpkt_out.valid <= 0;
	    end
	 end
      end
      // -------------------------------------- //
      // Write channel configuration
      // -------------------------------------- //
      else if (WRITE_CHANNEL == 1) begin
	 always @(posedge clk) begin
	    if (valid_out && isWriteRequest(txhdr_out)) begin
	       hazpkt_out.valid <= valid_out;
	       hazpkt_out.hdr   <= txhdr_out;
	       hazpkt_out.tid   <= tid_out;
	    end
	    else begin
	       hazpkt_out.valid <= 0;
	    end
	 end
      end
   endgenerate


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

endmodule // outoforder_wrf_channel
