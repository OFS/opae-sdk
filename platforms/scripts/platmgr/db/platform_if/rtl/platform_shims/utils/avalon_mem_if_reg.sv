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
// Add a configurable number of pipeline stages between a pair of Avalon
// memory interface objects.  Pipeline stages are complex because of the
// waitrequest protocol.
//

`include "platform_if.vh"

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
import local_mem_cfg_pkg::*;
`endif

module avalon_mem_if_reg
  #(
    // Number of stages to add when registering inputs or outputs
    parameter N_REG_STAGES = 1,

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
    parameter DATA_WIDTH = LOCAL_MEM_DATA_WIDTH,
    parameter ADDR_WIDTH = LOCAL_MEM_ADDR_WIDTH,
    parameter BURST_CNT_WIDTH = LOCAL_MEM_BURST_CNT_WIDTH
`else
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 10,
    parameter BURST_CNT_WIDTH = 4
`endif
    )
   (
    avalon_mem_if.to_fiu mem_fiu,
    avalon_mem_if.to_afu mem_afu
    );

    genvar s;
    generate
        if (N_REG_STAGES == 0)
        begin : wires
            avalon_mem_if_connect conn(.mem_fiu, .mem_afu);
        end
        else
        begin : regs
            // Pipeline stages.
            avalon_mem_if mem_pipe[N_REG_STAGES+1](mem_fiu.clk, mem_fiu.reset);

            // Map mem_fiu to stage 0 (wired) to make the for loop below simpler.
            avalon_mem_if_connect conn0(.mem_fiu(mem_fiu),
                                        .mem_afu(mem_pipe[0]));

            // Inject the requested number of stages
            for (s = 1; s <= N_REG_STAGES; s = s + 1)
            begin : p
                platform_utils_avalon_mm_bridge
                  #(
                    .DATA_WIDTH(DATA_WIDTH),
                    .HDL_ADDR_WIDTH(ADDR_WIDTH),
                    .BURSTCOUNT_WIDTH(BURST_CNT_WIDTH)
                    )
                  bridge
                   (
                    .clk(mem_pipe[s].clk),
                    .reset(mem_pipe[s].reset),

                    .s0_waitrequest(mem_pipe[s].waitrequest),
                    .s0_readdata(mem_pipe[s].readdata),
                    .s0_readdatavalid(mem_pipe[s].readdatavalid),
                    .s0_response(),
                    .s0_burstcount(mem_pipe[s].burstcount),
                    .s0_writedata(mem_pipe[s].writedata),
                    .s0_address(mem_pipe[s].address), 
                    .s0_write(mem_pipe[s].write), 
                    .s0_read(mem_pipe[s].read), 
                    .s0_byteenable(mem_pipe[s].byteenable), 
                    .s0_debugaccess(1'b0),

                    .m0_waitrequest(mem_pipe[s - 1].waitrequest),
                    .m0_readdata(mem_pipe[s - 1].readdata),
                    .m0_readdatavalid(mem_pipe[s - 1].readdatavalid),
                    .m0_response('x),
                    .m0_burstcount(mem_pipe[s - 1].burstcount),
                    .m0_writedata(mem_pipe[s - 1].writedata),
                    .m0_address(mem_pipe[s - 1].address), 
                    .m0_write(mem_pipe[s - 1].write), 
                    .m0_read(mem_pipe[s - 1].read), 
                    .m0_byteenable(mem_pipe[s - 1].byteenable),
                    .m0_debugaccess()
                    );

                // Debugging signal
                assign mem_pipe[s].bank_number = mem_pipe[s-1].bank_number;
            end

            // Map mem_afu to the last stage (wired)
            avalon_mem_if_connect conn1(.mem_fiu(mem_pipe[N_REG_STAGES]),
                                        .mem_afu(mem_afu));
        end
    endgenerate

endmodule // avalon_mem_if_reg
