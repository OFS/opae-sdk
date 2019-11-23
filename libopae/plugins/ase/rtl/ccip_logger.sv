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
 * Module Info: CCI Transactions Logger
 * Language   : System{Verilog}
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 */

import ase_pkg::*;
import ccip_if_pkg::*;

`include "platform.vh"

module ccip_logger
  #(
    parameter LOGNAME   = "CHANGE_MY_NAME.log"
    )
   (
    // Configure enable
    input logic finish_logger,
    input logic stdout_en,
    // Buffer message injection
    input logic log_timestamp_en,
    input logic log_string_en,
    ref string 	log_string,
    //////////////////////////////////////////////////////////
    // CCI interface
    input logic clk,
    input logic SoftReset,
    input 	t_if_ccip_Rx ccip_rx,
    input 	t_if_ccip_Tx ccip_tx
    );


   /*
    * ASE Hardware Interface (CCI) logger
    * - Logs CCI transaction into a transactions.tsv file
    * - Watch for "*valid", and write transaction to log name
    */
   // Log file descriptor
   int       log_fd;

   // Reset management
   logic     SoftReset_q;

   // AlmostFull management
   logic     C0TxAlmFull_q;
   logic     C1TxAlmFull_q;

   // Registers for comparing previous states
   always @(posedge clk) begin
      SoftReset_q       <= SoftReset;
      C0TxAlmFull_q     <= ccip_rx.c0TxAlmFull;
      C1TxAlmFull_q     <= ccip_rx.c1TxAlmFull;
   end

   // Config header
   t_ccip_c0_ReqMmioHdr C0RxMmioHdr;
   assign C0RxMmioHdr = t_ccip_c0_ReqMmioHdr'(ccip_rx.c0.hdr);

   // Umsg header
`ifdef ASE_ENABLE_UMSG_FEATURE
   UMsgHdr_t C0RxUMsgHdr;
   assign C0RxUMsgHdr = UMsgHdr_t'(ccip_rx.c0.hdr);
`endif

   // Intr header (cast to ccip_if_pkg types)
`ifdef ASE_ENABLE_INTR_FEATURE
   t_ccip_c1_ReqIntrHdr C1TxIntrReqHdr;
   assign C1TxIntrReqHdr = t_ccip_c1_ReqIntrHdr'(ccip_tx.c1.hdr);

   t_ccip_c1_RspIntrHdr C1RxIntrRspHdr;
   assign C1RxIntrRspHdr = t_ccip_c1_RspIntrHdr'(ccip_rx.c1.hdr);

`endif


   /*
    * Buffer channels, request and response types
    */
   // Print Channel function
   function string print_channel (t_ccip_vc vc_sel);
      begin
   	 case (vc_sel)
   	   eVC_VA  : return "VA ";
   	   eVC_VL0 : return "VL0";
   	   eVC_VH0 : return "VH0";
   	   eVC_VH1 : return "VH1";
   	 endcase
      end
   endfunction

   // Print Req Type - CH0
   function string print_c0_reqtype (t_ccip_c0_ReqMemHdr req);
      begin
	 case (req)
	   eREQ_RDLINE_S   : return "Rd_S       ";
	   eREQ_RDLINE_I   : return "Rd_I       ";
	   default        : return "** ERROR %m : eREQ-CH0 unindentified **" ;
	 endcase
      end
   endfunction

   // Print req type - CH1
   function string print_c1_reqtype (t_ccip_c1_ReqMemHdr req);
      begin
	 case (req)
	   eREQ_WRLINE_I   : return "Wr_I       ";
	   eREQ_WRLINE_M   : return "Wr_M       ";
	   eREQ_WRPUSH_I   : return "WrPush_I   ";
	   eREQ_WRFENCE    : return "WrFence    ";
	   eREQ_INTR       : return "IntrReq    ";
	   default         : return "** ERROR %m : eREQ-CH1 unindentified **" ;
	 endcase
      end
   endfunction

   // Print CH0 response type
   function string print_c0_resptype (t_ccip_c0_rsp resp);
      begin
	 case (resp)
	   eRSP_RDLINE     : return "RdResp     ";
	   default         : return "** ERROR %m : eRSP-CH0 unindentified **" ;
	 endcase
      end
   endfunction

   // Print CH1 response type
   function string print_c1_resptype (t_ccip_c1_rsp resp);
      begin
	 case (resp)
	   eRSP_WRLINE     : return "WrResp     ";
	   eRSP_WRFENCE    : return "WrFenceResp";
	   eRSP_INTR       : return "IntrResp   ";
	   default         : return "** ERROR %m : eRSP-CH1 unindentified **" ;
	 endcase
      end
   endfunction


   // Print CL number (in Request)
   function string print_cllen (t_ccip_clLen len);
      begin
	 case (len)
	   eCL_LEN_1 : return "#1CL";
	   eCL_LEN_2 : return "#2CL";
	   eCL_LEN_4 : return "#4CL";
	   default : return "** ERROR %m : clLen unindentified **" ;
	 endcase
      end
   endfunction


   // Print CL number (in Response)
   function string print_clnum (t_ccip_clNum num);
      begin
	 case (num)
	   2'b00 : return "#1CL";
	   2'b01 : return "#2CL";
	   2'b10 : return "#3CL";
	   2'b11 : return "#4CL";
	 endcase
      end
   endfunction


   // Print CSR data
   function string csr_data(int num_bytes, logic [CCIP_DATA_WIDTH-1:0] rx0_data);
      string str;
      begin
	 case (num_bytes)
	   4 :
	     begin
		$sformat(str, "%08h", rx0_data[31:0]);
	     end
	   8 :
	     begin
		$sformat(str, "%016h", rx0_data[63:0]);
	     end
	   64 :
	     begin
		$sformat(str, "%0128h", rx0_data[511:0]);
	     end
	 endcase
      return str;
      end
   endfunction


   // MMIO Request length
   function int mmioreq_length (logic [1:0] mmio_len);
      begin
	 case (mmio_len)
	   2'b00 : return 4;
	   2'b01 : return 8;
	   2'b10 : return 64;
	 endcase
      end
   endfunction // mmioreq_length


   // Space generator - formatting help
   function string ret_spaces (int num);
      string spaces;
      int    ii;
      begin
	 spaces = "";
	 for (ii = 0; ii < num; ii = ii + 1) begin
	    spaces = {spaces, " "};
	 end
	 return spaces;
      end
   endfunction


   /*
    * FUNCTION: print_and_post_log wrapper function to simplify logging
    */
   function void print_and_post_log(string formatted_string);
      begin
	 if (stdout_en)
	   $display(formatted_string);
	 $fwrite(log_fd, formatted_string);
	 $fflush();
      end
   endfunction // print_and_post_log

   // Placeholder strings
   string softreset_str;
   string c0TxAlmFull_str;
   string c1TxAlmFull_str;
   string c0rx_str;
   string c1rx_str;
   string c0tx_str;
   string c1tx_str;
   string c1tx_byte_en_str;
   string c2tx_str;


   /*
    * Watcher process
    */
   initial begin : logger_proc
      // Display
      $display("  [SIM]  Transaction Logger started");

      // Open transactions.tsv file
      log_fd = $fopen(LOGNAME, "w");

      // Watch CCI port
      forever begin
	 // -------------------------------------------------- //
	 // Indicate Software controlled reset
	 // -------------------------------------------------- //
	 if (SoftReset_q != SoftReset) begin
	    $sformat(softreset_str,
		     "%d\tSoftReset toggled from %b to %b\n",
		     $time,
		     SoftReset_q,
		     SoftReset);
	    print_and_post_log(softreset_str);
	 end
	 // -------------------------------------------------- //
	 // Track C0TxAlmFull transitions
	 // -------------------------------------------------- //
	 if (C0TxAlmFull_q != ccip_rx.c0TxAlmFull) begin
	    $sformat(c0TxAlmFull_str,
		     "%d\tC0Tx AlmFull toggled from %b to %b\n",
		     $time,
		     C0TxAlmFull_q,
		     ccip_rx.c0TxAlmFull);
	    print_and_post_log(c0TxAlmFull_str);
	 end
	 // -------------------------------------------------- //
	 // Track C1TxAlmFull transitions
	 // -------------------------------------------------- //
	 if (C1TxAlmFull_q != ccip_rx.c1TxAlmFull) begin
	    $sformat(c1TxAlmFull_str,
		     "%d\tC1Tx AlmFull toggled from %b to %b\n",
		     $time,
		     C1TxAlmFull_q,
		     ccip_rx.c1TxAlmFull);
	    print_and_post_log(c1TxAlmFull_str);
	 end
	 // -------------------------------------------------- //
	 // Buffer messages
	 // -------------------------------------------------- //
	 if (log_string_en) begin
	    if (log_timestamp_en) begin
	       $fwrite(log_fd, "-----------------------------------------------------\n");
	       $fwrite(log_fd, "%d\t%s\n", $time, log_string);
	    end
	    else begin
	       $fwrite(log_fd, "-----------------------------------------------------\n");
	       $fwrite(log_fd, "%s\n", log_string);
	    end
	 end
	 // -------------------------------------------------- //
	 // C0Rx Channel activity
	 // -------------------------------------------------- //
	 // MMIO Write Request
	 if (ccip_rx.c0.mmioWrValid) begin
	    $sformat(c0rx_str,
		     "%d\t   \tMMIOWrReq   \t  \t%x\t%d bytes\t%s\n",
		     $time,
		     C0RxMmioHdr.address,
		     mmioreq_length(C0RxMmioHdr.length),
		     csr_data(mmioreq_length(C0RxMmioHdr.length), ccip_rx.c0.data) );
	    print_and_post_log(c0rx_str);
	 end
	 // MMIO Read Request
	 else if (ccip_rx.c0.mmioRdValid) begin
	    $sformat(c0rx_str,
		     "%d\t   \tMMIORdReq   \t%x\t%x\t%d bytes\n",
	    	     $time,
	    	     C0RxMmioHdr.tid,
	    	     C0RxMmioHdr.address,
	    	     mmioreq_length(C0RxMmioHdr.length));
	    print_and_post_log(c0rx_str);
	 end // if (ccip_rx.c0.mmioRdValid)
	 // Read Response
	 else if (ccip_rx.c0.rspValid && isCCIPRdLineResponse(ccip_rx.c0.hdr.resp_type)) begin
	    $sformat(c0rx_str,
		     "%d\t%s\t%s\t%x\t%s\t%x\n",
	 	     $time,
	 	     print_channel(ccip_rx.c0.hdr.vc_used),
	 	     print_c0_resptype(ccip_rx.c0.hdr.resp_type),
	 	     ccip_rx.c0.hdr.mdata,
		     print_clnum(ccip_rx.c0.hdr.cl_num),
	 	     ccip_rx.c0.data);
	    print_and_post_log(c0rx_str);
	 end // if (ccip_tx.c0.rspValid && (ccip_rx.c0.hdr.resptype == eRSP_RDLINE))
	 /*************** SW -> MEM -> AFU Unordered Message  *************/
`ifdef ASE_ENABLE_UMSG_FEATURE
	 else if (ccip_rx.c0.rspValid && isCCIPUmsgResponse(ccip_rx.c0.hdr.resp_type)) begin
	    if (C0RxUMsgHdr.umsg_type) begin
	       $sformat(c0rx_str,
			"%d\t   \tUMsgHint   \t%d\n",
			$time,
			C0RxUMsgHdr.umsg_id);
	       print_and_post_log(c0rx_str);
	    end
	    else if (~C0RxUMsgHdr.umsg_type) begin
	       $sformat(c0rx_str,
			"%d\t   \tUMsgData   \t%d\t%x\n",
			$time,
			C0RxUMsgHdr.umsg_id,
			ccip_rx.c0.data);
	       print_and_post_log(c0rx_str);
	    end
	 end
`endif
	 // -------------------------------------------------- //
	 // C1Rx Channel activity
	 // -------------------------------------------------- //
	 // Write response
	 if (ccip_rx.c1.rspValid && isCCIPWrLineResponse(ccip_rx.c1.hdr.resp_type)) begin
	    $sformat(c1rx_str,
		     "%d\t%s\t%s\t%x\t%s\n",
	 	     $time,
	 	     print_channel(ccip_rx.c1.hdr.vc_used),
	 	     print_c1_resptype(ccip_rx.c1.hdr.resp_type),
	 	     ccip_rx.c1.hdr.mdata,
	 	     print_clnum(ccip_rx.c1.hdr.cl_num));
	    print_and_post_log(c1rx_str);
	 end
	 // Write Fence Response
	 else if (ccip_rx.c1.rspValid && isCCIPWrFenceResponse(ccip_rx.c1.hdr.resp_type)) begin
	    $sformat(c1rx_str,
		     "%d\t%s\tWrFenceRsp\t%x\n",
	 	     $time,
	 	     print_channel(ccip_rx.c1.hdr.vc_used),
	 	     ccip_rx.c1.hdr.mdata);
	    print_and_post_log(c1rx_str);
	 end
`ifdef ASE_ENABLE_INTR_FEATURE
	 else if (ccip_rx.c1.rspValid && isCCIPInterruptResponse(ccip_rx.c1.hdr.resp_type)) begin
	    $sformat(c1rx_str,
		     "%d\tInterrupt response on ID = %d\n",
		     $time,
		     C1RxIntrRspHdr.id);
	    print_and_post_log(c1rx_str);
	 end
`endif
	 // -------------------------------------------------- //
	 // C0Tx Channel activity
	 // -------------------------------------------------- //
	 // AFU -> MEM Read Request
	 if (ccip_tx.c0.valid && isCCIPRdLineRequest(ccip_tx.c0.hdr.req_type) ) begin
	    $sformat(c0tx_str,
		     "%d\t%s\t%s\t%x\t%x\t%s\n",
	 	     $time,
	 	     print_channel(ccip_tx.c0.hdr.vc_sel),
	 	     print_c0_reqtype(ccip_tx.c0.hdr.req_type),
	 	     ccip_tx.c0.hdr.mdata,
	 	     ccip_tx.c0.hdr.address,
		     print_cllen(ccip_tx.c0.hdr.cl_len));
	    print_and_post_log(c0tx_str);
	 end
	 // -------------------------------------------------- //
	 // C1Tx Channel activity
	 // -------------------------------------------------- //
	 // Write Request
	 if (ccip_tx.c1.valid && isCCIPWrLineRequest(ccip_tx.c1.hdr.req_type)) begin
	    // Partial line write with byte range?
	    c1tx_byte_en_str = "";
	    if (ccip_tx.c1.hdr.mode == eMOD_BYTE) begin
	       $sformat(c1tx_byte_en_str, " PW [start %0d, len %0d]",
		      ccip_tx.c1.hdr.byte_start, ccip_tx.c1.hdr.byte_len);
	    end

	    $sformat(c1tx_str,
		     "%d\t%s\t%s\t%x\t%x\t%x%s\t%s\n",
	 	     $time,
	 	     print_channel(ccip_tx.c1.hdr.vc_sel),
	 	     print_c1_reqtype(ccip_tx.c1.hdr.req_type),
	 	     ccip_tx.c1.hdr.mdata,
	 	     ccip_tx.c1.hdr.address,
	 	     ccip_tx.c1.data,
	 	     c1tx_byte_en_str,
		     print_clnum(ccip_tx.c1.hdr.cl_len));
	    print_and_post_log(c1tx_str);
	 end // if (ccip_tx.c1.valid && (ccip_tx.c1.hdr.req_type != eREQ_WRFENCE))
	 // Write Fence
	 else if (ccip_tx.c1.valid && isCCIPWrFenceRequest(ccip_tx.c1.hdr.req_type)) begin
	    $sformat(c1tx_str,
		     "%d\t%s\tWrFence \t%x\n",
		     $time,
		     print_channel(ccip_tx.c1.hdr.vc_sel),
		     ccip_tx.c1.hdr.mdata);
	    print_and_post_log(c1tx_str);
	 end
`ifdef ASE_ENABLE_INTR_FEATURE
	 else if (ccip_tx.c1.valid && isCCIPInterruptRequest(ccip_tx.c1.hdr.req_type)) begin
	    $sformat(c1tx_str,
		     "%d\tInterrupt Requested with ID = %d\n",
		     $time,
		     C1TxIntrReqHdr.id
		     );
	    print_and_post_log(c1tx_str);
	 end
`endif
	 // -------------------------------------------------- //
	 // C2Tx Channel activity
	 // -------------------------------------------------- //
	 if (ccip_tx.c2.mmioRdValid) begin
	    $sformat(c2tx_str,
		     "%d\t   \tMMIORdRsp   \t%x\t%x\n",
		     $time,
		     ccip_tx.c2.hdr.tid,
		     ccip_tx.c2.data);
	    print_and_post_log(c2tx_str);
	 end
	 // -------------------------------------------------- //
	 // FINISH command
	 // -------------------------------------------------- //
	 if (finish_logger == 1) begin
	    $fclose(log_fd);
	 end
	 // -------------------------------------------------- //
	 // Wait till next clock
	 // -------------------------------------------------- //
	 $fflush(log_fd);
	 @(posedge clk);
      end
   end

endmodule
