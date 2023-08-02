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

    // There is no back pressure on C2TX, so it must be large enough to hold the
    // maximum outstanding MMIO read requests.
    parameter C2TX_DEPTH_RADIX      = $clog2(ccip_cfg_pkg::MAX_OUTSTANDING_MMIO_RD_REQS),

    parameter C0RX_DEPTH_RADIX      = $clog2(2 * ccip_cfg_pkg::C0_MAX_BW_ACTIVE_LINES[0]),
    parameter C1RX_DEPTH_RADIX      = $clog2(2 * ccip_cfg_pkg::C1_MAX_BW_ACTIVE_LINES[0]),

    // Extra space to add to almust full buffering
    parameter EXTRA_ALMOST_FULL_STAGES = 0
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
    // Error vector (afu_clk domain)
    // ---------------------------------- //
    output logic [4:0] async_shim_error
    );

    localparam C0TX_TOTAL_WIDTH = $bits(t_ccip_c0_ReqMemHdr);
    localparam C1TX_TOTAL_WIDTH = $bits(t_ccip_c1_ReqMemHdr) + CCIP_CLDATA_WIDTH;
    localparam C2TX_TOTAL_WIDTH = $bits(t_ccip_c2_RspMmioHdr) + CCIP_MMIODATA_WIDTH;
    localparam C0RX_TOTAL_WIDTH = 3 + $bits(t_ccip_c0_RspMemHdr) + CCIP_CLDATA_WIDTH;
    localparam C1RX_TOTAL_WIDTH = $bits(t_ccip_c1_RspMemHdr);

    // TX buffers just need to be large enough to avoid pipeline back pressure.
    // The TX rate limiter is the slower of the AFU and BB clocks, not the TX buffer.
    localparam C0TX_DEPTH_RADIX = $clog2(3 * CCIP_TX_ALMOST_FULL_THRESHOLD +
                                         EXTRA_ALMOST_FULL_STAGES);

    // Leave a little extra room in Tx buffers to avoid overflow
    localparam C0TX_ALMOST_FULL_THRESHOLD = CCIP_TX_ALMOST_FULL_THRESHOLD +
                                            (CCIP_TX_ALMOST_FULL_THRESHOLD / 2) +
                                            EXTRA_ALMOST_FULL_STAGES;

    // Write buffers are slightly larger because even 4-line write requests
    // send a line at a time.
    localparam C1TX_DEPTH_RADIX = $clog2(4 * CCIP_TX_ALMOST_FULL_THRESHOLD +
                                         EXTRA_ALMOST_FULL_STAGES);
    localparam C1TX_ALMOST_FULL_THRESHOLD = CCIP_TX_ALMOST_FULL_THRESHOLD +
                                            (CCIP_TX_ALMOST_FULL_THRESHOLD / 2) +
                                            EXTRA_ALMOST_FULL_STAGES;

    //
    // Reset synchronizer
    //
    (* preserve *) logic reset[3:0] = '{1'b1, 1'b1, 1'b1, 1'b1};
    assign afu_softreset = reset[3];

    always @(posedge bb_clk)
    begin
        reset[0] <= bb_softreset;
    end

    always @(posedge afu_clk)
    begin
        reset[3:1] <= reset[2:0];
    end


    //
    // Power state and error synchronizer
    //
    (* preserve *) logic [1:0] pwrState[3:0];
    (* preserve *) logic error[3:0];
    assign afu_pwrState = pwrState[3];
    assign afu_error = error[3];

    always_ff @(posedge bb_clk)
    begin
        pwrState[0] <= bb_pwrState;
        error[0] <= bb_error;
    end

    always_ff @(posedge afu_clk)
    begin
        pwrState[3:1] <= pwrState[2:0];
        error[3:1] <= error[2:0];
    end


    // Stop all traffic when a buffer error is detected.  This tends to be much easier
    // to debug than just dropping overflow packets on the floor.
    logic buffer_error;
    always_ff @(posedge afu_clk)
    begin
        buffer_error <= (|(async_shim_error));
    end


    t_if_ccip_Rx bb_rx_q;
    t_if_ccip_Rx afu_rx_next;

    t_if_ccip_Tx bb_tx_next;
    t_if_ccip_Tx afu_tx_q;

    always_ff @(posedge afu_clk)
    begin
        afu_rx <= afu_rx_next;
        afu_tx_q <= afu_tx;
    end

    always_ff @(posedge bb_clk)
    begin
        bb_rx_q <= bb_rx;
        bb_tx <= bb_tx_next;
    end


    /*
     * C0Tx Channel
     */
    logic               c0tx_almfull;
    t_ccip_c0_ReqMemHdr c0tx_dout;
    logic               c0tx_rdreq, c0tx_rdreq_q;
    logic               c0tx_rdempty;
    logic               c0tx_fifo_wrfull;

    platform_utils_dc_fifo
      #(
        .DATA_WIDTH(C0TX_TOTAL_WIDTH),
        .DEPTH_RADIX(C0TX_DEPTH_RADIX),
        .ALMOST_FULL_THRESHOLD(C0TX_ALMOST_FULL_THRESHOLD)
        )
     c0tx_afifo
       (
        .data(afu_tx_q.c0.hdr),
        .wrreq(afu_tx_q.c0.valid && ! c0tx_fifo_wrfull),
        .rdreq(c0tx_rdreq),
        .wrclk(afu_clk),
        .rdclk(bb_clk),
        .aclr(reset[0]),
        .q(c0tx_dout),
        .rdusedw(),
        .wrusedw(),
        .rdfull(),
        .rdempty(c0tx_rdempty),
        .wrfull(c0tx_fifo_wrfull),
        .wralmfull(c0tx_almfull),
        .wrempty()
        );

    // Forward FIFO toward FIU when there is data and the output channel is available
    assign c0tx_rdreq = ! c0tx_rdempty && ! bb_rx_q.c0TxAlmFull;
    always_ff @(posedge bb_clk)
    begin
        c0tx_rdreq_q <= c0tx_rdreq;
    end

    always_comb
    begin
        bb_tx_next.c0.valid = c0tx_rdreq_q;
        bb_tx_next.c0.hdr = c0tx_dout;
    end

    // Track round-trip request -> response credits to avoid filling the
    // response pipeline.
    logic [C0RX_DEPTH_RADIX-1:0] c0req_cnt;
    platform_utils_ccip_c0_active_cnt
      #(
        .C0RX_DEPTH_RADIX (C0RX_DEPTH_RADIX)
        )
      c0req_credit_counter
        (
         .clk(afu_clk),
         .reset(afu_softreset),
         .c0Tx(afu_tx_q.c0),
         .c0Rx(afu_rx_next.c0),
         .cnt(c0req_cnt)
         );

    // Maximum number of line requests outstanding is the size of the buffer
    // minus the number of requests that may arrive after asserting almost full.
    // Multiply the threshold by 8 instead of 4 (the maximum line request
    // size) in order to leave room for MMIO requests and some delay in
    // the AFU responding to almost full.
    localparam C0_REQ_CREDIT_LIMIT = (2 ** C0RX_DEPTH_RADIX) -
                                     CCIP_TX_ALMOST_FULL_THRESHOLD * 8;
    generate
        if (C0_REQ_CREDIT_LIMIT <= 0)
        begin
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

    always_ff @(posedge afu_clk)
    begin
        afu_rx_next.c0TxAlmFull <= c0tx_almfull ||
                                   (c0req_cnt > C0RX_DEPTH_RADIX'(C0_REQ_CREDIT_LIMIT)) ||
                                   buffer_error;
    end


    /*
     * C1Tx Channel
     */
    logic               c1tx_almfull;
    t_ccip_c1_ReqMemHdr c1tx_dout_hdr;
    t_ccip_clData       c1tx_dout_data;
    logic               c1tx_rdreq, c1tx_rdreq_q;
    logic               c1tx_rdempty;
    logic               c1tx_fifo_wrfull;

    platform_utils_dc_fifo
      #(
        .DATA_WIDTH(C1TX_TOTAL_WIDTH),
        .DEPTH_RADIX(C1TX_DEPTH_RADIX),
        .ALMOST_FULL_THRESHOLD(C1TX_ALMOST_FULL_THRESHOLD)
        )
     c1tx_afifo
       (
        .data({afu_tx_q.c1.hdr, afu_tx_q.c1.data}),
        .wrreq(afu_tx_q.c1.valid && ! c1tx_fifo_wrfull),
        .rdreq(c1tx_rdreq),
        .wrclk(afu_clk),
        .rdclk(bb_clk),
        .aclr(reset[0]),
        .q({c1tx_dout_hdr, c1tx_dout_data}),
        .rdusedw(),
        .wrusedw(),
        .rdfull(),
        .rdempty(c1tx_rdempty),
        .wrfull(c1tx_fifo_wrfull),
        .wralmfull(c1tx_almfull),
        .wrempty()
        );

    // Track partial packets to avoid stopping the flow in the middle of a packet,
    // even when c1TxAlmFull is asserted.
    logic c1tx_in_partial_packet;
    logic c1tx_complete_partial_packet;
    // Force completion of the packet when in the middle of a packet and no requests
    // have fired since the partial packet was detected.  The packet will complete
    // with some bubbles due to the pipeline, but this doesn't matter since almost
    // full must have been asserted.
    assign c1tx_complete_partial_packet = c1tx_in_partial_packet &&
                                          ! c1tx_rdreq_q;

    // Forward FIFO toward FIU when there is data and the output channel is available
    assign c1tx_rdreq = ! c1tx_rdempty &&
                        (! bb_rx_q.c1TxAlmFull || c1tx_complete_partial_packet);

    always_ff @(posedge bb_clk)
    begin
        c1tx_rdreq_q <= c1tx_rdreq;
    end

    always_comb
    begin
        bb_tx_next.c1.valid = c1tx_rdreq_q;
        bb_tx_next.c1.hdr = c1tx_dout_hdr;
        bb_tx_next.c1.data = c1tx_dout_data;
    end

    //
    // Track whether the next beat is the start of a packet.
    //
    t_ccip_clNum c1tx_rem_beats;

    always_ff @(posedge bb_clk)
    begin
        if (c1tx_rdreq_q)
        begin
            if (c1tx_in_partial_packet)
            begin
                // In the middle of a packet.  Done when remaining beat count would be
                // zero in the next cycle.
                c1tx_in_partial_packet <= (c1tx_rem_beats != t_ccip_clNum'(1));
                c1tx_rem_beats <= c1tx_rem_beats - t_ccip_clNum'(1);
            end
            else
            begin
                // Is a new multi-beat write starting?
                c1tx_in_partial_packet <= c1tx_dout_hdr.sop &&
                                          (c1tx_dout_hdr.cl_len != eCL_LEN_1) &&
                                          ((c1tx_dout_hdr.req_type == eREQ_WRLINE_I) ||
                                           (c1tx_dout_hdr.req_type == eREQ_WRLINE_M));
                c1tx_rem_beats <= t_ccip_clNum'(c1tx_dout_hdr.cl_len);
            end
        end

        if (reset[0])
        begin
            c1tx_in_partial_packet <= 1'b0;
        end
    end

    // Track round-trip request -> response credits to avoid filling the
    // response pipeline.
    logic [C1RX_DEPTH_RADIX-1:0] c1req_cnt;
    platform_utils_ccip_c1_active_cnt
      #(
        .C1RX_DEPTH_RADIX(C1RX_DEPTH_RADIX)
        )
      c1req_credit_counter
       (
        .clk(afu_clk),
        .reset(afu_softreset),
        .c1Tx(afu_tx_q.c1),
        .c1Rx(afu_rx_next.c1),
        .cnt(c1req_cnt)
        );

    // Maximum number of line requests outstanding is the size of the buffer
    // minus the number of requests that may arrive after asserting almost full,
    // with some wiggle room added for message latency.
    localparam C1_REQ_CREDIT_LIMIT = (2 ** C1RX_DEPTH_RADIX) -
                                     CCIP_TX_ALMOST_FULL_THRESHOLD * 8;
    generate
        if (C1_REQ_CREDIT_LIMIT <= 0)
        begin
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

    always_ff @(posedge afu_clk)
    begin
        afu_rx_next.c1TxAlmFull <= c1tx_almfull ||
                                   (c1req_cnt > C1RX_DEPTH_RADIX'(C1_REQ_CREDIT_LIMIT)) ||
                                   buffer_error;
    end


    /*
     * C2Tx Channel
     */
    logic [C2TX_TOTAL_WIDTH-1:0] c2tx_dout;
    logic                        c2tx_rdreq, c2tx_rdreq_q;
    logic                        c2tx_rdempty;
    logic                        c2tx_fifo_wrfull;

    platform_utils_dc_fifo
      #(
        .DATA_WIDTH(C2TX_TOTAL_WIDTH),
        .DEPTH_RADIX(C2TX_DEPTH_RADIX)
        )
      c2tx_afifo
       (
        .data({afu_tx_q.c2.hdr, afu_tx_q.c2.data}),
        .wrreq(afu_tx_q.c2.mmioRdValid & ! c2tx_fifo_wrfull),
        .rdreq(c2tx_rdreq),
        .wrclk(afu_clk),
        .rdclk(bb_clk),
        .aclr(reset[0]),
        .q(c2tx_dout),
        .rdusedw(),
        .wrusedw(),
        .rdfull(),
        .rdempty(c2tx_rdempty),
        .wrfull(c2tx_fifo_wrfull),
        .wralmfull(),
        .wrempty()
        );

    assign c2tx_rdreq = ! c2tx_rdempty;
    always_ff @(posedge bb_clk)
    begin
        c2tx_rdreq_q <= c2tx_rdreq;
    end

    always_comb
    begin
        bb_tx_next.c2.mmioRdValid = c2tx_rdreq_q;
        {bb_tx_next.c2.hdr, bb_tx_next.c2.data} = c2tx_dout;
    end


    /*
     * C0Rx Channel
     */
    logic [C0RX_TOTAL_WIDTH-1:0] c0rx_dout;
    logic                        c0rx_rdreq, c0rx_rdreq_q;
    logic                        c0rx_rdempty;
    logic                        c0rx_fifo_wrfull;

    platform_utils_dc_fifo
      #(
        .DATA_WIDTH(C0RX_TOTAL_WIDTH),
        .DEPTH_RADIX(C0RX_DEPTH_RADIX)
        )
      c0rx_afifo
       (
        .data({bb_rx_q.c0.hdr, bb_rx_q.c0.data, bb_rx_q.c0.rspValid, bb_rx_q.c0.mmioRdValid, bb_rx_q.c0.mmioWrValid}),
        .wrreq(bb_rx_q.c0.rspValid | bb_rx_q.c0.mmioRdValid |  bb_rx_q.c0.mmioWrValid),
        .rdreq(c0rx_rdreq),
        .wrclk(bb_clk),
        .rdclk(afu_clk),
        .aclr(reset[0]),
        .q(c0rx_dout),
        .rdusedw(),
        .wrusedw(),
        .rdfull(),
        .rdempty(c0rx_rdempty),
        .wrfull(c0rx_fifo_wrfull),
        .wralmfull(),
        .wrempty()
        );

    assign c0rx_rdreq = ! c0rx_rdempty;
    always_ff @(posedge afu_clk)
    begin
        c0rx_rdreq_q <= c0rx_rdreq;
    end

    always_comb
    begin
        { afu_rx_next.c0.hdr, afu_rx_next.c0.data,
          afu_rx_next.c0.rspValid, afu_rx_next.c0.mmioRdValid,
          afu_rx_next.c0.mmioWrValid } = c0rx_dout;

        if (! c0rx_rdreq_q)
        begin
            afu_rx_next.c0.rspValid = 1'b0;
            afu_rx_next.c0.mmioRdValid = 1'b0;
            afu_rx_next.c0.mmioWrValid = 1'b0;
        end
    end


    /*
     * C1Rx Channel
     */
    t_ccip_c1_RspMemHdr c1rx_dout;
    logic               c1rx_rdreq, c1rx_rdreq_q;
    logic               c1rx_rdempty;
    logic               c1rx_fifo_wrfull;

    platform_utils_dc_fifo
      #(
        .DATA_WIDTH(C1RX_TOTAL_WIDTH),
        .DEPTH_RADIX(C1RX_DEPTH_RADIX)
        )
      c1rx_afifo
       (
        .data(bb_rx_q.c1.hdr),
        .wrreq(bb_rx_q.c1.rspValid),
        .rdreq(c1rx_rdreq),
        .wrclk(bb_clk),
        .rdclk(afu_clk),
        .aclr(reset[0]),
        .q(c1rx_dout),
        .rdusedw(),
        .wrusedw(),
        .rdfull(),
        .rdempty(c1rx_rdempty),
        .wrfull(c1rx_fifo_wrfull),
        .wralmfull(),
        .wrempty()
        );

    assign c1rx_rdreq = ! c1rx_rdempty;
    always_ff @(posedge afu_clk)
    begin
        c1rx_rdreq_q <= c1rx_rdreq;
    end

    always_comb
    begin
        afu_rx_next.c1.rspValid = c1rx_rdreq_q;
        afu_rx_next.c1.hdr = c1rx_dout;
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
    always_ff @(posedge afu_clk)
    begin
        if (afu_softreset)
        begin
            async_shim_error[2:0] <= 3'b0;
        end
        else
        begin
            // Hold the error state once set
            async_shim_error[0] <= async_shim_error[0] ||
                                   (c0tx_fifo_wrfull && afu_tx_q.c0.valid);
            async_shim_error[1] <= async_shim_error[1] ||
                                   (c1tx_fifo_wrfull && afu_tx_q.c1.valid);
            async_shim_error[2] <= async_shim_error[2] ||
                                   (c2tx_fifo_wrfull && afu_tx_q.c2.mmioRdValid);
        end
    end


    // FIU-side errors in the bb_clk domain
    (* preserve *) logic [1:0] async_shim_error_bb;

    always_ff @(posedge bb_clk)
    begin
        if (reset[0])
        begin
            async_shim_error_bb <= 2'b0;
        end
        else
        begin
            // Hold the error state once set
            async_shim_error_bb[0] <= async_shim_error_bb[0] ||
                                      (c0rx_fifo_wrfull && (bb_rx_q.c0.rspValid ||
                                                            bb_rx_q.c0.mmioRdValid ||
                                                            bb_rx_q.c0.mmioWrValid));
            async_shim_error_bb[1] <= async_shim_error_bb[1] ||
                                      (c1rx_fifo_wrfull && bb_rx_q.c1.rspValid);
        end
    end

    // Transfer FIU-side errors to the afu_clk domain
    logic [1:0] async_shim_error_afu, async_shim_error_afu_q;

    always_ff @(posedge afu_clk)
    begin
        async_shim_error_afu <= async_shim_error_bb;
        async_shim_error_afu_q <= async_shim_error_afu;
        async_shim_error[4:3] <= async_shim_error_afu_q;
    end


    // synthesis translate_off
    always_ff @(posedge afu_clk)
    begin
        if (async_shim_error[0])
            $warning("** ERROR ** C0Tx dropped transaction");
        if (async_shim_error[1])
            $warning("** ERROR ** C1Tx dropped transaction");
        if (async_shim_error[2])
            $warning("** ERROR ** C2Tx dropped transaction");
    end

    always_ff @(negedge afu_clk)
    begin
        if ((|(async_shim_error[2:0])))
        begin
            $fatal("Aborting due to dropped transaction");
        end
    end

    always_ff @(posedge bb_clk)
    begin
        if (async_shim_error_bb[0])
            $warning("** ERROR ** C0Rx dropped transaction");
        if (async_shim_error_bb[1])
            $warning("** ERROR ** C1Rx dropped transaction");
    end

    always_ff @(negedge bb_clk)
    begin
        if ((|(async_shim_error_bb)))
        begin
            $fatal("Aborting due to dropped transaction");
        end
    end
    // synthesis translate_on


    /*
     * Interface counts
     * - This block is enabled when DEBUG_ENABLE = 1, else disabled
     */
    generate
        if (DEBUG_ENABLE == 1)
        begin
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
            always_ff @(posedge afu_clk)
            begin
                if (afu_softreset)
                begin
                    afu_c0tx_cnt <= 0;
                    afu_c1tx_cnt <= 0;
                    afu_c2tx_cnt <= 0;
                    afu_c0rx_cnt <= 0;
                    afu_c1rx_cnt <= 0;
                end
                else
                begin
                    if (afu_tx_q.c0.valid)
                        afu_c0tx_cnt <= afu_c0tx_cnt + 1;
                    if (afu_tx_q.c1.valid)
                        afu_c1tx_cnt <= afu_c1tx_cnt + 1;
                    if (afu_tx_q.c2.mmioRdValid)
                        afu_c2tx_cnt <= afu_c2tx_cnt + 1;
                    if (afu_rx_next.c0.rspValid|afu_rx_next.c0.mmioRdValid|afu_rx_next.c0.mmioWrValid)
                        afu_c0rx_cnt <= afu_c0rx_cnt + 1;
                    if (afu_rx_next.c1.rspValid)
                        afu_c1rx_cnt <= afu_c1rx_cnt + 1;
                end
            end

            // bb_if counts
            always_ff @(posedge bb_clk)
            begin
                if (reset[0])
                begin
                    bb_c0tx_cnt <= 0;
                    bb_c1tx_cnt <= 0;
                    bb_c2tx_cnt <= 0;
                    bb_c0rx_cnt <= 0;
                    bb_c1rx_cnt <= 0;
                end
                else
                begin
                    if (bb_tx_next.c0.valid)
                        bb_c0tx_cnt <= bb_c0tx_cnt + 1;
                    if (bb_tx_next.c1.valid)
                        bb_c1tx_cnt <= bb_c1tx_cnt + 1;
                    if (bb_tx_next.c2.mmioRdValid)
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
