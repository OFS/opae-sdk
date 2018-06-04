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

//
// This shim manages just avalom_mem_if signals.  It handles both registering
// the signals for timing and crossing to a different clock.
//

`include "platform_if.vh"

module platform_shim_avalon_mem_if
  #(
    parameter NUM_LOCAL_MEM_BANKS = 2
    )
   (
    // AFU clock for memory when a clock crossing is requested
    input  logic        tgt_mem_afu_clk,

    avalon_mem_if.to_fiu mem_fiu[NUM_LOCAL_MEM_BANKS],
    avalon_mem_if.to_afu mem_afu[NUM_LOCAL_MEM_BANKS],
    output logic mem_afu_clk[NUM_LOCAL_MEM_BANKS],
    output logic mem_afu_reset[NUM_LOCAL_MEM_BANKS]
    );

    // ====================================================================
    // While clocking and register stage insertion are logically
    // independent, considering them together leads to an important
    // optimization. The clock crossing FIFO has a large buffer that
    // can be used to turn the standard Avalon MM waitrequest signal into
    // an almost full protocol. The buffer stages become a simple
    // pipeline.
    //
    // When there is no clock crossing FIFO, all register stages must
    // honor the waitrequest protocol.
    // ====================================================================

    //
    // Has the AFU JSON requested a clock crossing for the local memory signals?
    // We compute this here because it affects register stage insertion.
    //
`ifndef PLATFORM_PARAM_LOCAL_MEMORY_CLOCK
    // No local memory clock change.
    localparam LOCAL_MEMORY_CHANGE_CLOCK = 0;
`elsif PLATFORM_PARAM_LOCAL_MEMORY_CLOCK_IS_DEFAULT
    // AFU asked for default clock.
    localparam LOCAL_MEMORY_CHANGE_CLOCK = 0;
`else
    // AFU asked for some other clock.
    localparam LOCAL_MEMORY_CHANGE_CLOCK = 1;
`endif

    //
    // How many register stages should be inserted for timing?
    //
    function automatic int numTimingRegStages();
        int n_stages = 0;

        // Were timing registers requested in the AFU JSON?
`ifdef PLATFORM_PARAM_LOCAL_MEMORY_ADD_TIMING_REG_STAGES
        n_stages = `PLATFORM_PARAM_LOCAL_MEMORY_ADD_TIMING_REG_STAGES;
`endif

        // Override the register request if a clock crossing is being
        // inserted here.
        if (LOCAL_MEMORY_CHANGE_CLOCK)
        begin
            // Use at least the recommended number of stages
`ifdef PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_TIMING_REG_STAGES
            if (`PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_TIMING_REG_STAGES > n_stages)
            begin
                n_stages = `PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_TIMING_REG_STAGES;
            end
`endif
        end

        return n_stages;
    endfunction

    localparam NUM_TIMING_REG_STAGES = numTimingRegStages();


    genvar b;
    generate
        if (LOCAL_MEMORY_CHANGE_CLOCK == 0)
        begin : nc
            //
            // No clock crossing, maybe register stages.
            //
            for (b = 0; b < NUM_LOCAL_MEM_BANKS; b = b + 1)
            begin : pipe
                assign mem_afu_clk[b] = mem_fiu[b].clk;
                assign mem_afu_reset[b] = mem_fiu[b].reset;

                avalon_mem_if_reg
                  #(
                    .N_REG_STAGES(NUM_TIMING_REG_STAGES)
                    )
                  mem_pipe
                   (
                    .mem_fiu(mem_fiu[b]),
                    .mem_afu(mem_afu[b])
                    );
            end
        end
        else
        begin : c
            //
            // Cross to the specified clock.
            //
            avalon_mem_if mem_cross[NUM_LOCAL_MEM_BANKS](mem_afu_clk, mem_afu_reset);

            for (b = 0; b < NUM_LOCAL_MEM_BANKS; b = b + 1)
            begin : mm_async
                // Synchronize a reset with the target clock
                (* preserve *) logic [2:0] local_mem_reset_pipe = 3'b111;

                always @(posedge tgt_mem_afu_clk)
                begin
                    local_mem_reset_pipe[0] <= mem_fiu[b].reset;
                    local_mem_reset_pipe[2:1] <= local_mem_reset_pipe[1:0];
                end

                always_comb
                begin
                    mem_afu_clk[b] = tgt_mem_afu_clk;
                    mem_afu_reset[b] = local_mem_reset_pipe[2];
                end

                // We assume that a single waitrequest signal can propagate faster
                // than the entire bus, so limit the number of stages.
                localparam NUM_WAITREQUEST_STAGES =
                    // If pipeline is 4 stages or fewer then use the pipeline depth.
                    (NUM_TIMING_REG_STAGES <= 4 ? NUM_TIMING_REG_STAGES :
                        // Up to depth 16 pipelines, use 4 waitrequest stages.
                        // Beyond 16 stages, set the waitrequest depth to 1/4 the
                        // base pipeline depth.
                        (NUM_TIMING_REG_STAGES <= 16 ? 4 : (NUM_TIMING_REG_STAGES >> 2)));

                // A few extra stages to avoid off-by-one errors. There is plenty of
                // space in FIFO, so this has no performance consequences.
                localparam NUM_EXTRA_STAGES = (NUM_TIMING_REG_STAGES != 0) ? 4 : 0;

                // Set the almost full threshold to satisfy the buffering pipeline depth
                // plus the depth of the waitrequest pipeline plus a little extra to
                // avoid having to worry about off-by-one errors.
                localparam NUM_ALMFULL_SLOTS = NUM_TIMING_REG_STAGES +
                                               NUM_WAITREQUEST_STAGES +
                                               NUM_EXTRA_STAGES;

                // Clock crossing bridge
                avalon_mem_if_async_shim
                  #(
                    .COMMAND_ALMFULL_THRESHOLD(NUM_ALMFULL_SLOTS)
                    )
                  mem_async_shim
                   (
                    .mem_fiu(mem_fiu[b]),
                    .mem_afu(mem_cross[b])
                    );

                // Add requested register stages on the AFU side of the clock crossing.
                // In this case the register stages are a simple pipeline because
                // the clock crossing FIFO reserves space for these stages to drain
                // after waitrequest is asserted.
                avalon_mem_if_reg_simple
                  #(
                    .N_REG_STAGES(NUM_TIMING_REG_STAGES),
                    .N_WAITREQUEST_STAGES(NUM_WAITREQUEST_STAGES)
                    )
                  mem_pipe
                   (
                    .mem_fiu(mem_cross[b]),
                    .mem_afu(mem_afu[b])
                    );
            end
        end
    endgenerate

endmodule // platform_shim_avalon_mem_if

