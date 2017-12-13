// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// synthesis translate_off
`timescale 1ns / 1ps
// synthesis translate_on

// turn off superfluous verilog processor warnings
// altera message_level Level1
// altera message_off 10034 10035 10036 10037 10230 10240 10030 13469 16735 16788

module emif_ddr4_ddr4a_altera_avalon_onchip_memory2_170_yroldmy (
                                                                  // inputs:
                                                                   address,
                                                                   byteenable,
                                                                   chipselect,
                                                                   clk,
                                                                   clken,
                                                                   debugaccess,
                                                                   freeze,
                                                                   reset,
                                                                   reset_req,
                                                                   write,
                                                                   writedata,

                                                                  // outputs:
                                                                   readdata
                                                                )
;

  parameter INIT_FILE = "seq_cal_soft_m20k.hex";


  output  [ 31: 0] readdata;
  input   [ 11: 0] address;
  input   [  3: 0] byteenable;
  input            chipselect;
  input            clk;
  input            clken;
  input            debugaccess;
  input            freeze;
  input            reset;
  input            reset_req;
  input            write;
  input   [ 31: 0] writedata;


wire             clocken0;
wire    [ 31: 0] readdata;
wire             wren;
  assign wren = chipselect & write & debugaccess;
  assign clocken0 = clken & ~reset_req;
  altsyncram the_altsyncram
    (
      .address_a (address),
      .byteena_a (byteenable),
      .clock0 (clk),
      .clocken0 (clocken0),
      .data_a (writedata),
      .q_a (readdata),
      .wren_a (wren)
    );

  defparam the_altsyncram.byte_size = 8,
           the_altsyncram.init_file = INIT_FILE,
           the_altsyncram.lpm_type = "altsyncram",
           the_altsyncram.maximum_depth = 4096,
           the_altsyncram.numwords_a = 4096,
           the_altsyncram.operation_mode = "SINGLE_PORT",
           the_altsyncram.outdata_reg_a = "UNREGISTERED",
           the_altsyncram.ram_block_type = "AUTO",
           the_altsyncram.read_during_write_mode_mixed_ports = "DONT_CARE",
           the_altsyncram.read_during_write_mode_port_a = "DONT_CARE",
           the_altsyncram.width_a = 32,
           the_altsyncram.width_byteena_a = 4,
           the_altsyncram.widthad_a = 12;

  //s1, which is an e_avalon_slave
  //s2, which is an e_avalon_slave

endmodule

