// ***************************************************************************
// Copyright (c) 2017, Intel Corporation
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither the name of Intel Corporation nor the names of its contributors
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
// ***************************************************************************

module magic_number_rom (
  clk,
  reset,
  
  address,
  read,
  burst,
  readdata,
  waitrequest,
  readdatavalid
);

// these values spell out "Wrt_Sync" in ASCII
parameter MAGIC_NUMBER_LOW = 32'h53796E63;
parameter MAGIC_NUMBER_HIGH = 32'h5772745F;

input clk;
input reset;  // this reset needs to be fully synchronous

// this slave has a fixed read latency of 1 and will only let one burst transaction in at a time (max pending reads = 1)
input [1:0] address;  // not going to use address for anything, just need it for Qsys to see a 256B span
input read;
input [2:0] burst;    // using a max burst length of 4 to match the 4CL max burst length of the DMA (avoids Qsys including burst adapter)
output wire [511:0] readdata;
output reg waitrequest;
output reg readdatavalid;  // Avalon-MM doesn't support fixed latency bursting slaves so adding this to make the tools happy



reg [2:0] burst_counter;
wire load_burst_counter;
wire decrement_burst_counter;
wire set_waitrequest;
wire clear_waitrequest;


always @ (posedge clk)
begin
  if (reset)
  begin
    burst_counter <= 3'b000;
  end
  else if (load_burst_counter)
  begin
    burst_counter <= burst;
  end
  else if (decrement_burst_counter)
  begin
    burst_counter <= burst_counter - 1'b1;
  end
end


always @ (posedge clk)
begin
  if (reset)
  begin
    waitrequest <= 1'b0;
  end
  else if (set_waitrequest)
  begin
    waitrequest <= 1'b1;
  end
  else if (clear_waitrequest)
  begin
    waitrequest <= 1'b0;
  end
end


always @ (posedge clk)
begin
  if (reset)
  begin
    readdatavalid <= 1'b0;
  end
  else
  begin
    readdatavalid <= load_burst_counter | (burst_counter > 3'b001);  // this will make sure readdatavalid is high at the same time as waitrequest
  end
end


// keep repeating this value for all beats
assign readdata = {{448{1'b0}}, MAGIC_NUMBER_HIGH, MAGIC_NUMBER_LOW};  // this return data will have a fixed one cycle latency

assign set_waitrequest = (read == 1'b1) & (waitrequest == 1'b0);
assign clear_waitrequest = (burst_counter == 3'b001);  // when burst counter is 1 that is the last beat of the burst so we can deassert waitrequest to let the next read in

assign load_burst_counter = set_waitrequest;
assign decrement_burst_counter = (burst_counter != 3'b000);  // once burst read starts all the beats return back to back


endmodule
