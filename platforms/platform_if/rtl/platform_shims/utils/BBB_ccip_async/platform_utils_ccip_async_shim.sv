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
 */

/*
 * Module: platform_utils_ccip_async_shim
 *         CCI-P async shim to connect slower/faster AFUs to 400 Mhz Blue bitstream
 *
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * Documentation: See Related Application Note
 *
 */

`include "platform_if.vh"

module platform_utils_ccip_async_shim
  #(
    parameter DEBUG_ENABLE          = 0,
    parameter ENABLE_EXTRA_PIPELINE = 1,
    parameter C0TX_DEPTH_RADIX      = 8,
    parameter C1TX_DEPTH_RADIX      = 8,
    // There is no back pressure on C2TX, so it must be large enough to hold the
    // maximum outstanding MMIO read requests (64).
    parameter C2TX_DEPTH_RADIX      = 6,
    parameter C0RX_DEPTH_RADIX      = $clog2(2 * ccip_cfg_pkg::C0_MAX_BW_ACTIVE_LINES[0]),
    parameter C1RX_DEPTH_RADIX      = $clog2(2 * ccip_cfg_pkg::C1_MAX_BW_ACTIVE_LINES[0])
    )
   (
    // ---------------------------------- //
    // Blue Bitstream Interface
    // ---------------------------------- //
    input logic        bb_softreset,
    input logic        bb_clk,
    output             t_if_ccip_Tx bb_tx,
    input              t_if_ccip_Rx bb_rx,
    input logic [1:0]  bb_pwrState,
    input logic        bb_error,

    // ---------------------------------- //
    // Green Bitstream interface
    // ---------------------------------- //
    output logic       afu_softreset,
    input logic        afu_clk,
    input              t_if_ccip_Tx afu_tx,
    output             t_if_ccip_Rx afu_rx,
    output logic [1:0] afu_pwrState,
    output logic       afu_error,

    // ---------------------------------- //
    // Error vector
    // ---------------------------------- //
    output logic [4:0] async_shim_error
    );

   localparam C0TX_TOTAL_WIDTH = 1 + $bits(t_ccip_c0_ReqMemHdr) ;
   localparam C1TX_TOTAL_WIDTH = 1 + $bits(t_ccip_c1_ReqMemHdr) + CCIP_CLDATA_WIDTH;
   localparam C2TX_TOTAL_WIDTH = 1 + $bits(t_ccip_c2_RspMmioHdr) + CCIP_MMIODATA_WIDTH;
   localparam C0RX_TOTAL_WIDTH = 3 + $bits(t_ccip_c0_RspMemHdr) + CCIP_CLDATA_WIDTH;
   localparam C1RX_TOTAL_WIDTH = 1 + $bits(t_ccip_c1_RspMemHdr);


   //
   // Reset synchronizer
   //
   (* preserve *) logic reset[3:0] = '{1'b1, 1'b1, 1'b1, 1'b1};
   assign afu_softreset = reset[3];

   always @(posedge bb_clk) begin
      reset[0] <= bb_softreset;
   end

   always @(posedge afu_clk) begin
      reset[3:1] <= reset[2:0];
   end


   //
   // Power state and error synchronizer
   //
   (* preserve *) logic [1:0] pwrState[3:0];
   (* preserve *) logic error[3:0];
   assign afu_pwrState = pwrState[3];
   assign afu_error = error[3];

   always @(posedge bb_clk) begin
      pwrState[0] <= bb_pwrState;
      error[0] <= bb_error;
   end

   always @(posedge afu_clk) begin
      pwrState[3:1] <= pwrState[2:0];
      error[3:1] <= error[2:0];
   end


   t_if_ccip_Rx bb_rx_q;
   t_if_ccip_Rx afu_rx_q;

   t_if_ccip_Tx bb_tx_q;
   t_if_ccip_Tx afu_tx_q;

   always @(posedge afu_clk) begin
      afu_rx   <= afu_rx_q;
      afu_tx_q <= afu_tx;
   end

   always @(posedge bb_clk) begin
      bb_rx_q <= bb_rx;
      bb_tx   <= bb_tx_q;
   end


   /*
    * C0Tx Channel
    */
   logic [C0TX_DEPTH_RADIX-1:0] c0tx_cnt;
   logic [C0TX_TOTAL_WIDTH-1:0] c0tx_dout;
   logic                        c0tx_rdreq;
   logic                        c0tx_rdempty;
   logic                        c0tx_rdempty_q;
   logic                        c0tx_valid;
   logic                        c0tx_fifo_wrfull;

   platform_utils_ccip_afifo_channel
     #(
       .DATA_WIDTH  (C0TX_TOTAL_WIDTH),
       .DEPTH_RADIX (C0TX_DEPTH_RADIX)
       )
   c0tx_afifo
     (
      .data    ( {afu_tx_q.c0.hdr, afu_tx_q.c0.valid} ),
      .wrreq   ( afu_tx_q.c0.valid ),
      .rdreq   ( c0tx_rdreq ),
      .wrclk   ( afu_clk ),
      .rdclk   ( bb_clk ),
      .aclr    ( reset[0] ),
      .q       ( c0tx_dout ),
      .rdusedw ( ),
      .wrusedw ( c0tx_cnt ),
      .rdfull  ( ),
      .rdempty ( c0tx_rdempty ),
      .wrfull  ( c0tx_fifo_wrfull ),
      .wrempty ( )
      );

   // Track round-trip request -> response credits to avoid filling the
   // response pipeline.
   logic [C0RX_DEPTH_RADIX-1:0] c0req_cnt;
   platform_utils_ccip_async_c0_active_cnt
     #(
       .C0RX_DEPTH_RADIX (C0RX_DEPTH_RADIX)
       )
   c0req_credit_counter
     (
      .clk   ( afu_clk ),
      .reset ( afu_softreset ),
      .c0Tx  ( afu_tx_q.c0 ),
      .c0Rx  ( afu_rx_q.c0 ),
      .cnt   ( c0req_cnt )
      );

   always @(posedge bb_clk) begin
      c0tx_valid <= c0tx_rdreq & ~c0tx_rdempty;
   end

   // Extra pipeline register to ease timing pressure -- disable as needed
   generate
      if (ENABLE_EXTRA_PIPELINE == 1) begin
         always @(posedge bb_clk) begin
            c0tx_rdempty_q <= c0tx_rdempty;
         end
      end
      else begin
         always @(*) begin
            c0tx_rdempty_q <= c0tx_rdempty;
         end
      end
   endgenerate


   always @(posedge bb_clk) begin
      c0tx_rdreq <= ~bb_rx_q.c0TxAlmFull & ~c0tx_rdempty_q;
   end

   always @(posedge bb_clk) begin
      if (c0tx_valid) begin
         {bb_tx_q.c0.hdr, bb_tx_q.c0.valid} <= c0tx_dout;
      end
      else begin
         {bb_tx_q.c0.hdr, bb_tx_q.c0.valid} <= 0;
      end
   end

   // Maximum number of line requests outstanding is the size of the buffer
   // minus the number of requests that may arrive after asserting almost full.
   // Multiply the threshold by 8 instead of 4 (the maximum line request
   // size) in order to leave room for MMIO requests and some delay in
   // the AFU responding to almost full.
   localparam C0_REQ_CREDIT_LIMIT = (2 ** C0RX_DEPTH_RADIX) -
                                    CCIP_TX_ALMOST_FULL_THRESHOLD * 8;
   generate
       if (C0_REQ_CREDIT_LIMIT <= 0) begin
           //
           // Error: C0RX_DEPTH_RADIX is too small, given the number of
           //        requests that may be in flight after almost full is
           //        asserted!
           //
           // Force a compile-time failure...
           PARAMETER_ERROR dummy();
           always $display("C0RX_DEPTH_RADIX is too small");
       end
   endgenerate

   always @(posedge afu_clk) begin
      afu_rx_q.c0TxAlmFull <= c0tx_cnt[C0TX_DEPTH_RADIX-1] ||
                              (c0req_cnt > C0RX_DEPTH_RADIX'(C0_REQ_CREDIT_LIMIT));
   end


   /*
    * C1Tx Channel
    */
   logic [C1TX_DEPTH_RADIX-1:0] c1tx_cnt;
   logic [C1TX_TOTAL_WIDTH-1:0] c1tx_dout;
   logic                        c1tx_rdreq;
   logic                        c1tx_rdempty;
   logic                        c1tx_rdempty_q;
   logic                        c1tx_valid;
   logic                        c1tx_fifo_wrfull;

   platform_utils_ccip_afifo_channel
     #(
       .DATA_WIDTH  (C1TX_TOTAL_WIDTH),
       .DEPTH_RADIX (C1TX_DEPTH_RADIX)
       )
   c1tx_afifo
     (
      .data    ( {afu_tx_q.c1.hdr, afu_tx_q.c1.data, afu_tx_q.c1.valid} ),
      .wrreq   ( afu_tx_q.c1.valid ),
      .rdreq   ( c1tx_rdreq ),
      .wrclk   ( afu_clk ),
      .rdclk   ( bb_clk ),
      .aclr    ( reset[0] ),
      .q       ( c1tx_dout ),
      .rdusedw ( ),
      .wrusedw ( c1tx_cnt ),
      .rdfull  ( ),
      .rdempty ( c1tx_rdempty ),
      .wrfull  ( c1tx_fifo_wrfull ),
      .wrempty ( )
      );

   // Track round-trip request -> response credits to avoid filling the
   // response pipeline.
   logic [C1RX_DEPTH_RADIX-1:0] c1req_cnt;
   platform_utils_ccip_async_c1_active_cnt
     #(
       .C1RX_DEPTH_RADIX (C1RX_DEPTH_RADIX)
       )
   c1req_credit_counter
     (
      .clk   ( afu_clk ),
      .reset ( afu_softreset ),
      .c1Tx  ( afu_tx_q.c1 ),
      .c1Rx  ( afu_rx_q.c1 ),
      .cnt   ( c1req_cnt )
      );

   always @(posedge bb_clk) begin
      c1tx_valid <= c1tx_rdreq & ~c1tx_rdempty;
   end

   // Extra pipeline register to ease timing pressure -- disable as needed
   generate
      if (ENABLE_EXTRA_PIPELINE == 1) begin
         always @(posedge bb_clk) begin
            c1tx_rdempty_q <= c1tx_rdempty;
         end
      end
      else begin
         always @(*) begin
            c1tx_rdempty_q <= c1tx_rdempty;
         end
      end
   endgenerate

   always @(posedge bb_clk) begin
      c1tx_rdreq <= ~bb_rx_q.c1TxAlmFull & ~c1tx_rdempty_q;
   end

   always @(posedge bb_clk) begin
      if (c1tx_valid) begin
         {bb_tx_q.c1.hdr, bb_tx_q.c1.data, bb_tx_q.c1.valid} <= c1tx_dout;
      end
      else begin
         {bb_tx_q.c1.hdr, bb_tx_q.c1.data, bb_tx_q.c1.valid} <= 0;
      end
   end

   // Maximum number of line requests outstanding is the size of the buffer
   // minus the number of requests that may arrive after asserting almost full,
   // with some wiggle room added for message latency.
   localparam C1_REQ_CREDIT_LIMIT = (2 ** C1RX_DEPTH_RADIX) -
                                    CCIP_TX_ALMOST_FULL_THRESHOLD * 8;
   generate
       if (C1_REQ_CREDIT_LIMIT <= 0) begin
           //
           // Error: C1RX_DEPTH_RADIX is too small, given the number of
           //        requests that may be in flight after almost full is
           //        asserted!
           //
           // Force a compile-time failure...
           PARAMETER_ERROR dummy();
           always $display("C1RX_DEPTH_RADIX is too small");
       end
   endgenerate

   always @(posedge afu_clk) begin
      afu_rx_q.c1TxAlmFull <= c1tx_cnt[C1TX_DEPTH_RADIX-1] ||
                              (c1req_cnt > C1RX_DEPTH_RADIX'(C1_REQ_CREDIT_LIMIT));
   end


   /*
    * C2Tx Channel
    */
   logic [C2TX_TOTAL_WIDTH-1:0] c2tx_dout;
   logic                        c2tx_rdreq;
   logic                        c2tx_rdempty;
   logic                        c2tx_valid;
   logic                        c2tx_fifo_wrfull;

   platform_utils_ccip_afifo_channel
     #(
       .DATA_WIDTH  (C2TX_TOTAL_WIDTH),
       .DEPTH_RADIX (C2TX_DEPTH_RADIX)
       )
   c2tx_afifo
     (
      .data    ( {afu_tx_q.c2.hdr, afu_tx_q.c2.mmioRdValid, afu_tx_q.c2.data} ),
      .wrreq   ( afu_tx_q.c2.mmioRdValid ),
      .rdreq   ( c2tx_rdreq ),
      .wrclk   ( afu_clk ),
      .rdclk   ( bb_clk ),
      .aclr    ( reset[0] ),
      .q       ( c2tx_dout ),
      .rdusedw (),
      .wrusedw (),
      .rdfull  (),
      .rdempty ( c2tx_rdempty ),
      .wrfull  ( c2tx_fifo_wrfull ),
      .wrempty ()
      );

   always @(posedge bb_clk) begin
      c2tx_valid <= c2tx_rdreq & ~c2tx_rdempty;
   end

   always @(posedge bb_clk) begin
      c2tx_rdreq <= ~c2tx_rdempty;
   end

   always @(posedge bb_clk) begin
      if (c2tx_valid) begin
         {bb_tx_q.c2.hdr, bb_tx_q.c2.mmioRdValid, bb_tx_q.c2.data} <= c2tx_dout;
      end
      else begin
         {bb_tx_q.c2.hdr, bb_tx_q.c2.mmioRdValid, bb_tx_q.c2.data} <= 0;
      end
   end


   /*
    * C0Rx Channel
    */
   logic [C0RX_TOTAL_WIDTH-1:0] c0rx_dout;
   logic                        c0rx_valid;
   logic                        c0rx_rdreq;
   logic                        c0rx_rdempty;
   logic                        c0rx_fifo_wrfull;

   platform_utils_ccip_afifo_channel
     #(
       .DATA_WIDTH  (C0RX_TOTAL_WIDTH),
       .DEPTH_RADIX (C0RX_DEPTH_RADIX)
       )
   c0rx_afifo
     (
      .data    ( {bb_rx_q.c0.hdr, bb_rx_q.c0.data, bb_rx_q.c0.rspValid, bb_rx_q.c0.mmioRdValid, bb_rx_q.c0.mmioWrValid} ),
      .wrreq   ( bb_rx_q.c0.rspValid | bb_rx_q.c0.mmioRdValid |  bb_rx_q.c0.mmioWrValid ),
      .rdreq   ( c0rx_rdreq ),
      .wrclk   ( bb_clk ),
      .rdclk   ( afu_clk ),
      .aclr    ( reset[0] ),
      .q       ( c0rx_dout ),
      .rdusedw (),
      .wrusedw (),
      .rdfull  (),
      .rdempty ( c0rx_rdempty ),
      .wrfull  ( c0rx_fifo_wrfull ),
      .wrempty ()
      );

   always @(posedge afu_clk) begin
      c0rx_valid <= c0rx_rdreq & ~c0rx_rdempty;
   end

   always @(posedge afu_clk) begin
      c0rx_rdreq <= ~c0rx_rdempty;
   end

   always @(posedge afu_clk) begin
      if (c0rx_valid) begin
         {afu_rx_q.c0.hdr, afu_rx_q.c0.data, afu_rx_q.c0.rspValid, afu_rx_q.c0.mmioRdValid, afu_rx_q.c0.mmioWrValid} <= c0rx_dout;
      end
      else begin
         {afu_rx_q.c0.hdr, afu_rx_q.c0.data, afu_rx_q.c0.rspValid, afu_rx_q.c0.mmioRdValid, afu_rx_q.c0.mmioWrValid} <= 0;
      end
   end


   /*
    * C1Rx Channel
    */
   logic [C1RX_TOTAL_WIDTH-1:0] c1rx_dout;
   logic                        c1rx_valid;
   logic                        c1rx_rdreq;
   logic                        c1rx_rdempty;
   logic                        c1rx_fifo_wrfull;

   platform_utils_ccip_afifo_channel
     #(
       .DATA_WIDTH  (C1RX_TOTAL_WIDTH),
       .DEPTH_RADIX (C1RX_DEPTH_RADIX)
       )
   c1rx_afifo
     (
      .data    ( {bb_rx_q.c1.hdr, bb_rx_q.c1.rspValid} ),
      .wrreq   ( bb_rx_q.c1.rspValid ),
      .rdreq   ( c1rx_rdreq ),
      .wrclk   ( bb_clk ),
      .rdclk   ( afu_clk ),
      .aclr    ( reset[0] ),
      .q       ( c1rx_dout ),
      .rdusedw (),
      .wrusedw (),
      .rdfull  (),
      .rdempty ( c1rx_rdempty ),
      .wrfull  ( c1rx_fifo_wrfull ),
      .wrempty ()
      );


   always @(posedge afu_clk) begin
      c1rx_valid <= c1rx_rdreq & ~c1rx_rdempty;
   end

   always @(posedge afu_clk) begin
      c1rx_rdreq <= ~c1rx_rdempty;
   end

   always @(posedge afu_clk) begin
      if (c1rx_valid) begin
         {afu_rx_q.c1.hdr, afu_rx_q.c1.rspValid} <= c1rx_dout;
      end
      else begin
         {afu_rx_q.c1.hdr, afu_rx_q.c1.rspValid} <= 0;
      end
   end


   /*
    * Error vector (indicates write error)
    * --------------------------------------------------
    *   0 - C0Tx Write error
    *   1 - C1Tx Write error
    *   2 - C2Tx Write error
    *   3 - C0Rx Write error
    *   4 - C1Rx Write error
    */
   always @(posedge afu_clk) begin
      if (reset[0]) begin
         async_shim_error <= 5'b0;
      end
      else begin
         async_shim_error[0] <= c0tx_fifo_wrfull && afu_tx_q.c0.valid;
         async_shim_error[1] <= c1tx_fifo_wrfull && afu_tx_q.c1.valid;
         async_shim_error[2] <= c2tx_fifo_wrfull && afu_tx_q.c2.mmioRdValid;
         async_shim_error[3] <= c0rx_fifo_wrfull && (bb_rx_q.c0.rspValid|bb_rx_q.c0.mmioRdValid|bb_rx_q.c0.mmioWrValid );
         async_shim_error[4] <= c1rx_fifo_wrfull && bb_rx_q.c1.rspValid;
      end
   end

   // synthesis translate_off
   always @(posedge afu_clk) begin
      if (async_shim_error[0])
        $warning("** ERROR ** C0Tx may have dropped transaction");
      if (async_shim_error[1])
        $warning("** ERROR ** C1Tx may have dropped transaction");
      if (async_shim_error[2])
        $warning("** ERROR ** C2Tx may have dropped transaction");
   end

   always @(posedge bb_clk) begin
      if (async_shim_error[3])
        $warning("** ERROR ** C0Rx may have dropped transaction");
      if (async_shim_error[4])
        $warning("** ERROR ** C1Rx may have dropped transaction");
   end
   // synthesis translate_on


   /*
    * Interface counts
    * - This block is enabled when DEBUG_ENABLE = 1, else disabled
    */
   generate
      if (DEBUG_ENABLE == 1) begin
         // Counts
         (* preserve *) logic [31:0] afu_c0tx_cnt;
         (* preserve *) logic [31:0] afu_c1tx_cnt;
         (* preserve *) logic [31:0] afu_c2tx_cnt;
         (* preserve *) logic [31:0] afu_c0rx_cnt;
         (* preserve *) logic [31:0] afu_c1rx_cnt;
         (* preserve *) logic [31:0] bb_c0tx_cnt;
         (* preserve *) logic [31:0] bb_c1tx_cnt;
         (* preserve *) logic [31:0] bb_c2tx_cnt;
         (* preserve *) logic [31:0] bb_c0rx_cnt;
         (* preserve *) logic [31:0] bb_c1rx_cnt;

         // afu_if counts
         always @(posedge afu_clk) begin
            if (afu_softreset) begin
               afu_c0tx_cnt <= 0;
               afu_c1tx_cnt <= 0;
               afu_c2tx_cnt <= 0;
               afu_c0rx_cnt <= 0;
               afu_c1rx_cnt <= 0;
            end
            else begin
               if (afu_tx_q.c0.valid)
                 afu_c0tx_cnt <= afu_c0tx_cnt + 1;
               if (afu_tx_q.c1.valid)
                 afu_c1tx_cnt <= afu_c1tx_cnt + 1;
               if (afu_tx_q.c2.mmioRdValid)
                 afu_c2tx_cnt <= afu_c2tx_cnt + 1;
               if (afu_rx_q.c0.rspValid|afu_rx_q.c0.mmioRdValid|afu_rx_q.c0.mmioWrValid)
                 afu_c0rx_cnt <= afu_c0rx_cnt + 1;
               if (afu_rx_q.c1.rspValid)
                 afu_c1rx_cnt <= afu_c1rx_cnt + 1;
            end
         end

         // bb_if counts
         always @(posedge bb_clk) begin
            if (reset[0]) begin
               bb_c0tx_cnt <= 0;
               bb_c1tx_cnt <= 0;
               bb_c2tx_cnt <= 0;
               bb_c0rx_cnt <= 0;
               bb_c1rx_cnt <= 0;
            end
            else begin
               if (bb_tx_q.c0.valid)
                 bb_c0tx_cnt <= bb_c0tx_cnt + 1;
               if (bb_tx_q.c1.valid)
                 bb_c1tx_cnt <= bb_c1tx_cnt + 1;
               if (bb_tx_q.c2.mmioRdValid)
                 bb_c2tx_cnt <= bb_c2tx_cnt + 1;
               if (bb_rx_q.c0.rspValid|bb_rx_q.c0.mmioRdValid|bb_rx_q.c0.mmioWrValid)
                 bb_c0rx_cnt <= bb_c0rx_cnt + 1;
               if (bb_rx_q.c1.rspValid)
                 bb_c1rx_cnt <= bb_c1rx_cnt + 1;
            end
         end

      end
   endgenerate


endmodule
