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

`define AFU_ID_AVMM_SLAVE_DATA_WIDTH 64
`define AFU_ID_AVMM_SLAVE_ADDR_WIDTH 3

module afu_id_avmm_slave #(
        //uuid: 331db30c-9885-41ea-9081-f88b8f655caa
        parameter AFU_ID_H = 64'h331D_B30C_9885_41EA,
        parameter AFU_ID_L = 64'h9081_F88B_8F65_5CAA,
        
        parameter DFH_FEATURE_TYPE = 4'b0001,
        parameter DFH_AFU_MINOR_REV = 4'b0000,
        parameter DFH_AFU_MAJOR_REV = 4'b0000,
        parameter DFH_END_OF_LIST = 1'b1,
        parameter DFH_NEXT_OFFSET = 24'b0,
        parameter DFH_FEATURE_ID = 12'b0,
        
        parameter NEXT_AFU_OFFSET = 24'b0,
        
        parameter CREATE_SCRATCH_REG = 1'b0
	)
	(
	//input avmm_chipselect,
	input avmm_write,
	input avmm_read,
	//input 	[(`AFU_ID_AVMM_SLAVE_DATA_WIDTH/8)-1:0]	avmm_byteenable
	input [`AFU_ID_AVMM_SLAVE_DATA_WIDTH-1:0] avmm_writedata,
	output logic [`AFU_ID_AVMM_SLAVE_DATA_WIDTH-1:0] avmm_readdata,
	
	input [`AFU_ID_AVMM_SLAVE_ADDR_WIDTH-1:0]	avmm_address,
	
	input clk,
	input reset
);

	logic [`AFU_ID_AVMM_SLAVE_DATA_WIDTH-1:0] scratch_reg;

	//TODO: always_ff
	always@(posedge clk) begin
		avmm_readdata <= '0;
		scratch_reg <= scratch_reg;
		if(reset) begin
			avmm_readdata <= '0;
			scratch_reg <= '0;
		end
		else begin
			// set the registers on MMIO write request
			// these are user-defined AFU registers at offset 0x40 and 0x41
			if(avmm_write && CREATE_SCRATCH_REG) begin
				case(avmm_address)
					4'h5: scratch_reg <= avmm_writedata;
				endcase
			end
			// serve MMIO read requests
			if(avmm_read) begin
				case(avmm_address)
					// AFU header
					4'h0: avmm_readdata <= {
						DFH_FEATURE_TYPE, // Feature type = AFU
						8'b0,    // reserved
						DFH_AFU_MINOR_REV,    // afu minor revision = 0
						7'b0,    // reserved
						DFH_END_OF_LIST,    // end of DFH list = 1 
						DFH_NEXT_OFFSET,   // next DFH offset = 0
						DFH_AFU_MAJOR_REV,    // afu major revision = 0
						DFH_FEATURE_ID    // feature ID = 0
					};            
					4'h1: avmm_readdata <= AFU_ID_L; // afu id low
					4'h2: avmm_readdata <= AFU_ID_H; // afu id hi
					4'h3: avmm_readdata <= {40'h0, NEXT_AFU_OFFSET}; // next AFU
					4'h4: avmm_readdata <= 64'h0; // reserved
					4'h5: avmm_readdata <= CREATE_SCRATCH_REG ? scratch_reg : 64'h0;
					default:  avmm_readdata <= 64'h0;
				endcase
			end
		end
	end

endmodule
