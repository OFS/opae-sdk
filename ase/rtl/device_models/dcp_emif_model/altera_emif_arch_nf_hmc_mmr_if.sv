// (C) 2001-2017 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other
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



///////////////////////////////////////////////////////////////////////////////
// This module is responsible for exposing the MMR Avalon interfaces through which
// soft logic interacts with the Hard Memory Controller inside the tile.
// The tile WYSIWYG blocks collapse the individual Avalon signals into big
// buses. This module re-wires the big buses into proper Avalon interfaces.
//
///////////////////////////////////////////////////////////////////////////////
module altera_emif_arch_nf_hmc_mmr_if #(

   // Definition of port widths for "ctrl_mmr" interface (auto-generated)
   parameter PORT_CTRL_MMR_SLAVE_ADDRESS_WIDTH             = 1,
   parameter PORT_CTRL_MMR_SLAVE_RDATA_WIDTH               = 1,
   parameter PORT_CTRL_MMR_SLAVE_WDATA_WIDTH               = 1,
   parameter PORT_CTRL_MMR_SLAVE_BCOUNT_WIDTH              = 1
) (
   // MMR signals between core and HMC
   input  logic [33:0]                                        ctl2core_mmr_0,
   output logic [50:0]                                        core2ctl_mmr_0,
   input  logic [33:0]                                        ctl2core_mmr_1,
   output logic [50:0]                                        core2ctl_mmr_1,

   // Ports for "ctrl_mmr" interface (auto-generated)
   output logic                                               mmr_slave_waitrequest_0,
   input  logic                                               mmr_slave_read_0,
   input  logic                                               mmr_slave_write_0,
   input  logic [PORT_CTRL_MMR_SLAVE_ADDRESS_WIDTH-1:0]       mmr_slave_address_0,
   output logic [PORT_CTRL_MMR_SLAVE_RDATA_WIDTH-1:0]         mmr_slave_readdata_0,
   input  logic [PORT_CTRL_MMR_SLAVE_WDATA_WIDTH-1:0]         mmr_slave_writedata_0,
   input  logic [PORT_CTRL_MMR_SLAVE_BCOUNT_WIDTH-1:0]        mmr_slave_burstcount_0,
   input  logic                                               mmr_slave_beginbursttransfer_0,
   output logic                                               mmr_slave_readdatavalid_0,

   output logic                                               mmr_slave_waitrequest_1,
   input  logic                                               mmr_slave_read_1,
   input  logic                                               mmr_slave_write_1,
   input  logic [PORT_CTRL_MMR_SLAVE_ADDRESS_WIDTH-1:0]       mmr_slave_address_1,
   output logic [PORT_CTRL_MMR_SLAVE_RDATA_WIDTH-1:0]         mmr_slave_readdata_1,
   input  logic [PORT_CTRL_MMR_SLAVE_WDATA_WIDTH-1:0]         mmr_slave_writedata_1,
   input  logic [PORT_CTRL_MMR_SLAVE_BCOUNT_WIDTH-1:0]        mmr_slave_burstcount_1,
   input  logic                                               mmr_slave_beginbursttransfer_1,
   output logic                                               mmr_slave_readdatavalid_1
);
   timeunit 1ns;
   timeprecision 1ps;

   assign core2ctl_mmr_0[9:0]        = mmr_slave_address_0;
   assign core2ctl_mmr_0[13:10]      = 'b0;
   assign core2ctl_mmr_0[45:14]      = mmr_slave_writedata_0;
   assign core2ctl_mmr_0[46]         = mmr_slave_write_0;
   assign core2ctl_mmr_0[47]         = mmr_slave_read_0;
   assign core2ctl_mmr_0[49:48]      = mmr_slave_burstcount_0;
   assign core2ctl_mmr_0[50]         = mmr_slave_beginbursttransfer_0;

   assign mmr_slave_readdata_0       = ctl2core_mmr_0[31:0];
   assign mmr_slave_readdatavalid_0  = ctl2core_mmr_0[32];
   assign mmr_slave_waitrequest_0    = ctl2core_mmr_0[33];

   assign core2ctl_mmr_1[9:0]        = mmr_slave_address_1;
   assign core2ctl_mmr_1[13:10]      = 'b0;
   assign core2ctl_mmr_1[45:14]      = mmr_slave_writedata_1;
   assign core2ctl_mmr_1[46]         = mmr_slave_write_1;
   assign core2ctl_mmr_1[47]         = mmr_slave_read_1;
   assign core2ctl_mmr_1[49:48]      = mmr_slave_burstcount_1;
   assign core2ctl_mmr_1[50]         = mmr_slave_beginbursttransfer_1;

   assign mmr_slave_readdata_1       = ctl2core_mmr_1[31:0];
   assign mmr_slave_readdatavalid_1  = ctl2core_mmr_1[32];
   assign mmr_slave_waitrequest_1    = ctl2core_mmr_1[33];

endmodule

