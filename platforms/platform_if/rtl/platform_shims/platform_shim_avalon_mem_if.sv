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


    // ====================================================================
    //  Local memory auto-register for timing
    // ====================================================================

    logic mem_reg_clk[NUM_LOCAL_MEM_BANKS];
    logic mem_reg_reset[NUM_LOCAL_MEM_BANKS];
    avalon_mem_if mem_reg[NUM_LOCAL_MEM_BANKS](mem_reg_clk, mem_reg_reset);

    //
    // How many register stages should be inserted for timing?
    //
    function automatic int numTimingRegStages();
        int n_stages = 0;

        // Were timing registers requested in the AFU JSON?
`ifdef PLATFORM_PARAM_LOCAL_MEMORY_ADD_EXTRA_TIMING_REG_STAGES
        n_stages = `PLATFORM_PARAM_LOCAL_MEMORY_ADD_EXTRA_TIMING_REG_STAGES;
`endif

        // Override the register request if a clock crossing is being
        // inserted here.
        if (LOCAL_MEMORY_CHANGE_CLOCK)
        begin
            // Use at least the recommended number of stages
`ifdef PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_EXTRA_TIMING_REG_STAGES
            if (`PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_EXTRA_TIMING_REG_STAGES > n_stages)
            begin
                n_stages = `PLATFORM_PARAM_LOCAL_MEMORY_SUGGESTED_EXTRA_TIMING_REG_STAGES;
            end
`endif
        end

        return n_stages;
    endfunction

    localparam NUM_TIMING_REG_STAGES = numTimingRegStages();

    genvar b;
    generate
        for (b = 0; b < NUM_LOCAL_MEM_BANKS; b = b + 1)
        begin : pipe
            assign mem_reg_clk[b] = mem_fiu[b].clk;
            assign mem_reg_reset[b] = mem_fiu[b].reset;

            avalon_mem_if_reg
              #(
                .N_REG_STAGES(NUM_TIMING_REG_STAGES)
                )
              mem_pipe
               (
                .mem_fiu(mem_fiu[b]),
                .mem_afu(mem_reg[b])
                );
        end
    endgenerate


    // ====================================================================
    //  Convert local memory signals to the clock domain specified in the
    //  AFU's JSON file.
    // ====================================================================

    generate
        if (LOCAL_MEMORY_CHANGE_CLOCK == 0)
        begin : nc
            //
            // No clock crossing.  Just connect the memory wires.
            //
            for (b = 0; b < NUM_LOCAL_MEM_BANKS; b = b + 1)
            begin : mm_wires
                always_comb
                begin
                    mem_afu_clk[b] = mem_reg[b].clk;
                    mem_afu_reset[b] = mem_reg[b].reset;
                end

                avalon_mem_if_connect mem_connect(.mem_fiu(mem_reg[b]),
                                                  .mem_afu(mem_afu[b]));
            end
        end
        else
        begin : c
            //
            // Cross to the specified clock.
            //

            // Synchronize a reset with the target clock
            (* preserve *) logic [2:0] local_mem_reset_pipe = 3'b111;

            always @(posedge tgt_mem_afu_clk)
            begin
                local_mem_reset_pipe[0] <= mem_reg[0].reset;
                local_mem_reset_pipe[2:1] <= local_mem_reset_pipe[1:0];
            end

            for (b = 0; b < NUM_LOCAL_MEM_BANKS; b = b + 1)
            begin : mm_async
                always_comb
                begin
                    mem_afu_clk[b] = tgt_mem_afu_clk;
                    mem_afu_reset[b] = local_mem_reset_pipe[2];
                end

                // Clock crossing bridge.
                avalon_mem_if_async_shim mem_async_shim
                   (
                    .mem_fiu(mem_reg[b]),
                    .mem_afu(mem_afu[b])
                    );
            end
        end
    endgenerate

endmodule // platform_shim_avalon_mem_if

