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
// A simple version of Avalon MM interface register stage insertion.
// Waitrequest is treated as an almost full protocol, with the assumption
// that the FIU end of the connection can handle at least as many
// requests as the depth of the pipeline plus the latency of
// forwarding waitrequest from the FIU side to the AFU side.
//

`include "platform_if.vh"

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
import local_mem_cfg_pkg::*;
`endif

module avalon_mem_if_reg_simple
  #(
    // Number of stages to add when registering inputs or outputs
    parameter N_REG_STAGES = 1,
    parameter N_WAITREQUEST_STAGES = N_REG_STAGES
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
                always_ff @(posedge mem_fiu.clk)
                begin
                    // Waitrequest is a different pipeline, implemented below.
                    mem_pipe[s].waitrequest <= 1'b1;

                    mem_pipe[s].readdata <= mem_pipe[s-1].readdata;
                    mem_pipe[s].readdatavalid <= mem_pipe[s-1].readdatavalid;

                    mem_pipe[s-1].burstcount <= mem_pipe[s].burstcount;
                    mem_pipe[s-1].writedata <= mem_pipe[s].writedata;
                    mem_pipe[s-1].address <= mem_pipe[s].address;
                    mem_pipe[s-1].write <= mem_pipe[s].write;
                    mem_pipe[s-1].read <= mem_pipe[s].read;
                    mem_pipe[s-1].byteenable <= mem_pipe[s].byteenable;

                    if (mem_fiu.reset)
                    begin
                        mem_pipe[s-1].write <= 1'b0;
                        mem_pipe[s-1].read <= 1'b0;
                    end
                end

                // Debugging signal
                assign mem_pipe[s].bank_number = mem_pipe[s-1].bank_number;
            end


            // waitrequest is a shift register, with mem_fiu.waitrequest entering
            // at bit 0.
            logic [N_WAITREQUEST_STAGES:0] mem_waitrequest_pipe;
            assign mem_waitrequest_pipe[0] = mem_fiu.waitrequest;

            always_ff @(posedge mem_fiu.clk)
            begin
                // Shift the waitrequest pipeline
                mem_waitrequest_pipe[N_WAITREQUEST_STAGES:1] <=
                    mem_fiu.reset ? {N_WAITREQUEST_STAGES{1'b1}} :
                                    mem_waitrequest_pipe[N_WAITREQUEST_STAGES-1:0];
            end


            // Map mem_afu to the last stage (wired)
            always_comb
            begin
                mem_afu.waitrequest = mem_waitrequest_pipe[N_WAITREQUEST_STAGES];
                mem_afu.readdata = mem_pipe[N_REG_STAGES].readdata;
                mem_afu.readdatavalid = mem_pipe[N_REG_STAGES].readdatavalid;

                mem_pipe[N_REG_STAGES].burstcount = mem_afu.burstcount;
                mem_pipe[N_REG_STAGES].writedata = mem_afu.writedata;
                mem_pipe[N_REG_STAGES].address = mem_afu.address;
                mem_pipe[N_REG_STAGES].write = mem_afu.write && ! mem_afu.waitrequest;
                mem_pipe[N_REG_STAGES].read = mem_afu.read && ! mem_afu.waitrequest;
                mem_pipe[N_REG_STAGES].byteenable = mem_afu.byteenable;

                // Debugging signal
                mem_afu.bank_number = mem_pipe[N_REG_STAGES].bank_number;
            end
        end
    endgenerate

endmodule // avalon_mem_if_reg_simple
