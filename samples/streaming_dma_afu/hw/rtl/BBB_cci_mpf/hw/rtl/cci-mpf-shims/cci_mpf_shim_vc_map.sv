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

`include "cci_mpf_prim_hash.vh"


//
// Map read/write requests to eVC_VA to specific physical channels.  Mapping
// is a function of addresses such that a read or write to a given location
// is always mapped to the same channel.  This makes it possible to be sure
// that a value written is committed to memory before a later read or write.
// This method of tracking write responses works only within a single channel.
// Cross-channel commits require WrFence to eVC_VA, which is expensive.
//
// Throughput using eVC_VA is typically higher than throughput using explicit
// channel mapping.  This module seeks to optimize the ratio of requests
// for maximum throughput on a given hardware platform.
//


module cci_mpf_shim_vc_map
  #(
    // Allow dynamic mapping in response to traffic patterns?
    parameter ENABLE_DYNAMIC_VC_MAPPING = 1,

    // Maximum number in-flight requests per channel.
    parameter MAX_ACTIVE_REQS = 128,

    // Mdata index that can be used to tag a private WrFence.
    parameter RESERVED_MDATA_IDX = -1
    )
   (
    input  logic clk,

    // Connection toward the QA platform.  Reset comes in here.
    cci_mpf_if.to_fiu fiu,

    // Connections toward user code.
    cci_mpf_if.to_afu afu,

    cci_mpf_csrs.vc_map csrs,
    cci_mpf_csrs.vc_map_events events
    );

    // How often to sample for a new ratio (cycles)
    localparam MAX_SAMPLE_CYCLES_RADIX = 16;

    // Sample in groups of N_SAMPLE_REGIONS.  When N_SAMPLE_REGIONS successive
    // regions all have the same recommended configuration then update the
    // channel mapping ratio.
    localparam N_SAMPLE_REGIONS = 16;


    assign afu.reset = fiu.reset;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end


    assign fiu.c2Tx = afu.c2Tx;

    //
    // Response channels
    //
    logic is_vc_map_wrfence_rsp;

    always_comb
    begin
        afu.c0Rx = fiu.c0Rx;

        // Drop locally generated WrFence
        afu.c1Rx = fiu.c1Rx;
        if (is_vc_map_wrfence_rsp)
        begin
            afu.c1Rx = ccip_c1Rx_clearValids();
        end
    end


    //
    // Request flow control
    //
    logic block_tx_traffic;
    always_comb
    begin
        afu.c0TxAlmFull = fiu.c0TxAlmFull || block_tx_traffic;
        afu.c1TxAlmFull = fiu.c1TxAlmFull || block_tx_traffic;
    end


    // ====================================================================
    //
    //   Pick the right ratio for the current traffic and hardware.
    //
    // ====================================================================

    //
    // pickRatioVL0 assumes that there are three physical channels:
    // VL0 and two PCIe channels.  The value returned is the fraction of
    // requests that should be directed to VL0 in order to hit maximum
    // bandwidth.  The returned value is the numerator of the fraction,
    // measured as 64ths of all references.
    //

    // Fraction of requests to map to VL0 (of 64)
    typedef logic [5:0] t_map_ratio;

    function automatic t_map_ratio pickRatioVL0(logic mostlyRead,
                                                logic mostlyWrite,
                                                t_ccip_clLen reqLen);
        int r;

        //
        // These values are computed using the output of
        // test/test-mpf/test_mem_perf/sw/compute_vc_map_params in this MPF
        // library.  The program emits the performance of all possible
        // fractions and the best VL0 ratios are chosen manually.  When
        // multiple ratios result in the same performance it is usually
        // best to pick a value that is common to as many multi-beat sizes
        // as possible, since some AFUs emit more than one size request
        // simultaneously.
        //

        if (MPF_PLATFORM == "SKX")
        begin
            if (mostlyRead)
            begin
                case (reqLen)
                    eCL_LEN_1: r = 36;
                    eCL_LEN_2: r = 32;
                    default: r = 30;
                endcase
            end
            else if (mostlyWrite)
            begin
                case (reqLen)
                    eCL_LEN_1: r = 18;
                    eCL_LEN_2: r = 18;
                    default: r = 16;
                endcase
            end
            else
            begin
                case (reqLen)
                    eCL_LEN_1: r = 24;
                    eCL_LEN_2: r = 18;
                    default: r = 16;
                endcase
            end
        end
        else if (MPF_PLATFORM == "BDX")
        begin
            if (mostlyRead)
            begin
                case (reqLen)
                    eCL_LEN_1: r = 28;
                    eCL_LEN_2: r = 26;
                    default: r = 24;
                endcase
            end
            else if (mostlyWrite)
            begin
                case (reqLen)
                    eCL_LEN_1: r = 16;
                    eCL_LEN_2: r = 16;
                    default: r = 14;
                endcase
            end
            else
            begin
                case (reqLen)
                    eCL_LEN_1: r = 20;
                    eCL_LEN_2: r = 16;
                    default: r = 12;
                endcase
            end
        end
        else
        begin
            // Unknown platform.
            $fatal(2, "Unknown platform");
        end

        return t_map_ratio'(r);
    endfunction

    // Set the default channel mapping ratio.
    localparam RATIO_VL0_DEFAULT = pickRatioVL0(1'b0, 1'b0, eCL_LEN_2);


    // ====================================================================
    //
    //   Mapping control
    //
    // ====================================================================

    logic mapping_disabled;
    logic c0_mapping_disabled, c1_mapping_disabled;
    logic dynamic_mapping_disabled;
    logic map_all;

    t_map_ratio ratio_vl0;
    logic always_use_vl0;

    logic [MAX_SAMPLE_CYCLES_RADIX+2 : 0] always_use_vl0_threshold_mask;

    // Should request be mapped?
    function automatic logic req_needs_mapping(t_cci_vc vc_sel);
        return ((vc_sel == eVC_VA) || map_all);
    endfunction

    // Set when dynamic logic below suggests a better ratio.
    logic new_ratio_vl0_en;
    t_map_ratio new_ratio_vl0;
    logic new_always_use_vl0;

    logic [$clog2(MAX_SAMPLE_CYCLES_RADIX)-1 : 0] sample_interval_idx;
    // Default to sampling every 1K cycles
    localparam DEFAULT_SAMPLE_INTERVAL_IDX = 10;

    // History of mappings, exported as a CSR
    logic [63:0] vc_map_history;
    assign csrs.vc_map_history = { vc_map_history[63:8], 1'b0,
                                   // Bit 6, making 64/64ths if set
                                   always_use_vl0,
                                   always_use_vl0 ? 5'b0 : ratio_vl0 };

    always_ff @(posedge clk)
    begin
        if (csrs.vc_map_ctrl_valid)
        begin
            // See cci_mpf_csrs.h for bit assignments in the control word.

            // Group A
            if (csrs.vc_map_ctrl[63])
            begin
                mapping_disabled <= ~ (|(csrs.vc_map_ctrl[1:0]));
                c0_mapping_disabled <= ~ csrs.vc_map_ctrl[0];
                c1_mapping_disabled <= ~ csrs.vc_map_ctrl[1];

                dynamic_mapping_disabled <= ~ csrs.vc_map_ctrl[2];
                sample_interval_idx <=
                    (csrs.vc_map_ctrl[6:3] != 4'b0) ? csrs.vc_map_ctrl[6:3] :
                                                      DEFAULT_SAMPLE_INTERVAL_IDX;
                ratio_vl0 <= RATIO_VL0_DEFAULT;
            end

            // Group B
            if (csrs.vc_map_ctrl[62])
            begin
                map_all <= csrs.vc_map_ctrl[7];
            end

            // Group C
            if (csrs.vc_map_ctrl[61])
            begin
                dynamic_mapping_disabled <= 1'b1;
                ratio_vl0 <= csrs.vc_map_ctrl[8] ? csrs.vc_map_ctrl[14:9] :
                                                   RATIO_VL0_DEFAULT;
                always_use_vl0 <= csrs.vc_map_ctrl[15];
            end

            // Group D
            if (csrs.vc_map_ctrl[60])
            begin
                always_use_vl0_threshold_mask <=
                    ~(($bits(always_use_vl0_threshold_mask))'(csrs.vc_map_ctrl[31:16]));
            end
        end
        else if (new_ratio_vl0_en)
        begin
            // Dynamic logic has suggested a better ratio
            ratio_vl0 <= new_ratio_vl0;
            always_use_vl0 <= new_always_use_vl0;

            // Shift history
            vc_map_history[63:8] <= csrs.vc_map_history[55:0];
        end

        if (reset)
        begin
            mapping_disabled <= 1'b0;
            c0_mapping_disabled <= 1'b0;
            c1_mapping_disabled <= 1'b0;
            dynamic_mapping_disabled <= (ENABLE_DYNAMIC_VC_MAPPING == 0);
            sample_interval_idx <= DEFAULT_SAMPLE_INTERVAL_IDX;

            map_all <= 1'b0;
            ratio_vl0 <= RATIO_VL0_DEFAULT;
            always_use_vl0 <= 1'b0;
            always_use_vl0_threshold_mask <= ~(($bits(always_use_vl0_threshold_mask))'(0));

            vc_map_history <= 64'b0;
        end
    end


    //
    // Request mapping function.  The mapping is consistent for a given address.
    //
    function automatic t_ccip_vc mapVA(t_ccip_clAddr addr);
        t_ccip_vc vc;

        // The hash function operates on 32 bits of the address.  Drop the
        // low address bits that are covered by multi-line requests so that
        // a given address always winds up hashed to the same channel,
        // independent of the access size.
        logic [31:0] a = addr[$bits(t_ccip_clNum) +: 32];

        // Input bits 4 and 5 are underrepresented in the low 6 bits of
        // the CRC-32 hash.  Swap them with less important higher bits
        // of the address.
        logic [31:0] addr_swizzle = { a[29:4], a[31:30], a[3:0] };

        // Hash addresses for even distribution within the mapping table,
        // attempting to have the mapping be independent of the memory
        // access pattern.
        t_map_ratio hashed_idx = t_map_ratio'(hash32(addr_swizzle));

        // Now that entries are hashed we can just use ranges in the mapping.
        if ((hashed_idx < ratio_vl0) || always_use_vl0)
        begin
            vc = eVC_VL0;
        end
        else
        begin
            // Choose randomly with equal weight between PCIe channels
            vc = (hashed_idx[0] ? eVC_VH0 : eVC_VH1);
        end

        return vc;
    endfunction


    //
    // Compute the possible VA to physical mapping for both the read and
    // write request channels and register the incoming requests.  The
    // mapped channel may be used in the subsequent cycle.
    //
    t_if_cci_mpf_c0_Tx c0_tx;
    t_ccip_vc c0_vc_map;
    t_if_cci_mpf_c1_Tx c1_tx;
    t_ccip_vc c1_vc_map;

    always_ff @(posedge clk)
    begin
        c0_tx <= afu.c0Tx;
        c0_vc_map <= mapVA(cci_mpf_c0_getReqAddr(afu.c0Tx.hdr));

        c1_tx <= afu.c1Tx;
        c1_vc_map <= mapVA(cci_mpf_c1_getReqAddr(afu.c1Tx.hdr));
    end

    always_comb
    begin
        fiu.c0Tx = c0_tx;

        if (! c0_mapping_disabled &&
            cci_mpf_c0_getReqMapVA(c0_tx.hdr) &&
            req_needs_mapping(c0_tx.hdr.base.vc_sel))
        begin
            fiu.c0Tx.hdr.base.vc_sel = c0_vc_map;
        end
    end

    logic emit_wrfence_en;

    always_comb
    begin
        fiu.c1Tx = c1_tx;

        if (! c1_mapping_disabled &&
            cci_mpf_c1_getReqMapVA(c1_tx.hdr) &&
            req_needs_mapping(c1_tx.hdr.base.vc_sel))
        begin
            fiu.c1Tx.hdr.base.vc_sel = c1_vc_map;
        end

        //
        // Changing to a new ratio?  This will only be set when there is
        // no incoming request from the AFU.  Emit a WrFence on eVA_VC to
        // force full synchronization before changing the address mapping.
        //
        if (emit_wrfence_en)
        begin
            t_cci_mpf_ReqMemHdrParams wrfence_params;
            wrfence_params = cci_mpf_defaultReqHdrParams(0);
            wrfence_params.vc_sel = eVC_VA;
            wrfence_params.sop = 1'b0;

            fiu.c1Tx.hdr = cci_mpf_c1_genReqHdr(eREQ_WRFENCE,
                                                t_cci_clAddr'('x),
                                                t_cci_mdata'(1 << RESERVED_MDATA_IDX),
                                                wrfence_params);
            fiu.c1Tx.valid = 1'b1;
        end
    end


    // ====================================================================
    //
    //   Act on sampled data.
    //
    // ====================================================================

    typedef enum logic [1:0] {
        CCI_MPF_VC_MAP_SAMPLING,
        CCI_MPF_VC_MAP_EMIT_WRFENCE,
        CCI_MPF_VC_MAP_WAIT_WRFENCE_RSP,
        CCI_MPF_VC_MAP_WAIT_QUIET
    }
    t_cci_mpf_vc_map_state;
    t_cci_mpf_vc_map_state state;

    logic req_ratio_vl0_en;
    t_map_ratio req_ratio_vl0;
    logic req_always_use_vl0;

    logic c0_req_active;
    logic c1_req_active;

    //
    // Primary state machine.  When a new ratio is requested a WrFence must
    // be emitted and traffic stopped until all channels are synchronized.
    //
    always_ff @(posedge clk)
    begin
        case (state)
          CCI_MPF_VC_MAP_SAMPLING:
            begin
                // New request to change the ratio?
                if (req_ratio_vl0_en &&
                    ! mapping_disabled &&
                    ! dynamic_mapping_disabled &&
                    ((req_ratio_vl0 != ratio_vl0) ||
                     (req_always_use_vl0 != always_use_vl0)))
                begin
                    new_ratio_vl0 <= req_ratio_vl0;
                    new_always_use_vl0 <= req_always_use_vl0;

                    block_tx_traffic <= 1'b1;
                    state <= CCI_MPF_VC_MAP_EMIT_WRFENCE;
                end
            end

          CCI_MPF_VC_MAP_EMIT_WRFENCE:
            begin
                if (emit_wrfence_en)
                begin
                    state <= CCI_MPF_VC_MAP_WAIT_WRFENCE_RSP;
                end
            end

          CCI_MPF_VC_MAP_WAIT_WRFENCE_RSP:
            begin
                if (is_vc_map_wrfence_rsp)
                begin
                    new_ratio_vl0_en <= 1'b1;
                    events.vc_map_out_event_mapping_changed <= 1'b1;
                    state <= CCI_MPF_VC_MAP_WAIT_QUIET;
                end
            end

          CCI_MPF_VC_MAP_WAIT_QUIET:
            begin
                new_ratio_vl0_en <= 1'b0;
                events.vc_map_out_event_mapping_changed <= 1'b0;

                // Wait for all previous requests to complete since they are
                // now tracked on the wrong channel.  Writes were already
                // checked before the fence was emitted, so just wait for reads.
                if (! c0_req_active)
                begin
                    block_tx_traffic <= 1'b0;

                    state <= CCI_MPF_VC_MAP_SAMPLING;
                end
            end
        endcase

        if (reset)
        begin
            state <= CCI_MPF_VC_MAP_SAMPLING;
            block_tx_traffic <= 1'b0;
            events.vc_map_out_event_mapping_changed <= 1'b0;
            new_ratio_vl0_en <= 1'b0;
        end
    end

    // WrFence is emitted if requested, no C1 TX traffic is being
    // requested by the AFU and all writes have retired.
    logic wrfence_rdy;
    assign emit_wrfence_en = (state == CCI_MPF_VC_MAP_EMIT_WRFENCE) &&
                             wrfence_rdy &&
                             ! cci_mpf_c1TxIsValid(c1_tx);

    logic [2:0] wrfence_delay;

    always_ff @(posedge clk)
    begin
        wrfence_rdy <= ! c1_req_active &&
                       ! fiu.c0TxAlmFull &&
                       (&(wrfence_delay));

        // Impose a short delay on emitting wrfence to ensure enough time
        // with almost full high so AFU traffic stops.
        if (! (&(wrfence_delay)))
        begin
            // Saturating counter
            wrfence_delay <= wrfence_delay + 3'b1;
        end

        if (state != CCI_MPF_VC_MAP_EMIT_WRFENCE)
        begin
            wrfence_delay <= 3'b0;
        end

        if (reset)
        begin
            wrfence_rdy <= 1'b0;
            wrfence_delay <= 3'b0;
        end
    end

    // WrFence response received?
    assign is_vc_map_wrfence_rsp =
        cci_c1Rx_isWriteFenceRsp(fiu.c1Rx) &&
        fiu.c1Rx.hdr.mdata[RESERVED_MDATA_IDX];

    typedef logic [MAX_SAMPLE_CYCLES_RADIX-1 : 0] t_sample_cnt;
    t_sample_cnt n_sampled_reads;
    t_sample_cnt n_sampled_writes;

    // reads + writes, hence larger counters
    logic [MAX_SAMPLE_CYCLES_RADIX : 0] n_sampled_len1;
    logic [MAX_SAMPLE_CYCLES_RADIX : 0] n_sampled_len2;
    logic [MAX_SAMPLE_CYCLES_RADIX : 0] n_sampled_len4;
    // Counting lines of reads and writes, up to 8 lines per cycle
    logic [MAX_SAMPLE_CYCLES_RADIX+2 : 0] n_sampled_all;

    // Was a request on VA present?  No point in change unless there are some.
    logic saw_va_req;

    logic sample_region_done;
    logic sample_region_done_q;
    logic sample_ready;

    t_map_ratio sample_suggestions[0 : N_SAMPLE_REGIONS-1];

    // Do all regions match?
    logic all_regions_match;
    always_comb
    begin
        all_regions_match = 1'b1;

        for (int i = 0; i < N_SAMPLE_REGIONS-1; i = i + 1)
        begin
            all_regions_match =
                all_regions_match &&
                (sample_suggestions[i + 1] == sample_suggestions[i]);
        end
    end

    // Is the traffic so low that only the low latency channel should be used?
    logic req_always_use_vl0_next;
    assign req_always_use_vl0_next =
               ((n_sampled_all & always_use_vl0_threshold_mask) == 0);

    //
    // Update ratio being used at sampling boundary.  The "damper" variable
    // limits the rate of change.
    //
    logic [5 : 0] damper;

    always_ff @(posedge clk)
    begin
        // The enable register gates used of all other state state here
        req_ratio_vl0_en <= 1'b0;

        req_ratio_vl0 <= ratio_vl0;
        req_always_use_vl0 <= req_always_use_vl0_next;

        // Sampling window just ended?
        if (sample_ready)
        begin
            if (all_regions_match)
            begin
                // In a consistent recommended state.  Change to it if not
                // there already.
                req_ratio_vl0_en <= 1'b1;
                req_ratio_vl0 <= sample_suggestions[0];
            end
            else
            begin
                // Not in a consistent state.  After waiting a while change
                // to the default state.
                req_ratio_vl0_en <= damper[$bits(damper)-1];
                req_ratio_vl0 <= RATIO_VL0_DEFAULT;
            end

            // Low traffic?  Switch in and out of this state quickly, using
            // only one sampling window.  With low traffic the impact of
            // a write fence should be low.
            if (always_use_vl0 != req_always_use_vl0_next)
            begin
                req_ratio_vl0_en <= 1'b1;
            end
        end
    end


    //
    // Reduce spurious change by damping reversion to the default ratio.
    //
    always_ff @(posedge clk)
    begin
        if (sample_ready)
        begin
            if (all_regions_match)
            begin
                // Matched a group of regions
                damper <= 0;
            end
            else if (! damper[$bits(damper)-1])
            begin
                // Variable recommendations found in successive regions.
                // Increment damper with a saturating counter.
                damper <= damper + 1;
            end
        end

        if (reset)
        begin
            damper <= 0;
        end
    end


    //
    // What is the typical number of lines per request in the current
    // sample interval?
    //
    function automatic t_ccip_clLen sampled_req_len();
        // Only claim length 1 if all traffic is 1
        if ((n_sampled_len2 == 0) && (n_sampled_len4 == 0))
        begin
            return eCL_LEN_1;
        end
        else if ((n_sampled_len4 >> 3) > n_sampled_len2)
        begin
            // len 4 is at least 8x more frequent than len 2
            return eCL_LEN_4;
        end
        else
        begin
            return eCL_LEN_2;
        end
    endfunction

    always_ff @(posedge clk)
    begin
        // Run once every time a region ends (based on clocks)
        if (sample_region_done && (state == CCI_MPF_VC_MAP_SAMPLING))
        begin
            // Shift old suggestions
            for (int i = 0; i < N_SAMPLE_REGIONS-1; i = i + 1)
            begin
                sample_suggestions[i + 1] <= sample_suggestions[i];
            end

            // Compute a new suggestion.
            //
            // Claim mostly reads or mostly writes only if one is at
            // least 8x more frequent than the other.
            sample_suggestions[0] <=
                pickRatioVL0((n_sampled_reads >> 3) > n_sampled_writes,
                             (n_sampled_writes >> 3) > n_sampled_reads,
                             sampled_req_len());

            sample_ready <= saw_va_req;
        end
        else
        begin
            sample_ready <= 1'b0;
        end

        if (reset)
        begin
            sample_ready <= 1'b0;
        end
    end


    // ====================================================================
    //
    //   Count reads and writes over sample interval.
    //
    // ====================================================================

    logic [MAX_SAMPLE_CYCLES_RADIX : 0] cycle_cnt;
    always_ff @(posedge clk)
    begin
        sample_region_done <= (cycle_cnt[sample_interval_idx] == 1'b1) &&
                              ! sample_region_done_q;
        sample_region_done_q <= sample_region_done;

        if (sample_region_done)
        begin
            sample_region_done <= 1'b0;
        end

        if (reset)
        begin
            sample_region_done <= 1'b0;
            sample_region_done_q <= 1'b0;
        end
    end

    logic rd_len1;
    logic rd_len2;
    logic rd_len4;
    logic wr_len1;
    logic wr_len2;
    logic wr_len4;

    always_comb
    begin
        rd_len1 = 0;
        rd_len2 = 0;
        rd_len4 = 0;

        if (cci_mpf_c0TxIsReadReq(c0_tx))
        begin
            case (c0_tx.hdr.base.cl_len)
                eCL_LEN_1: rd_len1 = 1;
                eCL_LEN_2: rd_len2 = 1;
                  default: rd_len4 = 1;
            endcase
        end

        wr_len1 = 0;
        wr_len2 = 0;
        wr_len4 = 0;

        if (cci_mpf_c1TxIsWriteReq(c1_tx))
        begin
            case (c1_tx.hdr.base.cl_len)
                eCL_LEN_1: wr_len1 = 1;
                eCL_LEN_2: wr_len2 = 1;
                  default: wr_len4 = 1;
            endcase
        end
    end

    // How many lines were requested this cycle?
    logic [3:0] lines_this_cycle;
    always_comb
    begin
        lines_this_cycle = 4'(0);

        if (cci_mpf_c0TxIsReadReq(c0_tx))
        begin
            lines_this_cycle = lines_this_cycle +
                               4'(c0_tx.hdr.base.cl_len) +
                               4'd1;
        end

        if (cci_mpf_c1TxIsWriteReq(c1_tx))
        begin
            lines_this_cycle = lines_this_cycle +
                               4'(c1_tx.hdr.base.cl_len) +
                               4'd1;
        end
    end

    always_ff @(posedge clk)
    begin
        cycle_cnt <= cycle_cnt + 1;

        if (sample_region_done_q)
        begin
            // End of sampling interval.  Reset all counters.
            cycle_cnt <= 1'b1;

            n_sampled_reads <= 0;
            n_sampled_writes <= 0;
            n_sampled_all <= 0;
            n_sampled_len1 <= 0;
            n_sampled_len2 <= 0;
            n_sampled_len4 <= 0;
            saw_va_req <= 0;
        end
        else
        begin
            if (cci_mpf_c0TxIsReadReq(c0_tx))
            begin
                n_sampled_reads <= n_sampled_reads + 1;

                if (req_needs_mapping(c0_tx.hdr.base.vc_sel))
                begin
                    saw_va_req <= 1;
                end
            end

            if (cci_mpf_c1TxIsWriteReq(c1_tx))
            begin
                n_sampled_writes <= n_sampled_writes + 1;

                if (req_needs_mapping(c1_tx.hdr.base.vc_sel))
                begin
                    saw_va_req <= 1;
                end
            end

            n_sampled_all <= n_sampled_all + ($bits(n_sampled_all))'(lines_this_cycle);

            n_sampled_len1 <= n_sampled_len1 + rd_len1 + wr_len1;
            n_sampled_len2 <= n_sampled_len2 + rd_len2 + wr_len2;
            n_sampled_len4 <= n_sampled_len4 + rd_len4 + wr_len4;
        end

        if (reset)
        begin
            cycle_cnt <= 0;

            n_sampled_reads <= 0;
            n_sampled_writes <= 0;
            n_sampled_all <= 0;
            n_sampled_len1 <= 0;
            n_sampled_len2 <= 0;
            n_sampled_len4 <= 0;
            saw_va_req <= 0;
        end
    end


    // ====================================================================
    // 
    //   Track outstanding requests
    // 
    // ====================================================================

    cci_mpf_shim_vc_map_track_active
      #(
        .MAX_ACTIVE_REQS(MAX_ACTIVE_REQS)
        )
      c0_tracker
       (
        .clk,
        .reset,
        .noteReq(cci_mpf_c0TxIsReadReq(c0_tx)),
        .noteRsp(cci_mpf_c0Rx_isEOP(fiu.c0Rx) && cci_c0Rx_isReadRsp(fiu.c0Rx)),
        .isActive(c0_req_active)
        );

    cci_mpf_shim_vc_map_track_active
      #(
        .MAX_ACTIVE_REQS(MAX_ACTIVE_REQS)
        )
      c1_tracker
       (
        .clk,
        .reset,
        .noteReq(cci_mpf_c1TxIsWriteReq(c1_tx)),
        .noteRsp(cci_c1Rx_isWriteRsp(fiu.c1Rx)),
        .isActive(c1_req_active)
        );

endmodule // cci_mpf_shim_vc_map


//
// Track requests and responses on a channel and output a flag indicating
// whether any requests are active.
//
module cci_mpf_shim_vc_map_track_active
  #(
    parameter MAX_ACTIVE_REQS = 128
    )
   (
    input  logic clk,
    input  logic reset,

    // Client enables each cycle a request or response is processed
    input  logic noteReq,
    input  logic noteRsp,

    // False iff no requests are active
    output logic isActive
    );

    logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] num_outstanding_reqs;

    logic noteReq_q;
    logic noteRsp_q;

    always_ff @(posedge clk)
    begin
        noteReq_q <= noteReq;
        noteRsp_q <= noteRsp;

        if (reset)
        begin
            noteReq_q <= 1'b0;
            noteRsp_q <= 1'b0;
        end
    end

    always_ff @(posedge clk)
    begin
        isActive <= (|(num_outstanding_reqs)) || noteReq || noteReq_q;

        if (noteReq_q && ! noteRsp_q)
        begin
            num_outstanding_reqs <= num_outstanding_reqs + 1;
        end

        if (! noteReq_q && noteRsp_q)
        begin
            num_outstanding_reqs <= num_outstanding_reqs - 1;
        end

        if (reset)
        begin
            isActive <= 1'b0;
            num_outstanding_reqs <= 0;
        end
    end

endmodule // cci_mpf_shim_vc_map_track_active
