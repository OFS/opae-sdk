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
 * Module Info: CCI Sniffer (Rules checker, Hazard detector and warning)
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * Description:
 * ************
 * XZ checker:
 * Checks the TX signals in AFU to see if 'X' or 'z' is validated. ASE
 * does not validated 'X', 'z' on RX channels, these will not be
 * checked. When a violation is discovered, it will be printed in a
 * message,
 *
 * Hazard sniffer:
 * In the strictest sense, if there are multiple transactions to a
 * certain cache line
 *
 * All warnings are logged in ccip_warning_and_errors.txt
 */

import ase_pkg::*;
import ccip_if_pkg::*;
`include "platform.vh"

// `define STANDALONE_DEBUG

module ccip_checker
  #(
    parameter ERR_LOGNAME  = "ccip_warning_and_errors.txt"
    )
   (
    // Configure enable
    input logic               finish_logger,
    input logic               init_sniffer,
    input logic               ase_reset,
    // -------------------------------------------------------- //
    //      Channel overflow/realfull checks
    input logic               cf2as_ch0_realfull,
    input logic               cf2as_ch1_realfull,
    // -------------------------------------------------------- //
    //          Hazard/Indicator Signals                        //
    input                     ase_haz_if haz_if,
    output logic [SNIFF_VECTOR_WIDTH-1:0] error_code,
    // -------------------------------------------------------- //
    //              CCI-P interface                             //
    input logic               clk,
    input logic               SoftReset,
    input                     t_if_ccip_Rx ccip_rx,
    input                     t_if_ccip_Tx ccip_tx
    );
 logic [SNIFF_VECTOR_WIDTH-1:0] error_code_q;

   /*
    * Function Request type checker
    */
   // isCCIPReadRequest
   function automatic logic isCCIPReadRequest(t_ccip_c0_ReqMemHdr hdr);
      begin
     if (hdr.req_type inside {eREQ_RDLINE_S, eREQ_RDLINE_I}) begin
        return 1;
     end
     else begin
        return 0;
     end
      end
   endfunction // isCCIPReadRequest


   // isCCIPWriteRequest
   function automatic logic isCCIPWriteRequest(t_ccip_c1_ReqMemHdr hdr);
      begin
     if (hdr.req_type inside {eREQ_WRLINE_M, eREQ_WRLINE_I, eREQ_WRPUSH_I}) begin
        return 1;
     end
     else begin
        return 0;
     end
      end
   endfunction // isCCIPWriteRequest


   // isCCIPWrFenceRequest
   function automatic logic isCCIPWrFenceRequest(t_ccip_c1_ReqMemHdr hdr);
      begin
     if (ccip_tx.c1.hdr.req_type == eREQ_WRFENCE) begin
        return 1;
     end
     else begin
        return 0;
     end
      end
   endfunction // isCCIPWrFenceRequest


   // isEqualsXorZ
   function automatic logic isEqualsXorZ(reg inp);
      begin
     if ((inp === 1'bX)||(inp === 1'bx)) begin
        return 1;
     end
     else begin
        return 0;
     end
      end
   endfunction // isEqualsXorZ


   /*
    * File descriptors, codes etc
    */
   int                          fd_errlog;
   logic                      logfile_created;
   logic                      init_sniffer_q;

   // Watch init_sniffer
   always @(posedge clk) begin
      init_sniffer_q <= init_sniffer;
   end

   // Initialize sniffer
   always @(posedge clk) begin

      // Indicate logfile created
      if (init_sniffer) begin
        logfile_created <= 0;
        error_code_q[SNIFF_NO_ERROR]=1'b0;
     end
     else
        decode_error_code(0, SNIFF_NO_ERROR);

     // Print that checker is running
     if (~init_sniffer && init_sniffer_q) begin
        $display ("  [SIM]  Protocol Checker initialized");
     end
   end

   // FD open
   function automatic void open_logfile();
   begin
     fd_errlog = $fopen(ERR_LOGNAME, "w");
     logfile_created = 1;
   end
   endfunction

   // Watch logfile and close until finished
   initial begin
      // Wait until logfile exists
      wait (logfile_created == 1);

      // Wait until finish logger
      wait (finish_logger == 1);

      // Close loggers
      $display ("  [SIM]  Closing Protocol checker");
      $fclose(fd_errlog);
   end


   /*
    * Controlled kill of simulation
    */
   logic simkill_en = 0;
   int      simkill_cnt;

   // Print and simkill
   function void print_and_simkill();
   begin
     `BEGIN_RED_FONTCOLOR;
     $display(" [ERROR] %d : Simulation will end now", $time);
     `END_RED_FONTCOLOR;
     $fwrite(fd_errlog, " [ERROR] %d : Simulation will end now\n", $time);
     simkill_en = 1;
   end
   endfunction

   // Enumeration of simulation states
   typedef enum {
         SimkillIdle,
         SimkillCountdown,
         SimkillNow
         } SimkillEnumStates;
   SimkillEnumStates simkill_state;

   // Simkill countdown and issue simkill
   always @(posedge clk) begin
      if (ase_reset) begin
         simkill_cnt <= 20;
         simkill_state <= SimkillIdle;
         end
         else begin
         case (simkill_state)

           SimkillIdle:
           begin
             simkill_cnt <= 20;
             if (simkill_en) begin
               simkill_state <= SimkillCountdown;
             end
             else begin
               simkill_state <= SimkillIdle;
             end
           end

           SimkillCountdown:
           begin
             simkill_cnt <= simkill_cnt - 1;
             if (simkill_cnt <= 0) begin
               simkill_state <= SimkillNow;
             end
             else begin
               simkill_state <= SimkillCountdown;
             end
           end

           SimkillNow:
           begin
             simkill_state <= SimkillNow;
`ifndef STANDALONE_DEBUG
             start_simkill_countdown();
`endif
           end

           default:
           begin
             simkill_state <= SimkillIdle;
           end

         endcase
      end
   end

   /*
    * Helper functions
    */
   // Print string and write to file
  function void print_message_and_log(input logic warn_only,
                    input string logstr);
     begin
     // If logfile doesnt exist it, create it
    // always@(error)
     if (logfile_created == 0) begin
        open_logfile();
     end
     // Write log
     if (warn_only == 1) begin
        `BEGIN_RED_FONTCOLOR;
        $display(" [WARN]  %d : %s", $time, logstr);
        `END_RED_FONTCOLOR;
        $fwrite(fd_errlog, " [WARN]  %d : %s\n", $time, logstr);
     end
     else begin
        `BEGIN_RED_FONTCOLOR;
        $display(" [ERROR] %d : %s", $time, logstr);
        `END_RED_FONTCOLOR;
        $fwrite(fd_errlog, " [ERROR] %d : %s\n", $time, logstr);
        print_and_simkill();
     end
     end
   endfunction


   // Trigger Error bit by index
   always@(posedge clk)
   begin
        if (SoftReset)
            error_code=32'b0;
        else
            error_code=error_code_q;
   end

   // Error code enumeration
   task decode_error_code(
              input logic init,
              input       sniff_code_t code
              );
      string                   errcode_str;
      string                   log_str;
      begin
     if (init) begin
        error_code_q[code]=1'b0;
     end
     else begin
            errcode_str = code.name;
        case (code)
          // C0TX - Invalid request type
          SNIFF_C0TX_INVALID_REQTYPE:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr was issued with an invalid reqtype !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - Overflow check
          SNIFF_C0TX_OVERFLOW:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] Overflow detected on CCI-P Channel 0 !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - 2CL address alignment check
          SNIFF_C0TX_ADDRALIGN_2_ERROR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr Multi-line address request is not aligned 2-CL aligned (C0TxHdr.addr[0] != 1'b0) !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - 4CL address alignment check
          SNIFF_C0TX_ADDRALIGN_4_ERROR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr Multi-line address request is not aligned 4-CL aligned (C0TxHdr.addr[1:0] != 2'b00) !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - Reset ignored
          SNIFF_C0TX_RESET_IGNORED_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr was issued when AFU Reset is HIGH !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C0TX - X or Z found [Warning only]
          SNIFF_C0TX_XZ_FOUND_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr request contained a 'Z' or 'X' !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C0TX - 3CL Read Request
          SNIFF_C0TX_3CL_REQUEST:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr 3-CL request issued. This is illegal !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - Address found to be zero
          SNIFF_C0TX_ADDR_ZERO_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C0TxHdr address was ZERO.. this will cause simulation failure", errcode_str);
           print_message_and_log(1, log_str);
        end
              SNIFF_C0TX_UNEXP_ADDR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] Unexpected C0TxHdr address . this will cause simulation failure", errcode_str);
           print_message_and_log(1, log_str);
        end
          // C1TX - Invalid request type
          SNIFF_C1TX_INVALID_REQTYPE:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr was issued with an invalid reqtype !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1TX - Overflow check
          SNIFF_C1TX_OVERFLOW:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] Overflow detected on CCI-P Channel 1 !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1TX - 2CL address alignment check
          SNIFF_C1TX_ADDRALIGN_2_ERROR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr Multi-line address request is not aligned 2-CL aligned (C1TxHdr.addr[0] != 1'b0) !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1TX - 4CL address alignment check
          SNIFF_C1TX_ADDRALIGN_4_ERROR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr Multi-line address request is not aligned 4-CL aligned (C1TxHdr.addr[1:0] != 2'b00) !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1TX - Reset ignored
          SNIFF_C1TX_RESET_IGNORED_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr was issued when AFU Reset is HIGH !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C1TX - X or Z found [Warning only]
          SNIFF_C1TX_XZ_FOUND_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr request contained a 'Z' or 'X' !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C1Tx - Unexpected VC changes
          SNIFF_C1TX_UNEXP_VCSEL:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr VC-selection must not change in between multi-line beat !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C1Tx - Unexpected MDATA changes [Warning only]
          SNIFF_C1TX_UNEXP_MDATA:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr MDATA changed between multi-line beat !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C1Tx - Unexpected address changes
          SNIFF_C1TX_UNEXP_ADDR:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr multi-line beat found unexpected address - addr[1:0] must increment by 1 !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1Tx - Unexpected cl_len change [Warning only]
          SNIFF_C1TX_UNEXP_CLLEN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr cl_len field changed between multi-line beat !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C1Tx - unexpected request type change
          SNIFF_C1TX_UNEXP_REQTYPE:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr multi-line beat found unexpected Request type change !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1Tx - SOP field not set
          SNIFF_C1TX_SOP_NOT_SET:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr First transaction of multi-line beat must set SOP field to HIGH !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1Tx - SOP field set for subsequent transactions
          SNIFF_C1TX_SOP_SET_MCL1TO3:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr Subsequent transaction of multi-line beat must set SOP field to LOW !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C0TX - 3CL Request check
          SNIFF_C1TX_3CL_REQUEST:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr 3-CL request issued. This is illegal !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1Tx - Write fence observered between CL1-CL3 in MCL request
          SNIFF_C1TX_WRFENCE_IN_MCL1TO3:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr cannot issue a WriteFence in between a multi-line transaction !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

         // C1TX - Write Fence observed with SOP set.
           SNIFF_C1TX_WRFENCE_SOP_SET:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr cannot issue a WriteFence with SOP bit Set High !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C1TX - Address found to be zero
          SNIFF_C1TX_ADDR_ZERO_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] C1TxHdr address was ZERO.. this will cause simulation failure", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C2Tx - MMIO Read Response timeout
          MMIO_RDRSP_TIMEOUT:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] MMIO Read Response timed out. AFU must respond to MMIO Read responses within %d clocks !\n", errcode_str, `MMIO_RESPONSE_TIMEOUT);
           print_message_and_log(0, log_str);
        end

          // C2TX - MMIO Read Response was unsolicited
          MMIO_RDRSP_UNSOLICITED:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] ASE detected an unsolicited MMIO Response. In system, this can cause unexpected behavior !\n", errcode_str);
           print_message_and_log(0, log_str);
        end

          // C2TX - X or Z found
          MMIO_RDRSP_XZ_FOUND_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] MMIO Response contained a 'Z' or 'X' !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

          // C2TX - Reset ignored
          MMIO_RDRSP_RESET_IGNORED_WARN:
        begin
           error_code_q[code] = 1'b1;
           $sformat(log_str, "[%s] MMIO Response was issued when SoftReset signal was HIGH !\n", errcode_str);
           print_message_and_log(1, log_str);
        end

         SNIFF_NO_ERROR:
        begin
           error_code_q[code] = 1'b0;
        end
          // Unknown type -- this must not happen
          default:
        begin
           print_message_and_log(1, "** ASE Internal Error **: Undefined Error code found !");
        end

        endcase
     end
      end
   endtask


   /*
    * Reset ignorance check
    */
   always @(posedge clk) begin
      if (SoftReset && ccip_tx.c0.valid) begin
        decode_error_code(0, SNIFF_C0TX_RESET_IGNORED_WARN);
      end
      else
        error_code_q[SNIFF_C0TX_RESET_IGNORED_WARN]=1'b0;

     if (SoftReset && ccip_tx.c1.valid) begin
        decode_error_code(0, SNIFF_C1TX_RESET_IGNORED_WARN);
      end
      else
        error_code_q[SNIFF_C1TX_RESET_IGNORED_WARN]=1'b0;

     if (SoftReset && ccip_tx.c2.mmioRdValid) begin
        decode_error_code(0, MMIO_RDRSP_RESET_IGNORED_WARN);
      end
      else
        error_code_q[MMIO_RDRSP_RESET_IGNORED_WARN]=1'b0;
   end


   /*
    * Cast MMIO Header
    */
   t_ccip_c0_ReqMmioHdr C0RxCfg;
   assign C0RxCfg = t_ccip_c0_ReqMmioHdr'(ccip_rx.c0.hdr);


   /*
    * UNDEF & HIIMP checker
    */
   // Z and X flags
   reg                xz_tx0_flag;
   reg                xz_tx1_flag;
   reg                xz_tx2_flag;

   // XZ flags check
   assign xz_tx0_flag = ^{ccip_tx.c0.hdr.vc_sel,          ccip_tx.c0.hdr.cl_len, ccip_tx.c0.hdr.req_type, ccip_tx.c0.hdr.address, ccip_tx.c0.hdr.mdata};

   assign xz_tx1_flag = ^{ccip_tx.c1.hdr.vc_sel, ccip_tx.c1.hdr.sop, ccip_tx.c1.hdr.cl_len, ccip_tx.c1.hdr.req_type, ccip_tx.c1.hdr.address, ccip_tx.c1.hdr.mdata, ccip_tx.c1.data};

   assign xz_tx2_flag = ^{ccip_tx.c2.hdr.tid, ccip_tx.c2.data};

   // Trigger XZ warnings
   always @(posedge clk) begin
      // ------------------------------------------------- //
      if (ccip_tx.c0.valid  && isEqualsXorZ(xz_tx0_flag)) begin
     decode_error_code(0, SNIFF_C0TX_XZ_FOUND_WARN);
      end
      else
       error_code_q[SNIFF_C0TX_XZ_FOUND_WARN]=1'b0;

      // ------------------------------------------------- //
      if (ccip_tx.c1.valid && isEqualsXorZ(xz_tx1_flag) && isCCIPWriteRequest(ccip_tx.c1.hdr)) begin
     decode_error_code(0, SNIFF_C1TX_XZ_FOUND_WARN);
      end
      else
      error_code_q[SNIFF_C1TX_XZ_FOUND_WARN]=1'b0;

      // ------------------------------------------------- //
      if (ccip_tx.c2.mmioRdValid && isEqualsXorZ(xz_tx2_flag)) begin
     decode_error_code(0, MMIO_RDRSP_XZ_FOUND_WARN);
      end
      else
      error_code_q[ MMIO_RDRSP_XZ_FOUND_WARN]=1'b0;
      // ------------------------------------------------- //
   end


   /*
    * Full {0,1} signaling
    */
   always @(posedge clk) begin
      // ------------------------------------------------- //
      // Channel 0 overflow check
      if (cf2as_ch0_realfull && ccip_tx.c0.valid) begin
     decode_error_code(0, SNIFF_C0TX_OVERFLOW);
      end
      else
      error_code_q[ SNIFF_C0TX_OVERFLOW]=1'b0;

      // ------------------------------------------------- //
      // Channel 1 overflow check
      if (cf2as_ch1_realfull && ccip_tx.c1.valid) begin
     decode_error_code(0, SNIFF_C1TX_OVERFLOW);
      end
      else
       error_code_q[ SNIFF_C1TX_OVERFLOW]=1'b0;
      // ------------------------------------------------- //
   end


   /*
    * Illegal transaction checker
    */
   always @(posedge clk) begin : illegal_reqproc
      // ------------------------------------------------- //
      // C0TxHdr reqtype
      if (ccip_tx.c0.valid) begin
        if (ccip_tx.c0.hdr.req_type inside {eREQ_RDLINE_S, eREQ_RDLINE_I}) begin
            error_code_q[ SNIFF_C0TX_INVALID_REQTYPE]=1'b0;
        end
        else begin
        decode_error_code(0, SNIFF_C0TX_INVALID_REQTYPE);
        end
      end
      else
        error_code_q[ SNIFF_C0TX_INVALID_REQTYPE]=1'b0;

     // ------------------------------------------------- //
      // C1TxHdr reqtype
      if (ccip_tx.c1.valid) begin
        if (ccip_tx.c1.hdr.req_type inside {eREQ_WRLINE_M
`ifdef ASE_ENABLE_INTR_FEATURE
                            , eREQ_INTR
`endif
                            , eREQ_WRLINE_I
                            , eREQ_WRFENCE
                            , eREQ_WRPUSH_I}) begin
            error_code_q[SNIFF_C1TX_INVALID_REQTYPE]=1'b0;
        end
        else begin
            decode_error_code(0, SNIFF_C1TX_INVALID_REQTYPE);
        end
     end
     else
        error_code_q[SNIFF_C1TX_INVALID_REQTYPE]=1'b0;

     // ------------------------------------------------- //
   end


   /*
    * C0Tx Multi-line Request checker
    */
   // 3CL and Address alignment checker
   always @(posedge clk) begin : c0tx_mcl_checker
      if (ccip_tx.c0.valid && isCCIPReadRequest(ccip_tx.c0.hdr)) begin

      // -------------------------------------------------------- //
      // Invalid length - 3 CL checks
        if (ccip_tx.c0.hdr.cl_len == 2'b10) begin
            decode_error_code(0, SNIFF_C0TX_3CL_REQUEST);
        end
        else
            error_code_q[ SNIFF_C0TX_3CL_REQUEST]=1'b0;

     // -------------------------------------------------------- //
     // Address alignment checks
     if ((ccip_tx.c0.hdr.cl_len == 2'b01) && (ccip_tx.c0.hdr.address[0] != 1'b0)) begin
        decode_error_code(0, SNIFF_C0TX_ADDRALIGN_2_ERROR);
        decode_error_code(0, SNIFF_C0TX_UNEXP_ADDR);
        error_code_q[SNIFF_C0TX_ADDRALIGN_4_ERROR]=1'b0;
     end
     else if ((ccip_tx.c0.hdr.cl_len == 2'b11) && (ccip_tx.c0.hdr.address[1:0] != 2'b00)) begin
        decode_error_code(0, SNIFF_C0TX_ADDRALIGN_4_ERROR);
        error_code_q[SNIFF_C0TX_ADDRALIGN_2_ERROR]=1'b0;
        decode_error_code(0, SNIFF_C0TX_UNEXP_ADDR);
     end
     else
     begin
          error_code_q[SNIFF_C0TX_UNEXP_ADDR]=1'b0;
          error_code_q[SNIFF_C0TX_ADDRALIGN_2_ERROR]=1'b0;
          error_code_q[SNIFF_C0TX_ADDRALIGN_4_ERROR]=1'b0;
     end

     // -------------------------------------------------------- //
     // Address zero warning
     if (ccip_tx.c0.hdr.address == t_ccip_clAddr'(0)) begin
            decode_error_code(0, SNIFF_C0TX_ADDR_ZERO_WARN);
        end
        else
            error_code_q[SNIFF_C0TX_ADDR_ZERO_WARN]=1'b0;
        end
      else
      begin
          error_code_q[SNIFF_C0TX_UNEXP_ADDR]=1'b0;
          error_code_q[SNIFF_C0TX_ADDRALIGN_2_ERROR]=1'b0;
          error_code_q[SNIFF_C0TX_ADDRALIGN_4_ERROR]=1'b0;
          error_code_q[SNIFF_C0TX_ADDR_ZERO_WARN]=1'b0;
          error_code_q[SNIFF_C0TX_3CL_REQUEST]=1'b0;
      end
   end


   /*
    * C1Tx Multi-line Request checker
    */
   typedef enum {
            Exp_1CL_WrFence,
            Exp_2CL,
            Exp_3CL,
            Exp_4CL
            } ExpTxState;
   ExpTxState exp_c1state,exp_c1state_q;

   logic [15:0]     base_c1mdata;
   logic [1:0]         base_c1addr_low2;
   t_ccip_vc        base_c1vc;
   logic [1:0]         base_c1len;
   t_ccip_c1_req    base_c1reqtype;
   logic [1:0]      mcl_address;
   logic             c1tx_1to3_flag;
   logic            sop_mcl_flag;
   logic            wrfence_flag;
   logic            mcl_flag;
   logic             mcl_1to3_wrfence;

   // Base signal sampling
   always @(posedge clk) begin
      if (SoftReset) begin
     base_c1addr_low2 <= 0;
     base_c1vc        <= t_ccip_vc'(0);
     base_c1len       <= 0;
     base_c1mdata     <= 0;
     base_c1reqtype   <= t_ccip_c1_req'(0);
      end
      else
    begin
       if (ccip_tx.c1.hdr.sop && ccip_tx.c1.valid ) begin
          base_c1addr_low2 <= ccip_tx.c1.hdr.address[1:0];
          base_c1vc        <= ccip_tx.c1.hdr.vc_sel;
          base_c1len       <= ccip_tx.c1.hdr.cl_len;
          base_c1mdata     <= ccip_tx.c1.hdr.mdata;
          base_c1reqtype   <= ccip_tx.c1.hdr.req_type;
       end
    end
   end

   // Transaction Checker FSM
   always @(posedge clk) begin
      if (SoftReset) begin
        exp_c1state = Exp_1CL_WrFence;
      end
      else
      begin
              if(ccip_tx.c1.valid)
           begin
               exp_c1state = exp_c1state_q;
           end
      end
   end

   always@(* ) begin
    case (exp_c1state)

    // ==================================================== //
    // 1st line in MCL request OR Write Fence Request
    Exp_1CL_WrFence:
    begin
            mcl_flag=0;
            c1tx_1to3_flag = 0;
            mcl_address=2'b00;
            error_code_q[SNIFF_C1TX_UNEXP_ADDR]=1'b0;

        // ----------------------------------------- //
        // SOP check
    if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ~ccip_tx.c1.hdr.sop)
                sop_mcl_flag=1;
            else
                sop_mcl_flag=0;

        // ----------------------------------------- //
        // 3CL transaction check
            if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.cl_len == 2'b10) && ccip_tx.c1.hdr.sop)
                decode_error_code(0, SNIFF_C1TX_3CL_REQUEST);
            else
                error_code_q[SNIFF_C1TX_3CL_REQUEST]=1'b0;

        // ----------------------------------------- //
        // Address alignment checks
            if (ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.cl_len == 2'b01) && (ccip_tx.c1.hdr.address[0] != 1'b0))
            begin
                decode_error_code(0, SNIFF_C1TX_ADDRALIGN_2_ERROR);
                error_code_q[SNIFF_C1TX_ADDRALIGN_4_ERROR]=1'b0;
            end
            else if (ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.cl_len == 2'b11) && (ccip_tx.c1.hdr.address[1:0] != 2'b00))
            begin
                decode_error_code(0, SNIFF_C1TX_ADDRALIGN_4_ERROR);
                error_code_q[SNIFF_C1TX_ADDRALIGN_2_ERROR]=1'b0;
            end
            else
            begin
                error_code_q[SNIFF_C1TX_ADDRALIGN_2_ERROR]=1'b0;
                error_code_q[SNIFF_C1TX_ADDRALIGN_4_ERROR]=1'b0;
            end

        // -------------------------------------------------------- //
        // Address zero warning
            if (ccip_tx.c1.valid && (ccip_tx.c1.hdr.address == t_ccip_clAddr'(0)) && isCCIPWriteRequest(ccip_tx.c1.hdr))
                decode_error_code(0, SNIFF_C1TX_ADDR_ZERO_WARN);
            else
                error_code_q[SNIFF_C1TX_ADDR_ZERO_WARN]=1'b0;

        // ----------------------------------------- //
        // State Transition
            if (ccip_tx.c1.valid && isWrFenceRequest(ccip_tx.c1.hdr)) begin
                mcl_1to3_wrfence=0;
                exp_c1state_q = Exp_1CL_WrFence;
            end
            else if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && (ccip_tx.c1.hdr.cl_len == 2'b00)) begin
                mcl_1to3_wrfence=0;
                exp_c1state_q = Exp_1CL_WrFence;
            end
            else if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && (ccip_tx.c1.hdr.cl_len == 2'b01)) begin
                mcl_1to3_wrfence=1;
                exp_c1state_q = Exp_2CL;
            end
            else if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && (ccip_tx.c1.hdr.cl_len == 2'b11)) begin
                mcl_1to3_wrfence=1;
                exp_c1state_q = Exp_2CL;
            end
            else begin
                mcl_1to3_wrfence=0;
                c1tx_1to3_flag = 0;
                exp_c1state_q = Exp_1CL_WrFence;
            end
        end

       // ==================================================== //
       // 2nd line in MCL request
       Exp_2CL:
       begin
            sop_mcl_flag=0;
            mcl_address=base_c1addr_low2 + 1;
            /*if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.address[1:0] != (base_c1addr_low2 + 1))) begin
                   decode_error_code(0, SNIFF_C1TX_UNEXP_ADDR);
            end
            else
                error_code_q[SNIFF_C1TX_UNEXP_ADDR]=1'b0;*/
            // ----------------------------------------- //

        // ----------------------------------------- //
        // State transition
            if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (base_c1len == 2'b11)) begin
                mcl_flag=1;
                mcl_1to3_wrfence=1;
                exp_c1state_q = Exp_3CL;
            end
            else if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (base_c1len == 2'b01)||(base_c1len == 2'b00)) begin
                mcl_flag=1;
                mcl_1to3_wrfence=0;
                exp_c1state_q = Exp_1CL_WrFence;
            end
            else
            begin
                mcl_flag=0;
                mcl_1to3_wrfence=1;
                exp_c1state_q = Exp_2CL;
            end

            if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && ((base_c1len == 2'b11)||(base_c1len == 2'b01)))
                c1tx_1to3_flag = 1;
            else
                c1tx_1to3_flag = 0;
      end

       // ==================================================== //
       // 3rd line in MCL request
       Exp_3CL:
       begin
            sop_mcl_flag=0;
            mcl_address=base_c1addr_low2 + 2;
        /*    if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.address[1:0] != (base_c1addr_low2 + 2))) begin
                   decode_error_code(0, SNIFF_C1TX_UNEXP_ADDR);
            end
            else
                error_code_q[SNIFF_C1TX_UNEXP_ADDR]=1'b0;*/
        // ----------------------------------------- //

        // ----------------------------------------- //
        // State transition
        if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (base_c1len == 2'b11)) begin
            mcl_flag=1;
            mcl_1to3_wrfence=1;
            exp_c1state_q = Exp_4CL;
        end
        else
        begin
            mcl_flag=0;
            mcl_1to3_wrfence=1;
            exp_c1state_q = Exp_3CL;
        end
        if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && (base_c1len == 2'b11))
           c1tx_1to3_flag = 1;
         else
            c1tx_1to3_flag = 0;
        end

      // ==================================================== //
       // 4th line in MCL request
       Exp_4CL:
       begin
            sop_mcl_flag=0;
            mcl_address=base_c1addr_low2 + 3;
        /*    if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.address[1:0] != (base_c1addr_low2 + 3))) begin
                   decode_error_code(0, SNIFF_C1TX_UNEXP_ADDR);
            end
            else
                error_code_q[SNIFF_C1TX_UNEXP_ADDR]=1'b0;*/
        // ----------------------------------------- //

        // ----------------------------------------- //
        // State transition
        if (ccip_tx.c1.valid && isCCIPWrFenceRequest(ccip_tx.c1.hdr)) begin
            exp_c1state_q =Exp_4CL;
            mcl_1to3_wrfence=1;
            mcl_flag =0;
       end
       else
       begin
           mcl_1to3_wrfence=0;
           exp_c1state_q =Exp_1CL_WrFence;
           mcl_flag=1;
       end

       if (ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && ccip_tx.c1.hdr.sop && (base_c1len == 2'b11))
           c1tx_1to3_flag = 1;
         else
           c1tx_1to3_flag = 0;
        end

       // ==================================================== //
       // Lala-land
       default:
       begin
           mcl_address=2'b00;
           mcl_flag=0;
           c1tx_1to3_flag = 0;
           sop_mcl_flag =0;
              exp_c1state_q = Exp_1CL_WrFence;
       end
     endcase
     end

   /*
    * Subsequent line checks
    */
   always @(posedge clk) begin
      // ----------------------------------------- //
      //Check for unexpected Errors in C1Tx
      if (mcl_flag && ccip_tx.c1.valid && isCCIPWriteRequest(ccip_tx.c1.hdr) && (ccip_tx.c1.hdr.address[1:0] != (mcl_address))) begin
            decode_error_code(0, SNIFF_C1TX_UNEXP_ADDR);
            end
      else
            error_code_q[SNIFF_C1TX_UNEXP_ADDR]=1'b0;

      //Check if SOP is set or not for the 1st MCL
      if(sop_mcl_flag)
           decode_error_code(0, SNIFF_C1TX_SOP_NOT_SET);
      else
           error_code_q[SNIFF_C1TX_SOP_NOT_SET]=1'b0;

      // Write Fence must not be seen here
      if (isCCIPWrFenceRequest(ccip_tx.c1.hdr)&&  ccip_tx.c1.valid && mcl_1to3_wrfence) begin
            decode_error_code(0, SNIFF_C1TX_WRFENCE_IN_MCL1TO3);
      end
      else
            error_code_q[SNIFF_C1TX_WRFENCE_IN_MCL1TO3]=1'b0;

      // ----------------------------------------- //
      // C1TX 1to3 SOP check
      if (~(isCCIPWrFenceRequest(ccip_tx.c1.hdr)) && c1tx_1to3_flag && ccip_tx.c1.valid && ccip_tx.c1.hdr.sop) begin
            decode_error_code(0, SNIFF_C1TX_SOP_SET_MCL1TO3);
      end
      else
            error_code_q[SNIFF_C1TX_SOP_SET_MCL1TO3]=1'b0;

      // ----------------------------------------- //
      // CL_LEN modification check [Warning only]
      if (mcl_flag &&~(isCCIPWrFenceRequest(ccip_tx.c1.hdr)) &&  ~ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && (ccip_tx.c1.hdr.cl_len != base_c1len)) begin
            decode_error_code(0, SNIFF_C1TX_UNEXP_CLLEN);
      end
      else
            error_code_q[SNIFF_C1TX_UNEXP_CLLEN]=1'b0;

      // ----------------------------------------- //
      // VC modification check
      if (mcl_flag &&~(isCCIPWrFenceRequest(ccip_tx.c1.hdr))&& ~ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && (ccip_tx.c1.hdr.vc_sel != base_c1vc)) begin
            decode_error_code(0, SNIFF_C1TX_UNEXP_VCSEL);
      end
      else
            error_code_q[SNIFF_C1TX_UNEXP_VCSEL]=1'b0;

     // ----------------------------------------- //
      // MDATA modification check
      if (mcl_flag &&~(isCCIPWrFenceRequest(ccip_tx.c1.hdr))&& ~ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && (ccip_tx.c1.hdr.mdata != base_c1mdata)) begin
            decode_error_code(0, SNIFF_C1TX_UNEXP_MDATA);
      end
      else
            error_code_q[SNIFF_C1TX_UNEXP_MDATA]=1'b0;

        // ----------------------------------------- //
      // Write Fence and SOP bit set modification check
      if (isCCIPWrFenceRequest(ccip_tx.c1.hdr)&&  ccip_tx.c1.valid && ccip_tx.c1.hdr.sop) begin
            decode_error_code(0, SNIFF_C1TX_WRFENCE_SOP_SET);
      end
      else
            error_code_q[SNIFF_C1TX_WRFENCE_SOP_SET]=1'b0;

      // ----------------------------------------- //
      // Request Type modification check
      if (mcl_flag && ~ccip_tx.c1.hdr.sop && ccip_tx.c1.valid && (ccip_tx.c1.hdr.req_type != base_c1reqtype)) begin
            decode_error_code(0, SNIFF_C1TX_UNEXP_REQTYPE);
      end
      else
            error_code_q[SNIFF_C1TX_UNEXP_REQTYPE]=1'b0;
 end


   /*
    * Check memory transactions in flight, maintain active list
    */
 longint rd_active_addr_array[*];
   longint wr_active_addr_array[*];

   string  waw_haz_str;
   string  raw_haz_str;
   string  war_haz_str;
   logic   hazard_found;

  // ------------------------------------------- //
   // Hazard check process
   // - Take in address, check if exists in
   // ------------------------------------------- //
   always @(posedge clk) begin
      // ------------------------------------------- //
      // Read in (unroll necessary)
      // ------------------------------------------- //
      if (haz_if.read_in.valid) begin
        for (int ii = 0; ii <= haz_if.read_in.hdr.len ; ii = ii + 1) begin : read_channel_haz_monitor
        rd_active_addr_array[ haz_if.read_in.hdr.addr + ii ] = haz_if.read_in.hdr.addr + ii;

        // Check for outstanding write request
        if (wr_active_addr_array.exists(haz_if.read_in.hdr.addr + ii)) begin
            $sformat(war_haz_str,
            "%d | Potential for Write-after-Read hazard with potential for C0TxHdr=%s arriving earlier than write to same address\n",
            $time,
            return_txhdr(haz_if.read_in.hdr));
            print_message_and_log(1, war_haz_str);
        end
        end
      end

      // ------------------------------------------- //
      // Write in
      // ------------------------------------------- //
      if (haz_if.write_in.valid) begin
     // Check for outstanding read
        if (rd_active_addr_array.exists(haz_if.write_in.hdr.addr)) begin : write_channel_haz_monitor
             $sformat(raw_haz_str,
             "%d | Potential for Read-after-Write hazard with potential for C1TxHdr=%s arriving earlier than write to same address\n",
             $time,
             return_txhdr(haz_if.write_in.hdr));
             print_message_and_log(1, raw_haz_str);
        end
     // Check for outstanding write
        else if (wr_active_addr_array.exists(haz_if.write_in.hdr.addr)) begin
             $sformat(waw_haz_str,
             "%d | Potential for Write-after-Write hazard with potential for C1TxHdr=%s arriving earlier than write to same address\n",
             $time,
             return_txhdr(haz_if.write_in.hdr));
             print_message_and_log(1, waw_haz_str);
        end
        // If not, store
        else begin
            wr_active_addr_array[haz_if.write_in.hdr.addr] = haz_if.write_in.hdr.addr;
        end
     end

      // ------------------------------------------- //
      // Read out (delete from active list)
      // ------------------------------------------- //
      if (haz_if.read_out.valid) begin
        if (rd_active_addr_array.exists( haz_if.read_out.hdr.addr )) begin
         rd_active_addr_array.delete( haz_if.read_out.hdr.addr );
      end
      end

     // ------------------------------------------- //
      // Write out (delete from active list)
      // ------------------------------------------- //
      if (haz_if.write_out.valid) begin
        if (wr_active_addr_array.exists( haz_if.write_out.hdr.addr )) begin
            wr_active_addr_array.delete( haz_if.write_out.hdr.addr );
     end
     end
   end

   //Making sure error_code doesnt go into an undetermined state when only one channel is Active
   always@(*)
   begin
        error_code_q[31:30]=2'b0;
        if((ccip_rx.c0.mmioRdValid || ccip_tx.c2.mmioRdValid)&& ~ccip_tx.c1.valid && ~ccip_tx.c0.valid) begin
            error_code_q[31:5]=28'b0;
            error_code_q[0]=1'b0;
        end
        else if(ccip_tx.c1.valid && ~ccip_tx.c0.valid && ~(ccip_rx.c0.mmioRdValid || ccip_tx.c2.mmioRdValid) ) begin
            error_code_q[13:0]=13'b0;
        end
        else if(ccip_tx.c0.valid && ~(ccip_rx.c0.mmioRdValid || ccip_tx.c2.mmioRdValid) && ~ccip_tx.c1.valid ) begin
            error_code_q[4:0]=4'b0;
            error_code_q[31:14]=17'b0;
        end
   end

   /*
    * Multiple outstandind MMIO Response tracking
    * - Maintains `MMIO_MAX_OUTSTANDING records tracking activity
    * - Tracking key = MMIO TID
    */
   parameter int      MMIO_TRACKER_DEPTH = 2**CCIP_CFGHDR_TID_WIDTH;

   // Tracker structure
   typedef struct {
      // Status management
      logic [`MMIO_RESPONSE_TIMEOUT_RADIX-1:0] timer_val;
      logic                        timeout;
      logic                        active;
   } mmioread_track_t;
   mmioread_track_t mmioread_tracker[0:MMIO_TRACKER_DEPTH-1];

   // Push/pop control process
   task update_mmio_activity(
                    logic                    clear,
                    logic                    mmio_request,
                    logic                    mmio_response,
                    logic [CCIP_CFGHDR_TID_WIDTH-1:0] tid
                    );
      begin
        if (clear) begin
           mmioread_tracker[tid].active = 0;
        error_code_q[MMIO_RDRSP_UNSOLICITED]=1'b0;
        end
        else begin
        // If pop occured when not active
           if (~mmioread_tracker[tid].active && mmio_response) begin
           decode_error_code(0, MMIO_RDRSP_UNSOLICITED);
           end
        else
            error_code_q[MMIO_RDRSP_UNSOLICITED]=1'b0;
        // Active management
        if (mmio_request) begin
           mmioread_tracker[tid].active = 1;
        end
        else if (mmio_response) begin
           mmioread_tracker[tid].active = 0;
        end
        end
     end
   endtask

   // Push/pop glue
   always @(posedge clk) begin
      if (SoftReset) begin

        for (int track_i = 0; track_i < MMIO_TRACKER_DEPTH ; track_i = track_i + 1) begin
           update_mmio_activity(1, 0, 0, track_i);
        end
      end
      else begin
        // Push transaction to checker (mmioRead Request)
        if (ccip_rx.c0.mmioRdValid) begin
           update_mmio_activity(0, ccip_rx.c0.mmioRdValid, 0, C0RxCfg.tid);
        end
        // Pop transaction from checker (mmioRead Response)
        if (ccip_tx.c2.mmioRdValid) begin
           update_mmio_activity(0, 0, ccip_tx.c2.mmioRdValid, ccip_tx.c2.hdr.tid);
        end
      end
   end

   // Tracker block
   generate
      for (genvar ii = 0; ii < MMIO_TRACKER_DEPTH ; ii = ii + 1) begin : mmio_tracker_block
        // Counter value
        always @(posedge clk) begin
           if (ase_reset) begin
              mmioread_tracker[ii].timer_val <= 0;
           end
           else begin
              if (~mmioread_tracker[ii].active) begin
                mmioread_tracker[ii].timer_val <= 0;
              end
              else if (mmioread_tracker[ii].active) begin
            if(mmioread_tracker[ii].timer_val >= (`MMIO_RESPONSE_TIMEOUT ))
            begin
            end
            else
                mmioread_tracker[ii].timer_val <= mmioread_tracker[ii].timer_val + 1;
              end
           end
        end // always @ (posedge clk)

        // Timeout flag
        always @(posedge clk) begin
           if (ase_reset|~mmioread_tracker[ii].active) begin
              mmioread_tracker[ii].timeout <= 0;
           error_code_q[MMIO_RDRSP_TIMEOUT]=1'b0;
           end
           else if (mmioread_tracker[ii].timer_val >= (`MMIO_RESPONSE_TIMEOUT ))
        begin
              mmioread_tracker[ii].timeout <= 1;
           decode_error_code(0, MMIO_RDRSP_TIMEOUT);
           end
        else
           error_code_q[MMIO_RDRSP_TIMEOUT]=1'b0;
        end
    end
   endgenerate

endmodule // cci_sniffer
