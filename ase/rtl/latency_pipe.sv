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
 *
 * **************************************************************************
 *
 * Module Info: Generic register block & Latency pipe
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 */

module register
  #(
    parameter REG_WIDTH = 1
    )
   (
    input logic 		 clk,
    input logic 		 rst,
    input logic [REG_WIDTH-1:0]  d,
    output logic [REG_WIDTH-1:0] q
    );


   // DFF behaviour
   always @( posedge clk or posedge rst ) begin
      if (rst)
	q	<= 0;
      else
	q	<= d;
   end

endmodule


/*
 * latency_pipe : A generic N-stage, W-width pipeline that delays
 * input by a known number of clocks
 */
module latency_pipe
  #(
    parameter NUM_DELAY = 5,
    parameter PIPE_WIDTH = 1
    )
   (
    input logic 		  clk,
    input logic 		  rst,
    input logic [PIPE_WIDTH-1:0]  pipe_in,
    output logic [PIPE_WIDTH-1:0] pipe_out
    );

   logic [PIPE_WIDTH-1:0] 	  pipe_out_tmp [0:NUM_DELAY-1];


   // Register stages (instantiated here, not connected)
   genvar 			  ii;
   generate
      for(ii = 1; ii < NUM_DELAY; ii = ii + 1) begin : reg_array_gen
	 register
	       #(
		 .REG_WIDTH (PIPE_WIDTH)
		 )
	 reg_i
	       (
		.clk (clk),
		.rst (rst),
		.d   (pipe_out_tmp[ii-1]),
		.q   (pipe_out_tmp[ii])
		);
      end
   endgenerate

   assign pipe_out_tmp[0] = pipe_in;
   assign pipe_out = pipe_out_tmp[NUM_DELAY-1];

endmodule
