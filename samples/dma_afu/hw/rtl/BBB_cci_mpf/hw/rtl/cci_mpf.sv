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

//
// MPF main wrapper.  This module defines the edge of MPF that attaches
// to the FIU.  It then instantiates an MPF pipeline to implement the
// desired MPF services.
//

`include "cci_mpf_if.vh"
`include "cci_mpf_csrs.vh"
`include "cci_mpf_shim_edge.vh"
`include "cci_mpf_shim_pwrite.vh"


//
// This wrapper is a reference implementation of the composition of shims.
// Developers are free to compose memories with other properties.
//

module cci_mpf
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

    // Enable virtual to physical translation?
    parameter ENABLE_VTP = 1,

    // Enable mapping of eVC_VA to physical channels?  AFUs that both use
    // eVC_VA and read back memory locations written by the AFU must either
    // emit WrFence on VA or use explicit physical channels and enforce
    // write/read order.  Each method has tradeoffs.  WrFence VA is expensive
    // and should be emitted only infrequently.  Memory requests to eVC_VA
    // may have higher bandwidth than explicit mapping.  The MPF module for
    // physical channel mapping is optimized for each CCI platform.
    //
    // The mapVAtoPhysChannel extended header bit must be set on each
    // request to enable mapping.
    parameter ENABLE_VC_MAP = 0,
    // When ENABLE_VC_MAP is set the mapping is either static for the entire
    // run or dynamic, changing in response to traffic patterns.  The mapper
    // guarantees synchronization when the mapping changes by emitting a
    // WrFence on eVC_VA and draining all reads.  Ignored when ENABLE_VC_MAP
    // is 0.
    parameter ENABLE_DYNAMIC_VC_MAPPING = 1,

    // Manage traffic to reduce latency without sacrificing bandwidth?
    // The blue bitstream buffers for a given channel may be larger than
    // necessary to sustain full bandwidth.  Allowing more requests beyond
    // this threshold to enter the channel increases latency without
    // increasing bandwidth.  While this is fine for some applications,
    // those with multiple kernels connected to the CCI memory interface
    // may see performance gains when a kernel's performance is a
    // latency-sensitive.
    parameter ENABLE_LATENCY_QOS = 0,

    // Enforce write/write and write/read ordering with cache lines?
    parameter ENFORCE_WR_ORDER = 0,

    // Return read responses in the order they were requested?
    parameter SORT_READ_RESPONSES = 1,

    // Preserve Mdata field in write requests?  Turn this off if the AFU
    // merely counts write responses instead of checking Mdata.
    parameter PRESERVE_WRITE_MDATA = 0,

    // Enable partial write emulation.  CCI has no support for masked
    // writes that merge new data with existing data in a line.  MPF
    // adds byte-level masks to the write request header and emulates
    // partial writes as a read-modify-write operation.  When coupled
    // with ENFORCE_WR_ORDER, partial writes are free of races on the
    // FPGA side.  There are no guarantees of atomicity and there is
    // no protection against races with CPU-generates writes.
    parameter ENABLE_PARTIAL_WRITES = 0,

    // Experimental:  Merge nearby reads from the same address?  Some
    // applications generate reads to the same line within a few cycles
    // of each other.  This module reduces the requests to single host
    // read and replicates the result.  The module requires a wide
    // block RAM FIFO, so should not be enabled without some thought.
    parameter MERGE_DUPLICATE_READS = 0
    )
   (
    input  logic      clk,

    //
    // Signals connecting to QA Platform
    //
    cci_mpf_if.to_fiu fiu,

    //
    // Signals connecting to AFU, the client code
    //
    cci_mpf_if.to_afu afu,

    //
    // Is a request active somewhere in the memory?  Clients waiting for
    // all responses to complete can monitor these signals.
    //
    output logic c0NotEmpty,
    output logic c1NotEmpty
    );

    // Maximum number of outstanding read and write requests per channel
`ifdef MPF_PLATFORM_SKX
    localparam MAX_ACTIVE_REQS = 1024;
`elsif MPF_PLATFORM_DCP_PCIE
    localparam MAX_ACTIVE_REQS = 512;
`elsif MPF_PLATFORM_BDX
    localparam MAX_ACTIVE_REQS = 1024;
`elsif MPF_PLATFORM_OME
    localparam MAX_ACTIVE_REQS = 128;
`else
    ** ERROR: Unknown platform
`endif

    // Reserved bits in the mdata field, used by various modules.
    localparam RESERVED_MDATA_IDX = CCI_PLATFORM_MDATA_WIDTH - 2;

    // No point in enabling VC Map when there is only one channel
    localparam MPF_ENABLE_VC_MAP =
        (MPF_PLATFORM_NUM_PHYSICAL_CHANNELS > 1) ? ENABLE_VC_MAP : 0;

    logic reset = 1'b1;
    always @(posedge clk)
    begin
        reset <= fiu.reset;
    end

    cci_mpf_csrs mpf_csrs ();


    // ====================================================================
    //
    //  Track channel credits, limiting the number of outstanding requests.
    //  Thresholds are platform-specific.
    //
    //  This module must go right at the edge of the FIU since it is
    //  managing the almost full signal up the stack to maintain optimal
    //  traffic into the FIU.
    //
    // ====================================================================

    cci_mpf_if stgm1_fiu_latency_qos (.clk);

    generate
        if (ENABLE_LATENCY_QOS)
        begin : lat_qos
            cci_mpf_shim_latency_qos
              #(
                .MAX_ACTIVE_LINES(MAX_ACTIVE_REQS)
                )
              latency_qos
               (
                .clk,
                .fiu(fiu),
                .afu(stgm1_fiu_latency_qos),
                .csrs(mpf_csrs)
                );
        end
        else
        begin : no_lat_qos
            cci_mpf_shim_null
              no_latency_qos
               (
                .clk,
                .fiu(fiu),
                .afu(stgm1_fiu_latency_qos)
                );
        end
    endgenerate


    // ====================================================================
    //
    //  Mandatory MPF edge connection to both the external AFU and FIU
    //  links and to both ends of the MPF pipeline defined in this module.
    //
    // ====================================================================

    cci_mpf_if stgm2_mpf_fiu (.clk);

    // Number of unique write request packets that may be active in MPF.
    // Multi-flit packets count as one entry. Packets become active when
    // a TX request arrives from the AFU and are inactivated as the TX
    // request exits MPF toward the FIU.
    localparam N_WRITE_HEAP_ENTRIES = 128;

    cci_mpf_shim_edge_if
      #(
        .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES)
        )
      edge_if();

    cci_mpf_shim_pwrite_if
      #(
        .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES)
        )
      pwrite();

    cci_mpf_shim_vtp_pt_walk_if pt_walk();

    cci_mpf_shim_edge_fiu
      #(
        .ENABLE_PARTIAL_WRITES(ENABLE_PARTIAL_WRITES),
        .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES),

        // VTP needs to generate loads internally in order to walk the
        // page table.  The reserved bit in Mdata is a location offered
        // to the page table walker to tag internal loads.  The Mdata
        // location is guaranteed to be zero on all requests flowing
        // in to VTP from the AFU.
        .RESERVED_MDATA_IDX(RESERVED_MDATA_IDX)
        )
      mpf_edge_fiu
       (
        .clk,
        .fiu(stgm1_fiu_latency_qos),
        .afu(stgm2_mpf_fiu),
        .afu_edge(edge_if),
        .pt_walk,
        .pwrite
        );


    // ====================================================================
    //
    //  Manage CSRs used by MPF
    //
    // ====================================================================

    cci_mpf_if stgm3_fiu_csrs (.clk);

    cci_mpf_shim_csr
      #(
        .MPF_INSTANCE_ID(MPF_INSTANCE_ID),
        .DFH_MMIO_BASE_ADDR(DFH_MMIO_BASE_ADDR),
        .DFH_MMIO_NEXT_ADDR(DFH_MMIO_NEXT_ADDR),
        .MPF_ENABLE_VTP(ENABLE_VTP),
        .MPF_ENABLE_RSP_ORDER(SORT_READ_RESPONSES),
        .MPF_ENABLE_VC_MAP(MPF_ENABLE_VC_MAP),
        .MPF_ENABLE_LATENCY_QOS(ENABLE_LATENCY_QOS),
        .MPF_ENABLE_WRO(ENFORCE_WR_ORDER),
        .MPF_ENABLE_PWRITE(ENABLE_PARTIAL_WRITES)
        )
      csr
       (
        .clk,
        .fiu(stgm2_mpf_fiu),
        .afu(stgm3_fiu_csrs),
        .csrs(mpf_csrs),
        .events(mpf_csrs)
        );


    // ====================================================================
    //
    //  If VTP is enabled then add a translation server.  All VTP AFU
    //  pipeline shims will sends requests to this shared server.
    //
    // ====================================================================

    localparam N_VTP_PORTS = 2;

    cci_mpf_shim_vtp_svc_if vtp_svc_ports[0 : N_VTP_PORTS-1] ();

    generate
        if (ENABLE_VTP)
        begin : v_to_p
            cci_mpf_svc_vtp
              #(
                .N_VTP_PORTS(N_VTP_PORTS),
                .DEBUG_MESSAGES(0)
                )
              vtp
               (
                .clk,
                .reset,
                .vtp_svc(vtp_svc_ports),
                .pt_walk_walker(pt_walk),
                .pt_walk_client(pt_walk),
                .csrs(mpf_csrs),
                .events(mpf_csrs)
                );
        end
        else
        begin : no_vtp
            // Tie off page table walker
            assign pt_walk.readEn = 1'b0;
            assign pt_walk.readAddr = 'x;
        end
    endgenerate


    // ====================================================================
    //
    //  Instantiate an MPF pipeline composed of the desired shims
    //
    // ====================================================================

    cci_mpf_pipe_std
      #(
        .MAX_ACTIVE_REQS(MAX_ACTIVE_REQS),
        .MPF_INSTANCE_ID(MPF_INSTANCE_ID),
        .DFH_MMIO_BASE_ADDR(DFH_MMIO_BASE_ADDR),
        .DFH_MMIO_NEXT_ADDR(DFH_MMIO_NEXT_ADDR),
        .ENABLE_VTP(ENABLE_VTP),
        .ENABLE_VC_MAP(MPF_ENABLE_VC_MAP),
        .ENABLE_DYNAMIC_VC_MAPPING(ENABLE_DYNAMIC_VC_MAPPING),
        .ENFORCE_WR_ORDER(ENFORCE_WR_ORDER),
        .SORT_READ_RESPONSES(SORT_READ_RESPONSES),
        .PRESERVE_WRITE_MDATA(PRESERVE_WRITE_MDATA),
        .MERGE_DUPLICATE_READS(MERGE_DUPLICATE_READS),
        .ENABLE_PARTIAL_WRITES(ENABLE_PARTIAL_WRITES),
        .N_WRITE_HEAP_ENTRIES(N_WRITE_HEAP_ENTRIES),
        .RESERVED_MDATA_IDX(RESERVED_MDATA_IDX)
        )
      mpf_pipe
       (
        .clk,
        .fiu(stgm3_fiu_csrs),
        .afu,
        .mpf_csrs,
        .edge_if,
        .pwrite,
        .vtp_svc(vtp_svc_ports[0:1])
        );


    // ====================================================================
    //
    //  Track active request counts
    //
    // ====================================================================

    // Leave an extra bit since the counter is at the edge before the
    // initial buffer that limits request counts to MAX_ACTIVE_REQS.
    // The count can thus be higher than MAX_ACTIVE_REQS here.
    typedef logic [$clog2(MAX_ACTIVE_REQS) : 0] t_active_cnt;

    t_active_cnt c0_num_active, c1_num_active;

    logic c0_active_incr, c0_active_decr;
    logic c1_active_incr, c1_active_decr;

    always_comb
    begin
        c0_active_incr = cci_mpf_c0TxIsReadReq(afu.c0Tx);
        c0_active_decr = cci_mpf_c0Rx_isEOP(afu.c0Rx);

        c1_active_incr = cci_mpf_c1TxIsWriteReq(afu.c1Tx) && afu.c1Tx.hdr.base.sop;
        c1_active_decr = cci_c1Rx_isWriteRsp(afu.c1Rx);
    end

    always_ff @(posedge clk)
    begin
        c0NotEmpty <= c0_active_incr || (|(c0_num_active));
        c1NotEmpty <= c1_active_incr || (|(c1_num_active));

        if (c0_active_incr != c0_active_decr)
        begin
            if (c0_active_incr)
                c0_num_active <= c0_num_active + t_active_cnt'(1);
            else
                c0_num_active <= c0_num_active - t_active_cnt'(1);
        end

        if (c1_active_incr != c1_active_decr)
        begin
            if (c1_active_incr)
                c1_num_active <= c1_num_active + t_active_cnt'(1);
            else
                c1_num_active <= c1_num_active - t_active_cnt'(1);
        end

        if (reset)
        begin
            c0NotEmpty <= 1'b0;
            c1NotEmpty <= 1'b0;

            c0_num_active <= t_active_cnt'(0);
            c1_num_active <= t_active_cnt'(0);
        end
    end

endmodule // cci_mpf
