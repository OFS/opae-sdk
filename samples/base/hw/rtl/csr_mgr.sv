//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
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

`include "cci_mpf_if.vh"
`include "cci_mpf_app_conf_default.vh"
`include "csr_mgr.vh"


//
// Application-independent, generic CSR read and write manager.
//
// The CSR manager implements the required device feature header at
// MMIO address 0, including the AFU ID field.  The AFU ID is passed
// in as a value in the app_csrs SystemVerilog interface parameter.
//

module csr_mgr
  #(
    // The CSR manager implements a feature at MMIO address 0.  If the
    // application requires another feature entry, the MMIO address of
    // the next feature header should be passed in here.  The CSR
    // manager will construct a device feature chain pointing to it.
    parameter NEXT_DFH_BYTE_OFFSET = 0
    )
   (
    input  logic clk,

    // Connection toward the host.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    // CCI-P power and error wires
    input  logic [1:0] pck_cp2af_pwrState,
    input  logic pck_cp2af_error,

    // CSR connections to application code.
    app_csrs.csr csrs
    );

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    // Most wires flow straight through and are, at most, read in this shim.
    assign fiu.c0Tx = afu.c0Tx;
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull;
    assign fiu.c1Tx = afu.c1Tx;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull;

    assign afu.c0Rx = fiu.c0Rx;
    assign afu.c1Rx = fiu.c1Rx;

    // Use a small index space for the local CSRs.
    localparam MAX_CSR_IDX = 32 + NUM_APP_CSRS;
    typedef logic [$clog2(MAX_CSR_IDX)-1 : 0] t_afu_csr_idx;

    t_if_cci_c0_Rx c0Rx;
    t_if_cci_c1_Rx c1Rx;

    // Address of incoming MMIO request, truncated to only the bits needed
    // to compute local CSR addresses.
    t_cci_mmioAddr mmio_req_addr;
    assign mmio_req_addr = cci_csr_getAddress(c0Rx);
      
    // TID of incoming MMIO request
    t_ccip_tid mmio_req_tid;
    assign mmio_req_tid = cci_csr_getTid(c0Rx);

    // Is the incoming MMIO address in the range of CSRs here?
    logic mmio_req_addr_valid;
    assign mmio_req_addr_valid = (mmio_req_addr < 2 * MAX_CSR_IDX);

    // CSR read from a valid address?
    logic is_csr_read;
    assign is_csr_read = cci_csr_isRead(c0Rx) && mmio_req_addr_valid;

    // CSR write to a valid address?
    logic is_csr_write;
    assign is_csr_write = cci_csr_isWrite(c0Rx) && mmio_req_addr_valid;

    // Convert MMIO address to our internal CSR index space (64 bit CSRs)
    t_afu_csr_idx csr_idx;
    assign csr_idx = mmio_req_addr[1 +: $bits(csr_idx)];


    //
    // The common (application-independent) CSR space contains
    // counters available in all applications.
    //

    t_csr_mgr_counter ctr_rd_cache_hits;
    t_csr_mgr_counter ctr_wr_cache_hits;

    // Count reads and writes separately on VL0 but not on PCIe channels.
    // The separate read/write counters can be used along with cache hit
    // statistics for analyzing hit rates of reads vs. writes.
    t_csr_mgr_counter ctr_chan_vl0_rd;
    t_csr_mgr_counter ctr_chan_vl0_wr;
    t_csr_mgr_counter ctr_chan_vh0;
    t_csr_mgr_counter ctr_chan_vh1;

    // Count almost full cycles
    t_csr_mgr_counter ctr_rd_almFull;
    t_csr_mgr_counter ctr_wr_almFull;

    // Was a CCI-P error raised at any time?
    logic error_raised;

    // Power threshold error events.  These are saturating counters.
    logic [15:0] pwr_events_ap1;    // 50% reduction
    logic [15:0] pwr_events_ap2;    // 90% reduction


    // Register incoming messages
    always_ff @(posedge clk)
    begin
        c0Rx <= fiu.c0Rx;
        c1Rx <= fiu.c1Rx;
    end


    //
    // CSR reads
    //
    t_if_cci_c2_Tx c2Tx;
    always_ff @(posedge clk)
    begin
        // Forward MMIO read responses from other modules closer to the AFU.
        fiu.c2Tx <= afu.c2Tx;

        // Pass local read responses toward the host.  This code has a race
        // if two MMIO read requests are outstanding at the same time, since
        // the local response will overwrite the value forwarded from afu.c2Tx.
        // Our sample applications never have two MMIO reads outstanding
        // simulatenously, so we leave the hardware here simple.
        if (c2Tx.mmioRdValid)
        begin
            fiu.c2Tx <= c2Tx;
        end
    end

    //
    // Generate read responses to local register c2Tx, which is forwarded
    // in the next cycle to fiu.c2Tx above.
    //
    always_ff @(posedge clk)
    begin
        c2Tx.mmioRdValid <= is_csr_read;
        c2Tx.hdr.tid <= mmio_req_tid;

        case (csr_idx) inside
          0: // AFU DFH (device feature header)
            begin
                // Construct the DFH (CSR 0)
                t_ccip_dfh afu_dfh;
                afu_dfh = ccip_dfh_defaultDFH();
                afu_dfh.f_type = eFTYP_AFU;
                afu_dfh.eol = (NEXT_DFH_BYTE_OFFSET == 0);
                afu_dfh.nextFeature = NEXT_DFH_BYTE_OFFSET;

                c2Tx.data <= afu_dfh;
            end

          // AFU_ID_L
          //   The application must assign the same AFU ID value to
          //   csrs.afu_id every cycle.
          1: c2Tx.data <= csrs.afu_id[63:0];

          // AFU_ID_H
          2: c2Tx.data <= csrs.afu_id[127:64];

          // DFH_RSVD0
          3: c2Tx.data <= t_ccip_mmioData'(0);

          // DFH_RSVD1
          4: c2Tx.data <= t_ccip_mmioData'(0);

          //
          // Standard CSRs available in all applications.  These CSR
          // indices must correspond to CSR_COMMON in csr_mgr.h.
          //

          // AFU frequency (MHz)
          8: c2Tx.data <= `AFU_CLOCK_FREQ;

          // Cache read hits
          9: c2Tx.data <= ctr_rd_cache_hits;

          // Cache write hits
          10: c2Tx.data <= ctr_wr_cache_hits;

          // Responses on VL0/VH0/VH1
          11: c2Tx.data <= ctr_chan_vl0_rd;
          12: c2Tx.data <= ctr_chan_vl0_wr;
          13: c2Tx.data <= ctr_chan_vh0;
          14: c2Tx.data <= ctr_chan_vh1;

          // FIU state
          15: c2Tx.data <= { pwr_events_ap2,    // [63:48]
                             pwr_events_ap1,    // [47:32]
                             29'(0),
                             error_raised,      // [2]
                             fiu.c1TxAlmFull,   // [1]
                             fiu.c0TxAlmFull }; // [0]

          // Almost full counters
          16: c2Tx.data <= ctr_rd_almFull;
          17: c2Tx.data <= ctr_wr_almFull;

          // Application CSRs start at 32
          [32 : (32 + NUM_APP_CSRS - 1)]:
            begin
                c2Tx.data <= csrs.cpu_rd_csrs[csr_idx - 32].data;
            end

           default:
             begin
                c2Tx.data <= t_ccip_mmioData'('x);
             end
        endcase

        if (reset)
        begin
            c2Tx.mmioRdValid <= 1'b0;
        end
    end


    logic [63:0] csr_wr_data;
    assign csr_wr_data = 64'(c0Rx.data);

    //
    // CSR writes from the host
    //
    always_ff @(posedge clk)
    begin
        for (int i = 0; i < NUM_APP_CSRS; i = i + 1)
        begin
            // Fan out data to all targets
            csrs.cpu_wr_csrs[i].data <= 64'(c0Rx.data);
            // Set en for one cycle when the index matches.
            csrs.cpu_wr_csrs[i].en <= (t_afu_csr_idx'(i + 32) == csr_idx) &&
                                      is_csr_write;
        end
    end


    //
    // Track common counters
    //

    logic rd_is_vl0;
    logic rd_is_vl0_q;
    assign rd_is_vl0 = ccip_c0Rx_isReadRsp(c0Rx) && (c0Rx.hdr.vc_used == eVC_VL0);
    logic wr_is_vl0;
    logic [2:0] wr_cnt_vl0_q;
    assign wr_is_vl0 = ccip_c1Rx_isWriteRsp(c1Rx) && (c1Rx.hdr.vc_used == eVC_VL0);

    logic rd_is_vh0;
    logic rd_is_vh0_q;
    assign rd_is_vh0 = ccip_c0Rx_isReadRsp(c0Rx) && (c0Rx.hdr.vc_used == eVC_VH0);
    logic wr_is_vh0;
    logic [2:0] wr_cnt_vh0_q;
    assign wr_is_vh0 = ccip_c1Rx_isWriteRsp(c1Rx) && (c1Rx.hdr.vc_used == eVC_VH0);

    logic rd_is_vh1;
    logic rd_is_vh1_q;
    assign rd_is_vh1 = ccip_c0Rx_isReadRsp(c0Rx) && (c0Rx.hdr.vc_used == eVC_VH1);
    logic wr_is_vh1;
    logic [2:0] wr_cnt_vh1_q;
    assign wr_is_vh1 = ccip_c1Rx_isWriteRsp(c1Rx) && (c1Rx.hdr.vc_used == eVC_VH1);

    // Number of lines in write response
    logic [2:0] wr_rsp_len;
    assign wr_rsp_len = 3'(1) + (c1Rx.hdr.format ? 3'(c1Rx.hdr.cl_num) : 3'(0));

    logic c0TxAlmFull, c1TxAlmFull;

    always_ff @(posedge clk)
    begin
        if (ccip_c0Rx_isReadRsp(c0Rx) && c0Rx.hdr.hit_miss)
        begin
            ctr_rd_cache_hits <= ctr_rd_cache_hits + t_csr_mgr_counter'(1);
        end

        if (ccip_c1Rx_isWriteRsp(c1Rx) && c1Rx.hdr.hit_miss)
        begin
            ctr_wr_cache_hits <= ctr_wr_cache_hits +
                                 t_csr_mgr_counter'(wr_rsp_len);
        end

        rd_is_vl0_q <= rd_is_vl0;
        wr_cnt_vl0_q <= (wr_is_vl0 ? wr_rsp_len : 3'(0));
        ctr_chan_vl0_rd <= ctr_chan_vl0_rd + t_csr_mgr_counter'(rd_is_vl0_q);
        ctr_chan_vl0_wr <= ctr_chan_vl0_wr + t_csr_mgr_counter'(wr_cnt_vl0_q);

        rd_is_vh0_q <= rd_is_vh0;
        wr_cnt_vh0_q <= (wr_is_vh0 ? wr_rsp_len : 3'(0));
        ctr_chan_vh0 <= ctr_chan_vh0 + t_csr_mgr_counter'(3'(rd_is_vh0_q) +
                                                           wr_cnt_vh0_q);

        rd_is_vh1_q <= rd_is_vh1;
        wr_cnt_vh1_q <= (wr_is_vh1 ? wr_rsp_len : 3'(0));
        ctr_chan_vh1 <= ctr_chan_vh1 + t_csr_mgr_counter'(3'(rd_is_vh1_q) +
                                                           wr_cnt_vh1_q);

        c0TxAlmFull <= fiu.c0TxAlmFull;
        ctr_rd_almFull <= ctr_rd_almFull + t_csr_mgr_counter'(c0TxAlmFull);
        c1TxAlmFull <= fiu.c1TxAlmFull;
        ctr_wr_almFull <= ctr_wr_almFull + t_csr_mgr_counter'(c1TxAlmFull);

        if (reset)
        begin
            ctr_rd_cache_hits <= t_csr_mgr_counter'(0);
            ctr_wr_cache_hits <= t_csr_mgr_counter'(0);
            ctr_chan_vl0_rd <= t_csr_mgr_counter'(0);
            ctr_chan_vl0_wr <= t_csr_mgr_counter'(0);
            ctr_chan_vh0 <= t_csr_mgr_counter'(0);
            ctr_chan_vh1 <= t_csr_mgr_counter'(0);
            ctr_rd_almFull <= t_csr_mgr_counter'(0);
            ctr_wr_almFull <= t_csr_mgr_counter'(0);

            rd_is_vl0_q <= 1'b0;
            wr_cnt_vl0_q <= 3'b0;
            rd_is_vh0_q <= 1'b0;
            wr_cnt_vh0_q <= 3'b0;
            rd_is_vh1_q <= 1'b0;
            wr_cnt_vh1_q <= 3'b0;
        end
    end


    // Sticky error flag
    always_ff @(posedge clk)
    begin
        error_raised <= error_raised || pck_cp2af_error;

        if (reset)
        begin
            error_raised <= 1'b0;
        end
    end

    // Power reduction required?  Count transitions, not cycles.
    logic pwr_event_ap1_active, pwr_event_ap2_active;

    always_ff @(posedge clk)
    begin
        // New AP1 raised and the counter isn't saturated
        if ((pck_cp2af_pwrState == 2'h1) && ! pwr_event_ap1_active &&
            ! (&(pwr_events_ap1)))
        begin
            pwr_events_ap1 <= pwr_events_ap1 + 1;
        end
        pwr_event_ap1_active <= (pck_cp2af_pwrState == 2'h1);

        // New AP2 raised and the counter isn't saturated
        if ((pck_cp2af_pwrState == 2'h3) && ! pwr_event_ap2_active &&
            ! (&(pwr_events_ap2)))
        begin
            pwr_events_ap2 <= pwr_events_ap2 + 1;
        end
        pwr_event_ap2_active <= (pck_cp2af_pwrState == 2'h3);

        if (reset)
        begin
            pwr_events_ap1 <= 0;
            pwr_events_ap2 <= 0;
        end
    end

endmodule // csr_mgr
