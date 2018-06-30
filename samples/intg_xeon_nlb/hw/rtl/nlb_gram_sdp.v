// ***************************************************************************
// Copyright (c) 2013-2016, Intel Corporation
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
// Module Name:         gram_sdp.v
// Project:             NLB AFU 
// Description:
//
// ***************************************************************************
// gram_sdp.v: Generic simple dual port RAM with one write port and one read port
// qigang.wang@intel.com Copyright Intel 2008
// edited by pratik marolia on 3/15/2010
// Created 2008Oct16
// referenced Arthur's VHDL version
//
// Generic dual port RAM. This module helps to keep your HDL code architecture
// independent. 
//
// Four modes are supported. All of them use synchronous write and differ only
// in read. 
// Mode  Read Latency   write-to-read latency   Read behavior
// 0     0              1                       asynchronous read
// 1     1              1                       Unknown data on simultaneous access
// 2     1              2                       Old data on simultaneous access
// 3     2              2                       Unknown data on simultaneous access
//
// This module makes use of synthesis tool's automatic RAM recognition feature.
// It can infer distributed as well as block RAM. The type of inferred RAM
// depends on GRAM_STYLE and mode. Mode 0 can only be mapped to
// distributed RAM. Mode 1/2/3 can be mapped to either distributed or block
// RAM. There are three supported values for GRAM_STYLE.
// GRAM_AUTO : Let the tool to decide 
// GRAM_BLCK : Use block RAM
// GRAM_DIST : Use distributed RAM
// 
// Diagram of GRAM:
//
//           +---+      +------------+     +------+
//   raddr --|1/3|______|            |     | 2/3  |
//           |>  |      |            |-----|      |-- dout
//           +---+      |            |     |>     |
//        din __________|   RAM      |     +------+
//      waddr __________|            |
//        we  __________|            |
//        clk __________|\           |
//                      |/           |
//                      +------------+
//
// You can override parameters to customize RAM.
//

`include "nlb_cfg_pkg.vh"

module nlb_gram_sdp (clk,      // input   clock
                we,        // input   write enable
                waddr,     // input   write address with configurable width
                din,       // input   write data with configurable width
                raddr,     // input   read address with configurable width
                dout);     // output  write data with configurable width

    parameter BUS_SIZE_ADDR =4;          // number of bits of address bus
    parameter BUS_SIZE_DATA =32;         // number of bits of data bus
    parameter GRAM_MODE      =2'd3;       // GRAM read mode
    parameter GRAM_STYLE     =`GRAM_AUTO; // GRAM_AUTO, GRAM_BLCK, GRAM_DIST
    
    input                           clk;
    input                           we;
    input   [BUS_SIZE_ADDR-1:0]     waddr;
    input   [BUS_SIZE_DATA-1:0]     din;
    input   [BUS_SIZE_ADDR-1:0]     raddr;
    output  [BUS_SIZE_DATA-1:0]     dout;
    
    (* `GRAM_STYLE = GRAM_STYLE *)
    reg [BUS_SIZE_DATA-1:0] ram [(2**BUS_SIZE_ADDR)-1:0];
    
    reg [BUS_SIZE_ADDR-1:0] raddr_q;
    reg [BUS_SIZE_DATA-1:0] dout;
    reg [BUS_SIZE_DATA-1:0] ram_dout;
    /*synthesis translate_off */
    reg                     driveX;         // simultaneous access detected. Drive X on output
    /*synthesis translate_on */
    
    always @(posedge clk)
    begin
      if (we)
        ram[waddr]<=din; // synchronous write the RAM
    end
    generate
      case (GRAM_MODE)
        0: begin : GEN_ASYN_READ                    // asynchronous read
        //-----------------------------------------------------------------------
             always @(*) dout = ram[raddr];
           end
        1: begin : GEN_SYN_READ                     // synchronous read
        //-----------------------------------------------------------------------
             always @(posedge clk)
             begin                                  /* synthesis translate_off */
                    if(driveX)
                            dout <= 'hx;
                    else                            /* synthesis translate_on */
                            dout <= ram[raddr];
             end
                                                    /*synthesis translate_off */
             always @(*)
             begin
                    driveX = 0;
                                                    
                    if(raddr==waddr && we)
                            driveX  = 1;
                    else    driveX  = 0;            
       
             end                                    /*synthesis translate_on */
             
           end
        2: begin : GEN_FALSE_SYN_READ               // False synchronous read, buffer output
        //-----------------------------------------------------------------------
             always @(*)
             begin
                    ram_dout = ram[raddr];
                                                    /*synthesis translate_off */
                    if(raddr==waddr && we)
                    ram_dout = 'hx;                 /*synthesis translate_on */
             end
             always @(posedge clk)
             begin
                    dout <= ram_dout;
             end
           end
        3: begin : GEN_SYN_READ_BUF_OUTPUT          // synchronous read, buffer output
        //-----------------------------------------------------------------------
             always @(posedge clk)
             begin
                    ram_dout<= ram[raddr];
                    dout    <= ram_dout;
                                                    /*synthesis translate_off */
                    if(driveX)
                    dout    <= 'hx;
                    if(raddr==waddr && we)
                            driveX <= 1;
                    else    driveX <= 0;            /*synthesis translate_on */
             end
           end
      endcase
    endgenerate
    
endmodule
