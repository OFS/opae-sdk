//
// Copyright (c) 2016, Intel Corporation
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
`include "cci_mpf_csrs.vh"

//
// There is a single CSR (MMIO read/write) manager in MPF, shared by all
// shims.  We do this because the required buffering is large enough to be
// worth sharing across all shims.  When a shim is not present in a
// system the corresponding CSRs have no meaning.
//

// MMIO address range of MPF CSRs
parameter CCI_MPF_CSR_SIZE = CCI_MPF_MMIO_SIZE;

// Size in 64 bit words
parameter CCI_MPF_CSR_SIZE64 = (CCI_MPF_CSR_SIZE >> 3);

// Type for holding MPF CSR address as an offset from DFH_MMIO_BASE_ADDR
typedef logic [$clog2(CCI_MPF_CSR_SIZE64)-1:0] t_mpf_csr_offset;

// Offset of each shim's CSR range from feature list start.  This is
// similar to base addresses above, but the origin is the first feature
// managed by MPF.
parameter CCI_MPF_VTP_CSR_OFFSET = 0;
parameter CCI_MPF_RSP_ORDER_CSR_OFFSET =   CCI_MPF_VTP_CSR_OFFSET +
                                           CCI_MPF_VTP_CSR_SIZE;
parameter CCI_MPF_VC_MAP_CSR_OFFSET =      CCI_MPF_RSP_ORDER_CSR_OFFSET +
                                           CCI_MPF_RSP_ORDER_CSR_SIZE;
parameter CCI_MPF_LATENCY_QOS_CSR_OFFSET = CCI_MPF_VC_MAP_CSR_OFFSET +
                                           CCI_MPF_VC_MAP_CSR_SIZE;
parameter CCI_MPF_WRO_CSR_OFFSET =         CCI_MPF_LATENCY_QOS_CSR_OFFSET +
                                           CCI_MPF_LATENCY_QOS_CSR_SIZE;
parameter CCI_MPF_PWRITE_CSR_OFFSET =      CCI_MPF_WRO_CSR_OFFSET +
                                           CCI_MPF_WRO_CSR_SIZE;

// Size of the intermediate statistics counter bucket. These buckets
// are added periodically to the CSR memory by cci_mpf_shim_csr.
parameter CCI_MPF_STAT_CNT_WIDTH = 16;
typedef logic [CCI_MPF_STAT_CNT_WIDTH-1:0] t_cci_mpf_stat_cnt;

parameter CCI_MPF_CSR_NUM_STATS = 12;
typedef t_mpf_csr_offset [0:CCI_MPF_CSR_NUM_STATS-1] t_stat_csr_offset_vec;
typedef t_cci_mpf_stat_cnt [0:CCI_MPF_CSR_NUM_STATS-1] t_stat_upd_count_vec;


module cci_mpf_shim_csr
  #(
    // Instance ID reported in feature IDs of all device feature
    // headers instantiated under this instance of MPF.  If only a single
    // MPF instance is instantiated in the AFU then leaving the instance
    // ID at 1 is probably the right choice.
    parameter MPF_INSTANCE_ID = 1,

    // MMIO base address (byte level) allocated to MPF for feature lists
    // and CSRs.  The AFU allocating this module must build at least
    // a device feature header (DFH) for the AFU.  The chain of device
    // features in the AFU must then point to the base address here
    // as another feature in the chain.  MPF will continue the list.
    // The base address here must point to a region that is at least
    // CCI_MPF_MMIO_SIZE bytes.
    parameter DFH_MMIO_BASE_ADDR = 0,

    // Address of the next device feature header outside MPF.  MPF will
    // terminate the feature list if the next address is 0.
    parameter DFH_MMIO_NEXT_ADDR = 0,

    // Are shims enabled?
    parameter MPF_ENABLE_VTP = 0,
    parameter MPF_ENABLE_RSP_ORDER = 0,
    parameter MPF_ENABLE_VC_MAP = 0,
    parameter MPF_ENABLE_LATENCY_QOS = 0,
    parameter MPF_ENABLE_WRO = 0,
    parameter MPF_ENABLE_PWRITE = 0
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    // CSR connections to other shims
    cci_mpf_csrs.csr csrs,
    cci_mpf_csrs.csr_events events
    );

    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    // Most connections flow straight through and are, at most, read in this shim.
    assign fiu.c0Tx = afu.c0Tx;
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull;
    assign fiu.c1Tx = afu.c1Tx;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull;

    assign afu.c0Rx = fiu.c0Rx;
    assign afu.c1Rx = fiu.c1Rx;

    localparam CCI_MPF_CSR_LAST = DFH_MMIO_BASE_ADDR + CCI_MPF_CSR_SIZE;

    // Base address of each shim's CSR range
    localparam CCI_MPF_VTP_CSR_BASE = DFH_MMIO_BASE_ADDR;
    localparam CCI_MPF_RSP_ORDER_CSR_BASE =   CCI_MPF_VTP_CSR_BASE +
                                              CCI_MPF_VTP_CSR_SIZE;
    localparam CCI_MPF_VC_MAP_CSR_BASE =      CCI_MPF_RSP_ORDER_CSR_BASE +
                                              CCI_MPF_RSP_ORDER_CSR_SIZE;
    localparam CCI_MPF_LATENCY_QOS_CSR_BASE = CCI_MPF_VC_MAP_CSR_BASE +
                                              CCI_MPF_VC_MAP_CSR_SIZE;
    localparam CCI_MPF_WRO_CSR_BASE =         CCI_MPF_LATENCY_QOS_CSR_BASE +
                                              CCI_MPF_LATENCY_QOS_CSR_SIZE;
    localparam CCI_MPF_PWRITE_CSR_BASE =      CCI_MPF_WRO_CSR_BASE +
                                              CCI_MPF_WRO_CSR_SIZE;


    // Register incoming requests
    t_if_cci_c0_Rx c0_rx;
    always_ff @(posedge clk)
    begin
        c0_rx <= fiu.c0Rx;
    end


    // ====================================================================
    //
    //  CSR writes from host to FPGA
    //
    // ====================================================================

    // Check for a CSR address match
    function automatic logic csrAddrMatches(
        input t_if_cci_c0_Rx c0Rx,
        input int c);

        // Target address.  The CSR space is 4-byte addressable.  The
        // low 2 address bits must be 0 and aren't transmitted.
        t_cci_mmioAddr tgt = t_cci_mmioAddr'(c >> 2);

        // Actual address sent in CSR write.
        t_cci_mmioAddr addr = cci_csr_getAddress(c0Rx);

        return cci_csr_isWrite(c0Rx) && (addr == tgt);
    endfunction

    //
    // VTP CSR writes (host to FPGA)
    //
    always_ff @(posedge clk)
    begin
        // Momentary data set unconditionally to avoid control logic.
        csrs.vc_map_ctrl <= c0_rx.data[63:0];
        csrs.latency_qos_ctrl <= c0_rx.data[63:0];
        csrs.wro_ctrl <= c0_rx.data[63:0];

        csrs.vc_map_ctrl_valid <=
            csrAddrMatches(c0_rx, CCI_MPF_VC_MAP_CSR_BASE +
                                  CCI_MPF_VC_MAP_CSR_CTRL_REG);

        csrs.latency_qos_ctrl_valid <=
            csrAddrMatches(c0_rx, CCI_MPF_LATENCY_QOS_CSR_BASE +
                                  CCI_MPF_LATENCY_QOS_CSR_CTRL_REG);

        csrs.wro_ctrl_valid <=
            csrAddrMatches(c0_rx, CCI_MPF_WRO_CSR_BASE +
                                  CCI_MPF_WRO_CSR_CTRL_REG);


        if (csrAddrMatches(c0_rx, CCI_MPF_VTP_CSR_BASE +
                                  CCI_MPF_VTP_CSR_MODE))
        begin
            csrs.vtp_in_mode <= t_cci_mpf_vtp_csr_mode'(c0_rx.data);
        end
        else
        begin
            // Invalidate page table held only one cycle
            csrs.vtp_in_mode.inval_translation_cache <= 1'b0;
        end

        if (csrAddrMatches(c0_rx, CCI_MPF_VTP_CSR_BASE +
                                  CCI_MPF_VTP_CSR_PAGE_TABLE_PADDR))
        begin
            csrs.vtp_in_page_table_base <= t_cci_clAddr'(c0_rx.data);
            csrs.vtp_in_page_table_base_valid <= 1'b1;
        end

        // Inval page held only one cycle
        csrs.vtp_in_inval_page <= t_cci_clAddr'(c0_rx.data);
        csrs.vtp_in_inval_page_valid <=
            csrAddrMatches(c0_rx, CCI_MPF_VTP_CSR_BASE +
                                  CCI_MPF_VTP_CSR_INVAL_PAGE_VADDR);

        if (reset)
        begin
            csrs.vtp_in_mode <= t_cci_mpf_vtp_csr_mode'(0);
            csrs.vtp_in_page_table_base_valid <= 1'b0;
            csrs.vtp_in_inval_page_valid <= 1'b0;
            csrs.vc_map_ctrl_valid <= 1'b0;
            csrs.latency_qos_ctrl_valid <= 1'b0;
            csrs.wro_ctrl_valid <= 1'b0;
        end
    end


    // ====================================================================
    //
    //  CSR reads from host
    //
    // ====================================================================

    //
    // Hold CSR read response state in memory
    //
    logic csr_mem_rd_en;
    t_mpf_csr_offset csr_mem_rd_idx;
    logic csr_mem_rd_rdy;

    logic [63:0] csr_mem_rd_val;
    logic csr_mem_rd_val_valid;

    logic csr_mem_wr_en;
    logic [63:0] csr_mem_wr_val;
    t_mpf_csr_offset csr_mem_wr_idx;
    logic csr_mem_wr_rdy;

    cci_mpf_shim_csr_rd_memory
      #(
        .MPF_INSTANCE_ID(MPF_INSTANCE_ID),
        .DFH_MMIO_NEXT_ADDR(DFH_MMIO_NEXT_ADDR),

        .CCI_MPF_VTP_CSR_BASE(CCI_MPF_VTP_CSR_BASE),
        .CCI_MPF_RSP_ORDER_CSR_BASE(CCI_MPF_RSP_ORDER_CSR_BASE),
        .CCI_MPF_VC_MAP_CSR_BASE(CCI_MPF_VC_MAP_CSR_BASE),
        .CCI_MPF_LATENCY_QOS_CSR_BASE(CCI_MPF_LATENCY_QOS_CSR_BASE),
        .CCI_MPF_WRO_CSR_BASE(CCI_MPF_WRO_CSR_BASE),
        .CCI_MPF_PWRITE_CSR_BASE(CCI_MPF_PWRITE_CSR_BASE),
        .MPF_ENABLE_VTP(MPF_ENABLE_VTP),
        .MPF_ENABLE_RSP_ORDER(MPF_ENABLE_RSP_ORDER),
        .MPF_ENABLE_VC_MAP(MPF_ENABLE_VC_MAP),
        .MPF_ENABLE_LATENCY_QOS(MPF_ENABLE_LATENCY_QOS),
        .MPF_ENABLE_WRO(MPF_ENABLE_WRO),
        .MPF_ENABLE_PWRITE(MPF_ENABLE_PWRITE)
        )
      rd_mem
       (
        .clk,
        .reset,

        .csr_mem_rd_en,
        .csr_mem_rd_idx,
        .csr_mem_rd_rdy,

        .csr_mem_rd_val,
        .csr_mem_rd_val_valid,

        .csr_mem_wr_en,
        .csr_mem_wr_idx,
        .csr_mem_wr_val,
        .csr_mem_wr_rdy
        );


    //
    // Statistics counters
    //
    logic stat_upd_rdy;
    logic stat_upd_active;
    logic stat_upd_en;
    t_stat_csr_offset_vec stat_upd_offset_vec;
    t_stat_upd_count_vec stat_upd_count_vec;

    cci_mpf_shim_csr_events
      stats
       (
        .clk,
        .reset,

        .stat_upd_rdy,
        .stat_upd_en,
        .stat_upd_offset_vec,
        .stat_upd_count_vec,

        .events
        );


    t_if_ccip_c2_Tx c2_rsp;
    logic c2_rsp_en;

    // Forward responses to host, either generated locally (c2_rsp) or from
    // the AFU.
    always_ff @(posedge clk)
    begin
        fiu.c2Tx <= (c2_rsp_en ? c2_rsp : afu.c2Tx);
    end

    logic mmio_req_valid;
    t_mpf_csr_offset mmio_req_addr;
    t_ccip_tid mmio_req_tid;

    // New MMIO read request?
    logic mmio_read_start;
    logic mmio_read_active;
    assign mmio_read_start = mmio_req_valid && ! mmio_read_active &&
                             csr_mem_rd_rdy && ! stat_upd_active;

    // Give priority to existing MMIO responses from the AFU
    assign c2_rsp_en = ! afu.c2Tx.mmioRdValid && c2_rsp.mmioRdValid;

    always_ff @(posedge clk)
    begin
        if (c2_rsp_en)
        begin
            // Read response forwarded to host
            c2_rsp.mmioRdValid <= 1'b0;
            mmio_read_active <= 1'b0;
        end
        else
        begin
            if (mmio_read_start)
            begin
                // New MMIO read request.  Request the value of the register.
                case (mmio_req_addr)
                 (CCI_MPF_VTP_CSR_OFFSET + CCI_MPF_VTP_CSR_STAT_PT_WALK_LAST_VADDR) >> 3:
                    begin
                        // VC MAP state history register
                        c2_rsp.data <= events.vtp_out_pt_walk_last_vaddr;
                        c2_rsp.mmioRdValid <= 1'b1;
                    end

                 (CCI_MPF_VC_MAP_CSR_OFFSET + CCI_MPF_VC_MAP_CSR_STAT_HISTORY) >> 3:
                    begin
                        // VC MAP state history register
                        c2_rsp.data <= csrs.vc_map_history;
                        c2_rsp.mmioRdValid <= 1'b1;
                    end

                 default:
                    // Most requests are to counters in block RAM.
                    mmio_read_active <= 1'b1;
                endcase

                c2_rsp.hdr.tid <= mmio_req_tid;
            end

            if (mmio_read_active && ! c2_rsp.mmioRdValid && csr_mem_rd_val_valid)
            begin
                // Got the CSR value.  Store it in the output buffer.
                c2_rsp.mmioRdValid <= 1'b1;
                c2_rsp.data <= csr_mem_rd_val;
            end
        end

        if (reset)
        begin
            c2_rsp.mmioRdValid <= 1'b0;
            mmio_read_active <= 1'b0;
        end
    end


    //
    // This platform has MMIO.  Up to 64 MMIO reads may be in flight.
    // Buffer incoming read requests since the read response port
    // contends with other responders.
    //

    logic mmio_req_enq_en;

    // Address of incoming request
    t_cci_mmioAddr mmio_req_addr_in;
    assign mmio_req_addr_in = cci_csr_getAddress(c0_rx);

    t_cci_mmioAddr mmio_req_addr_in_offset;
    assign mmio_req_addr_in_offset = mmio_req_addr_in -
                                     t_cci_mmioAddr'(DFH_MMIO_BASE_ADDR >> 2);

    // Store incoming requests only if the address is possibly in range
    assign mmio_req_enq_en = cci_csr_isRead(c0_rx) &&
                             mmio_req_addr_in >= (DFH_MMIO_BASE_ADDR >> 2) &&
                             mmio_req_addr_in < (CCI_MPF_CSR_LAST >> 2);

    // Register FIFO input for timing
    logic [$bits(t_mpf_csr_offset) + CCIP_TID_WIDTH - 1 : 0] req_fifo_in;
    logic req_fifo_in_en;

    always_ff @(posedge clk)
    begin
        // Offset comes in as an index to 32 bit words.  Convert it to a 64
        // bit word index, which is all VTP uses for CSR addresses.
        req_fifo_in <= { mmio_req_addr_in_offset[1 +: $bits(t_mpf_csr_offset)],
                         cci_csr_getTid(c0_rx) };
        req_fifo_in_en <= mmio_req_enq_en;
    end

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_mpf_csr_offset) + CCIP_TID_WIDTH),
        .N_ENTRIES(64),
        .REGISTER_OUTPUT(1)
        )
      req_fifo
        (
         .clk,
         .reset,
         // Store only the MMIO address bits needed for decode
         .enq_data(req_fifo_in),
         .enq_en(req_fifo_in_en),
         .notFull(),
         .almostFull(),
         .first({mmio_req_addr, mmio_req_tid}),
         .deq_en(mmio_read_start),
         .notEmpty(mmio_req_valid)
         );


    enum logic [2:0] {
        STAT_IDLE,
        STAT_PROCESS_VEC,
        STAT_UPDATE0,
        STAT_UPDATE1,
        STAT_WRITEBACK
    }
    stat_upd_state;

    t_stat_csr_offset_vec stat_upd_offsets;
    t_stat_upd_count_vec stat_upd_counts;

    logic stat_bucket_upd;
    logic [63:0] stat_bucket_wr_val;
    logic stat_bucket_overflow;
    logic [31:0] csr_mem_rd_high;

    //
    // State machine for processing statistics updates.
    //
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            stat_upd_state <= STAT_IDLE;
            stat_upd_rdy <= 1'b1;
            stat_upd_active <= 1'b0;
        end
        else
        begin
            case (stat_upd_state)
              STAT_IDLE:
                begin
                    if (stat_upd_en)
                    begin
                        // Update statistics counters from vector of
                        // small intermediate counters requested by stats
                        // module.
                        stat_upd_state <= STAT_PROCESS_VEC;
                        stat_upd_rdy <= 1'b0;
                    end

                    stat_upd_offsets <= stat_upd_offset_vec;
                    stat_upd_counts <= stat_upd_count_vec;
                end

              STAT_PROCESS_VEC:
                begin
                    // Is the CSR memory available for reading?
                    if (stat_bucket_upd)
                    begin
                        // Started the read for one entry
                        stat_upd_state <= STAT_UPDATE0;
                        stat_upd_active <= 1'b1;
                    end
                end

              STAT_UPDATE0:
                begin
                    // Wait for CSR memory read response
                    if (csr_mem_rd_val_valid)
                    begin
                        stat_upd_state <= STAT_UPDATE1;
                    end

                    // Low half of update plus overflow.  stat_upd_counts
                    // must be <= 32 bits.
                    { stat_bucket_overflow, stat_bucket_wr_val[31:0] } <=
                        { 1'b0, csr_mem_rd_val[31:0] } + 33'(stat_upd_counts[0]);

                    // Preserve the upper half of the current CSR value
                    csr_mem_rd_high <= csr_mem_rd_val[63:32];
                end

              STAT_UPDATE1:
                begin
                    // High half of update -- just overflow.
                    stat_upd_state <= STAT_WRITEBACK;

                    stat_bucket_wr_val[63:32] <=
                        csr_mem_rd_high + 32'(stat_bucket_overflow);
                end

              STAT_WRITEBACK:
                begin
                    if (csr_mem_wr_en)
                    begin
                        // Writeback complete.  Either process the next
                        // bucket or done with current update list.
                        stat_upd_active <= 1'b0;

                        if (stat_upd_offsets[1] != t_mpf_csr_offset'(0))
                        begin
                            stat_upd_state <= STAT_PROCESS_VEC;
                        end
                        else
                        begin
                            stat_upd_state <= STAT_IDLE;
                            stat_upd_rdy <= 1'b1;
                        end

                        // Shift the incremental counter update vector
                        for (int s = 0; s < CCI_MPF_CSR_NUM_STATS - 1; s = s + 1)
                        begin
                            stat_upd_offsets[s] <= stat_upd_offsets[s + 1];
                            stat_upd_counts[s] <= stat_upd_counts[s + 1];
                        end

                        stat_upd_offsets[CCI_MPF_CSR_NUM_STATS-1] <=
                            t_mpf_csr_offset'(0);
                    end
                end
            endcase
        end
    end

    assign stat_bucket_upd = csr_mem_rd_rdy && ! mmio_req_valid && 
                             (stat_upd_state == STAT_PROCESS_VEC);

    assign csr_mem_rd_en = mmio_read_start || stat_bucket_upd;
    assign csr_mem_rd_idx = mmio_read_start ? mmio_req_addr : stat_upd_offsets[0];

    assign csr_mem_wr_en = (stat_upd_state == STAT_WRITEBACK) && csr_mem_wr_rdy;
    assign csr_mem_wr_idx = stat_upd_offsets[0];
    assign csr_mem_wr_val = stat_bucket_wr_val;

endmodule // cci_mpf_shim_csr


module cci_mpf_shim_csr_events
   (
    input  logic clk,
    input  logic reset,

    // cci_mpf_shim_csr ready to accept updated counts?
    input  logic stat_upd_rdy,
    // Update counts
    output logic stat_upd_en,
    // Indices of CSR read register being updated
    output t_stat_csr_offset_vec stat_upd_offset_vec,
    // Counts to add to corresponding CSR read register
    output t_stat_upd_count_vec stat_upd_count_vec,

    cci_mpf_csrs.csr_events events
    );

    logic consume_counters;


    // ====================================================================
    //
    //  Write local counter updates to CSR read memory periodically
    //
    // ====================================================================

    logic [10:0] upd_counts;
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            upd_counts <= 10'b0;
        end
        else if (consume_counters)
        begin
            upd_counts <= 10'b0;
        end
        else
        begin
            upd_counts <= upd_counts + 10'b1;
        end
    end

    assign stat_upd_en = consume_counters;
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            consume_counters <= 1'b0;
        end
        else
        begin
            consume_counters <= (upd_counts[10] == 1'b1) && stat_upd_rdy;
        end
    end


    // ====================================================================
    //
    //  Update counters from event triggers.
    //
    // ====================================================================

    // Define a macro for updating a counter's register accumulator.
    // Each counter gets a small register accumulator which is added
    // to the main block-RAM counter after some timeout.

    `define MPF_CSR_STAT_ACCUM(FEATURE, IDX, CSR_OFFSET, NAME, EVENT) \
        t_cci_mpf_stat_cnt NAME; \
        \
        /* Map the counter to its position in the block RAM and CSR */ \
        /* address space. */ \
        assign stat_upd_offset_vec[IDX] = \
            t_mpf_csr_offset'((CCI_MPF_``FEATURE``_CSR_OFFSET + \
                               CCI_MPF_``FEATURE``_CSR_STAT_``CSR_OFFSET) >> 3); \
        assign stat_upd_count_vec[IDX] = NAME; \
        \
        /* The "_cur" version is either the accumulator's value or 0 if */ \
        /* the accumulators are consumed this cycle for writing to BRAM. */ \
        t_cci_mpf_stat_cnt NAME``_cur; \
        assign NAME``_cur = (consume_counters ? t_cci_mpf_stat_cnt'(0) : NAME); \
        \
        logic NAME``_incr; \
        always_ff @(posedge clk) \
        begin \
            if (reset) \
            begin \
                NAME``_incr <= 1'b0; \
                NAME`` <= t_cci_mpf_stat_cnt'(0); \
            end \
            else \
            begin \
                NAME``_incr <= events.``EVENT; \
                NAME`` <= NAME``_cur + t_cci_mpf_stat_cnt'(NAME``_incr); \
            end \
        end

    // + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
    //
    // The number of accumulators here should match CCI_MPF_CSR_NUM_STATS.
    //
    // + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +

    `MPF_CSR_STAT_ACCUM(VTP, 0, 4KB_TLB_NUM_HITS, vtp_4kb_hits, vtp_out_event_4kb_hit)
    `MPF_CSR_STAT_ACCUM(VTP, 1, 4KB_TLB_NUM_MISSES, vtp_4kb_misses, vtp_out_event_4kb_miss)
    `MPF_CSR_STAT_ACCUM(VTP, 2, 2MB_TLB_NUM_HITS, vtp_2mb_hits, vtp_out_event_2mb_hit)
    `MPF_CSR_STAT_ACCUM(VTP, 3, 2MB_TLB_NUM_MISSES, vtp_2mb_misses, vtp_out_event_2mb_miss)
    `MPF_CSR_STAT_ACCUM(VTP, 4, PT_WALK_BUSY_CYCLES, vtp_pt_walk_busy_cycles, vtp_out_event_pt_walk_busy)
    `MPF_CSR_STAT_ACCUM(VTP, 5, FAILED_TRANSLATIONS, vtp_failed_translations, vtp_out_event_failed_translation)

    `MPF_CSR_STAT_ACCUM(VC_MAP, 6, NUM_MAPPING_CHANGES, vc_map_mapping_changes, vc_map_out_event_mapping_changed)

    `MPF_CSR_STAT_ACCUM(WRO,  7, RR_CONFLICT, wro_rr_conflicts, wro_out_event_rr_conflict)
    `MPF_CSR_STAT_ACCUM(WRO,  8, RW_CONFLICT, wro_rw_conflicts, wro_out_event_rw_conflict)
    `MPF_CSR_STAT_ACCUM(WRO,  9, WR_CONFLICT, wro_wr_conflicts, wro_out_event_wr_conflict)
    `MPF_CSR_STAT_ACCUM(WRO, 10, WW_CONFLICT, wro_ww_conflicts, wro_out_event_ww_conflict)

    `MPF_CSR_STAT_ACCUM(PWRITE, 11, NUM_PWRITES, pwrite_num_pwrites, pwrite_out_event_pwrite)

endmodule // cci_mpf_shim_csr_events


//
// Manage the backing storage for CSR reads.  The memory holds both device
// feature headers and other state, such as statistics tracking performance
// of the features.
//
module cci_mpf_shim_csr_rd_memory
  #(
    parameter MPF_INSTANCE_ID = 1,
    parameter DFH_MMIO_NEXT_ADDR = 0,

    parameter CCI_MPF_VTP_CSR_BASE = 0,
    parameter CCI_MPF_RSP_ORDER_CSR_BASE = 0,
    parameter CCI_MPF_VC_MAP_CSR_BASE = 0,
    parameter CCI_MPF_LATENCY_QOS_CSR_BASE = 0,
    parameter CCI_MPF_WRO_CSR_BASE = 0,
    parameter CCI_MPF_PWRITE_CSR_BASE = 0,
    parameter MPF_ENABLE_VTP = 0,
    parameter MPF_ENABLE_RSP_ORDER = 0,
    parameter MPF_ENABLE_VC_MAP = 0,
    parameter MPF_ENABLE_LATENCY_QOS = 0,
    parameter MPF_ENABLE_WRO = 0,
    parameter MPF_ENABLE_PWRITE = 0
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic csr_mem_rd_en,
    input  t_mpf_csr_offset csr_mem_rd_idx,
    output logic csr_mem_rd_rdy,

    output logic [63:0] csr_mem_rd_val,
    output logic csr_mem_rd_val_valid,

    input  logic csr_mem_wr_en,
    input  t_mpf_csr_offset csr_mem_wr_idx,
    input  logic [63:0] csr_mem_wr_val,
    output logic csr_mem_wr_rdy
    );


    // ====================================================================
    //
    //   CSR backing storage.  The CSR memory is 32 bits wide and double
    //   pumped in order to use a single RAM.
    //
    // ====================================================================
    
    logic mem_rdy;

    logic wen;
    t_mpf_csr_offset waddr;
    // Upper or lower 32 bit half of full 64 bit entry
    logic waddr_upper_part;
    logic [31:0] wdata;

    t_mpf_csr_offset raddr;
    // Upper or lower 32 bit half of full 64 bit entry
    logic raddr_upper_part;
    logic [31:0] rdata;

    cci_mpf_prim_ram_simple_init
      #(
        .N_ENTRIES(CCI_MPF_CSR_SIZE64 * 2),
        .N_DATA_BITS(32),
        .N_OUTPUT_REG_STAGES(1)
        )
      csr_mem
       (
        .clk,
        .reset,
        .rdy(mem_rdy),

        .wen,
        .waddr({ waddr, waddr_upper_part }),
        .wdata,

        .raddr({ raddr, raddr_upper_part }),
        .rdata
        );


    // ====================================================================
    //
    //   Writes: a write takes two cycles to push 64 bits through the 32
    //   bit wide interface.  csr_mem_wr_rdy is false during the second
    //   half of the write.
    //
    // ====================================================================

    // Initialization state.
    logic initialized;
    localparam NUM_INIT_ENTRIES = 18;   // Count of 64 bit entries
    logic [(NUM_INIT_ENTRIES*2)-1 : 0][31:0] init_val;
    t_mpf_csr_offset [NUM_INIT_ENTRIES-1 : 0] init_idx;

    assign csr_mem_wr_rdy = initialized && ! waddr_upper_part;

    logic csr_mem_wr_en_q;
    t_mpf_csr_offset csr_mem_wr_idx_q;
    logic [63:0] csr_mem_wr_val_q;

    // Delay writes by a cycle to reduce MUX depth
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            csr_mem_wr_en_q <= 1'b0;
            waddr_upper_part <= 1'b0;
            csr_mem_wr_idx_q <= t_mpf_csr_offset'(0);
        end
        else
        begin
            csr_mem_wr_en_q <= csr_mem_wr_en;
            waddr_upper_part <= waddr_upper_part ^ wen;

            if (csr_mem_wr_en)
            begin
                csr_mem_wr_idx_q <= csr_mem_wr_idx;
            end
        end

        // Manage write data
        if (csr_mem_wr_en)
        begin
            // New incoming write
            csr_mem_wr_val_q <= csr_mem_wr_val;
        end
        else if (csr_mem_wr_en_q && ! waddr_upper_part)
        begin
            // Lower part written.  Shift upper part into position.
            csr_mem_wr_val_q[31:0] <= csr_mem_wr_val_q[63:32];
        end
    end

    always_comb
    begin
        // Trigger a write either when requested by client, when
        // completing a 64 bit write or during initialization.
        wen = mem_rdy && (csr_mem_wr_en_q || waddr_upper_part || ! initialized);

        if (initialized)
        begin
            waddr = csr_mem_wr_idx_q;
            wdata = csr_mem_wr_val_q[31:0];
        end
        else
        begin
            waddr = init_idx;
            wdata = init_val[0];
        end
    end


    // ====================================================================
    //
    //   Reads are two phase to retrieve 64 bits from the 32 bit memory
    //   bus.
    //
    // ====================================================================

    // Track read progress
    logic rd_busy;
    logic rd_req_q;
    logic rd_req_qq;
    logic rd_req_qqq;

    assign csr_mem_rd_rdy = initialized && ! rd_busy;

    t_mpf_csr_offset csr_mem_rd_idx_q;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            raddr_upper_part <= 1'b0;
            csr_mem_rd_idx_q <= t_mpf_csr_offset'(0);
        end
        else
        begin
            raddr_upper_part <= raddr_upper_part ^
                                (csr_mem_rd_en || raddr_upper_part);

            csr_mem_rd_idx_q <= csr_mem_rd_idx;
        end
    end

    assign raddr = (raddr_upper_part ? csr_mem_rd_idx_q : csr_mem_rd_idx);

    // Response after 3 cycles
    assign csr_mem_rd_val_valid = rd_req_qqq;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            rd_busy <= 1'b0;
            rd_req_q <= 1'b0;
            rd_req_qq <= 1'b0;
            rd_req_qqq <= 1'b0;
        end
        else
        begin
            // Toggle read busy as read requests flow in and then out
            rd_busy <= rd_busy ^ (csr_mem_rd_en | rd_req_qqq);

            rd_req_q <= csr_mem_rd_en;
            rd_req_qq <= rd_req_q;
            rd_req_qqq <= rd_req_qq;
        end
    end

    // Hold multi-cycle read response
    logic [31:0] rd_val_low;

    always_ff @(posedge clk)
    begin
        if (rd_req_qq)
        begin
            rd_val_low <= rdata;
        end
    end

    assign csr_mem_rd_val = { rdata, rd_val_low };


    // ====================================================================
    //
    //   Initial state of readable CSR memory holds device feature headers
    //   and other constant state.  Statistics will be incorporated at run
    //   time.
    //
    // ====================================================================

    logic [(NUM_INIT_ENTRIES*2)-1 : 0][31:0] init_val_start;
    t_mpf_csr_offset [NUM_INIT_ENTRIES-1 : 0] init_idx_start;
    logic [$clog2(NUM_INIT_ENTRIES) : 0] init_cnt;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            // Populate the initialization vectors on reset
            initialized <= 1'b0;
            init_cnt <= (NUM_INIT_ENTRIES - 1);
            init_val <= init_val_start;
            init_idx <= init_idx_start;
        end
        else if (mem_rdy && ! initialized)
        begin
            //
            // Once the memory is ready one 32 bit initialization will be
            // written every cycle.
            //

            // Finishing the upper part of a 64 bit write?
            if (waddr_upper_part)
            begin
                initialized <= (init_cnt == 0);
                init_cnt <= init_cnt - 1;

                // Shift vector holding indices to initialize
                for (int i = 0; i < NUM_INIT_ENTRIES-1; i = i + 1)
                begin
                    init_idx[i] <= init_idx[i+1];
                end
            end

            // Shift init data by 32 bit chunks every write cycle
            for (int i = 0; i < (NUM_INIT_ENTRIES*2)-1; i = i + 1)
            begin
                init_val[i] <= init_val[i+1];
            end
        end
    end

    function automatic t_ccip_dfh genDFH(t_ccip_feature_next nextFeature);
        t_ccip_dfh dfh;

        dfh = ccip_dfh_defaultDFH();
        dfh.f_type = eFTYP_BBB;
        dfh.id = t_ccip_feature_id'(MPF_INSTANCE_ID);
        dfh.nextFeature = nextFeature;

        return dfh;
    endfunction

    always_comb
    begin
        t_ccip_dfh vtp_dfh;
        logic [127:0] vtp_uid;
        t_ccip_dfh rsp_order_dfh;
        logic [127:0] rsp_order_uid;
        t_ccip_dfh vc_map_dfh;
        logic [127:0] vc_map_uid;
        t_ccip_dfh latency_qos_dfh;
        logic [127:0] latency_qos_uid;
        t_ccip_dfh wro_dfh;
        logic [127:0] wro_uid;
        t_ccip_dfh pwrite_dfh;
        logic [127:0] pwrite_uid;

        // Construct the feature headers for each feature
        vtp_dfh = genDFH(CCI_MPF_VTP_CSR_SIZE);
        vtp_uid = (MPF_ENABLE_VTP != 0) ?
                      // UID of VTP feature (from cci_mpf_csrs.h)
                      128'hc8a2982f_ff96_42bf_a705_45727f501901 :
                      128'h0;

        rsp_order_dfh = genDFH(CCI_MPF_RSP_ORDER_CSR_SIZE);
        rsp_order_uid = (MPF_ENABLE_RSP_ORDER != 0) ?
                      // UID of RSP_ORDER feature (from cci_mpf_csrs.h)
                      128'h4c9c96f4_65ba_4dd8_b383_c70ace57bfe4 :
                      128'h0;

        vc_map_dfh = genDFH(CCI_MPF_VC_MAP_CSR_SIZE);
        vc_map_uid = (MPF_ENABLE_VC_MAP != 0) ?
                      // UID of VC_MAP feature (from cci_mpf_csrs.h)
                      128'h5046c86f_ba48_4856_b8f9_3b76e3dd4e74 :
                      128'h0;

        latency_qos_dfh = genDFH(CCI_MPF_LATENCY_QOS_CSR_SIZE);
        latency_qos_uid = (MPF_ENABLE_LATENCY_QOS != 0) ?
                      // UID of LATENCY_QOS feature (from cci_mpf_csrs.h)
                      128'hb35138f6_ea39_4603_9412_a4cf1a999c49 :
                      128'h0;

        wro_dfh = genDFH(CCI_MPF_WRO_CSR_SIZE);
        wro_uid = (MPF_ENABLE_WRO != 0) ?
                      // UID of WRO feature (from cci_mpf_csrs.h)
                      128'h56b06b48_9dd7_4004_a47e_0681b4207a6d :
                      128'h0;

        pwrite_dfh = genDFH(CCI_MPF_PWRITE_CSR_SIZE);
        pwrite_uid = (MPF_ENABLE_PWRITE != 0) ?
                      // UID of PWRITE feature (from cci_mpf_csrs.h)
                      128'h9bdbbcaf_2c5a_4d17_a636_75b19a0b4f5c :
                      128'h0;

        if (DFH_MMIO_NEXT_ADDR == 0)
        begin
            // PWRITE is the last feature in the AFU's list
            pwrite_dfh.eol = 1'b1;
        end
        else
        begin
            // Point to the next feature (outside of MPF)
            pwrite_dfh.nextFeature = DFH_MMIO_NEXT_ADDR - CCI_MPF_PWRITE_CSR_BASE;
        end

        // Each DFH entry is 64 bits.  Each UID entry is 128 bits.
        init_val_start = { vtp_dfh, vtp_uid,
                           rsp_order_dfh, rsp_order_uid,
                           vc_map_dfh, vc_map_uid,
                           latency_qos_dfh, latency_qos_uid,
                           wro_dfh, wro_uid,
                           pwrite_dfh, pwrite_uid };

        init_idx_start = {
            // VTP DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_VTP_CSR_OFFSET + CCI_MPF_VTP_CSR_DFH) >> 3),
            // VTP UID high
            t_mpf_csr_offset'((CCI_MPF_VTP_CSR_OFFSET + CCI_MPF_VTP_CSR_ID_H) >> 3),
            // VTP UID low
            t_mpf_csr_offset'((CCI_MPF_VTP_CSR_OFFSET + CCI_MPF_VTP_CSR_ID_L) >> 3),

            // RSP_ORDER DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_RSP_ORDER_CSR_OFFSET + CCI_MPF_RSP_ORDER_CSR_DFH) >> 3),
            // RSP_ORDER UID high
            t_mpf_csr_offset'((CCI_MPF_RSP_ORDER_CSR_OFFSET + CCI_MPF_RSP_ORDER_CSR_ID_H) >> 3),
            // RSP_ORDER UID low
            t_mpf_csr_offset'((CCI_MPF_RSP_ORDER_CSR_OFFSET + CCI_MPF_RSP_ORDER_CSR_ID_L) >> 3),

            // VC_MAP DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_VC_MAP_CSR_OFFSET + CCI_MPF_VC_MAP_CSR_DFH) >> 3),
            // VC_MAP UID high
            t_mpf_csr_offset'((CCI_MPF_VC_MAP_CSR_OFFSET + CCI_MPF_VC_MAP_CSR_ID_H) >> 3),
            // VC_MAP UID low
            t_mpf_csr_offset'((CCI_MPF_VC_MAP_CSR_OFFSET + CCI_MPF_VC_MAP_CSR_ID_L) >> 3),

            // LATENCY_QOS DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_LATENCY_QOS_CSR_OFFSET + CCI_MPF_LATENCY_QOS_CSR_DFH) >> 3),
            // LATENCY_QOS UID high
            t_mpf_csr_offset'((CCI_MPF_LATENCY_QOS_CSR_OFFSET + CCI_MPF_LATENCY_QOS_CSR_ID_H) >> 3),
            // LATENCY_QOS UID low
            t_mpf_csr_offset'((CCI_MPF_LATENCY_QOS_CSR_OFFSET + CCI_MPF_LATENCY_QOS_CSR_ID_L) >> 3),

            // WRO DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_WRO_CSR_OFFSET + CCI_MPF_WRO_CSR_DFH) >> 3),
            // WRO UID high
            t_mpf_csr_offset'((CCI_MPF_WRO_CSR_OFFSET + CCI_MPF_WRO_CSR_ID_H) >> 3),
            // WRO UID low
            t_mpf_csr_offset'((CCI_MPF_WRO_CSR_OFFSET + CCI_MPF_WRO_CSR_ID_L) >> 3),

            // PWRITE DFH (device feature header)
            t_mpf_csr_offset'((CCI_MPF_PWRITE_CSR_OFFSET + CCI_MPF_PWRITE_CSR_DFH) >> 3),
            // PWRITE UID high
            t_mpf_csr_offset'((CCI_MPF_PWRITE_CSR_OFFSET + CCI_MPF_PWRITE_CSR_ID_H) >> 3),
            // PWRITE UID low
            t_mpf_csr_offset'((CCI_MPF_PWRITE_CSR_OFFSET + CCI_MPF_PWRITE_CSR_ID_L) >> 3)
            };
    end

endmodule // cci_mpf_shim_csr_rd_memory
