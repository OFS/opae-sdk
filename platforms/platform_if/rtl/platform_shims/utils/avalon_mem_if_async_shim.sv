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
// Clock crossing bridge for the SystemVerilog avalon_mem_if.
//

`include "platform_if.vh"

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
import local_mem_cfg_pkg::*;
`endif

module avalon_mem_if_async_shim
  #(
`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
    parameter DATA_WIDTH = LOCAL_MEM_DATA_WIDTH,
    parameter ADDR_WIDTH = LOCAL_MEM_ADDR_WIDTH,
    parameter BURST_CNT_WIDTH = LOCAL_MEM_BURST_CNT_WIDTH,
`else
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 10,
    parameter BURST_CNT_WIDTH = 4,
`endif
    parameter COMMAND_FIFO_DEPTH = 128
    )
   (
    avalon_mem_if.to_fiu mem_fiu,
    avalon_mem_if.to_afu mem_afu
    );

    platform_utils_avalon_mm_clock_crossing_bridge
      #(
        .DATA_WIDTH(DATA_WIDTH),
        .HDL_ADDR_WIDTH(ADDR_WIDTH),
        .BURSTCOUNT_WIDTH(BURST_CNT_WIDTH),
        .COMMAND_FIFO_DEPTH(COMMAND_FIFO_DEPTH),
        .RESPONSE_FIFO_DEPTH(2 ** (BURST_CNT_WIDTH + 1))
        )
      avmm_cross
       (
        .s0_clk(mem_afu.clk),
        .s0_reset(mem_afu.reset),

        .m0_clk(mem_fiu.clk),
        .m0_reset(mem_fiu.reset),

        .s0_waitrequest(mem_afu.waitrequest),
        .s0_readdata(mem_afu.readdata),
        .s0_readdatavalid(mem_afu.readdatavalid),
        .s0_burstcount(mem_afu.burstcount),
        .s0_writedata(mem_afu.writedata),
        .s0_address(mem_afu.address),
        .s0_write(mem_afu.write),
        .s0_read(mem_afu.read),
        .s0_byteenable(mem_afu.byteenable),
        .s0_debugaccess(1'b0),

        .m0_waitrequest(mem_fiu.waitrequest),
        .m0_readdata(mem_fiu.readdata),
        .m0_readdatavalid(mem_fiu.readdatavalid),
        .m0_burstcount(mem_fiu.burstcount),
        .m0_writedata(mem_fiu.writedata),
        .m0_address(mem_fiu.address),
        .m0_write(mem_fiu.write),
        .m0_read(mem_fiu.read),
        .m0_byteenable(mem_fiu.byteenable),
        .m0_debugaccess()
        );

endmodule // avalon_mem_if_async_shim
