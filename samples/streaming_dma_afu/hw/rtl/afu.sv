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

import ccip_if_pkg::*;
import ccip_avmm_pkg::*;

module afu (
	// ---------------------------global signals-------------------------------------------------
    input afu_clk,
    input	reset,	  //              in    std_logic;           CCI interface reset. The Accelerator IP must use this Reset. ACTIVE HIGH
        
        
`ifdef INCLUDE_DDR4
     input                    DDR4a_USERCLK,
     input                    DDR4a_waitrequest,
     input  [511:0]           DDR4a_readdata,
     input                    DDR4a_readdatavalid,
     output  [6:0]            DDR4a_burstcount,
     output  [511:0]          DDR4a_writedata,
     output  [26:0]           DDR4a_address,
     output                   DDR4a_write,
     output                   DDR4a_read,
     output  [63:0]           DDR4a_byteenable,
     input                    DDR4b_USERCLK,
     input                    DDR4b_waitrequest,
     input  [511:0]           DDR4b_readdata,
     input                    DDR4b_readdatavalid,
     output  [6:0]            DDR4b_burstcount,
     output  [511:0]          DDR4b_writedata,
     output  [26:0]           DDR4b_address,
     output                   DDR4b_write,
     output                   DDR4b_read,
     output  [63:0]           DDR4b_byteenable,
`endif
        
	// ---------------------------IF signals between CCI and AFU  --------------------------------
	input	t_if_ccip_Rx    cp2af_sRxPort,
	input	t_if_ccip_c0_Rx cp2af_mmio_c0rx,
	output	t_if_ccip_Tx	af2cp_sTxPort
);

    wire [31:0]           DDR4a_byte_address;
    wire [31:0]           DDR4b_byte_address;
`ifndef INCLUDE_DDR4
	wire          DDR4a_waitrequest;
	wire [511:0]  DDR4a_readdata;
	wire          DDR4a_readdatavalid;
	wire [6:0]   DDR4a_burstcount;
	wire [511:0] DDR4a_writedata;
	wire [26:0]  DDR4a_address;
	wire         DDR4a_write;
	wire         DDR4a_read;
	wire [63:0]  DDR4a_byteenable;
	wire          DDR4b_waitrequest;
	wire [511:0]  DDR4b_readdata;
	wire          DDR4b_readdatavalid;
	wire [6:0]   DDR4b_burstcount;
	wire [511:0] DDR4b_writedata;
	wire [26:0]  DDR4b_address;
	wire [63:0]  DDR4b_byteenable;
	wire         DDR4b_write;
	wire         DDR4b_read;
	
	`timescale 1 ps / 1 ps
	reg          DDR4a_USERCLK = 0;  
	always begin
		#1875 DDR4a_USERCLK = ~DDR4a_USERCLK;
	end
	
	reg          DDR4b_USERCLK = 0;  
	always begin
		#1800 DDR4b_USERCLK = ~DDR4b_USERCLK;
	end
	
	mem_sim_model ddr4a_inst(
		.clk(DDR4a_USERCLK),
		.reset(reset),
		.avmm_waitrequest(DDR4a_waitrequest),
		.avmm_readdata(DDR4a_readdata),
		.avmm_readdatavalid(DDR4a_readdatavalid),
		.avmm_burstcount(DDR4a_burstcount),
		.avmm_writedata(DDR4a_writedata),
		.avmm_address(DDR4a_address),
		.avmm_write(DDR4a_write),
		.avmm_read(DDR4a_read),
		.avmm_byteenable(DDR4a_byteenable)
	);
	
	mem_sim_model ddr4b_inst(
		.clk(DDR4b_USERCLK),
		.reset(reset),
		.avmm_waitrequest(DDR4b_waitrequest),
		.avmm_readdata(DDR4b_readdata),
		.avmm_readdatavalid(DDR4b_readdatavalid),
		.avmm_burstcount(DDR4b_burstcount),
		.avmm_writedata(DDR4b_writedata),
		.avmm_address(DDR4b_address),
		.avmm_write(DDR4b_write),
		.avmm_read(DDR4b_read),
		.avmm_byteenable(DDR4b_byteenable)
	);
`endif

    // DMA will send out bursts of 4 (max) to the memory controllers
    assign DDR4a_burstcount[6:3] = 4'b0000;
    assign DDR4b_burstcount[6:3] = 4'b0000;

	assign DDR4a_address = DDR4a_byte_address[31:6];
	assign DDR4b_address = DDR4b_byte_address[31:6];
	
	//ccip avmm signals
	wire requestor_avmm_wr_waitrequest;
	wire [CCIP_AVMM_REQUESTOR_DATA_WIDTH-1:0]	requestor_avmm_wr_writedata;
	wire [CCIP_AVMM_REQUESTOR_WR_ADDR_WIDTH-1:0]	requestor_avmm_wr_address;
	wire requestor_avmm_wr_write;
	wire [CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0]	requestor_avmm_wr_burstcount;
   wire [1:0] requestor_avmm_wr_write_response;
   wire requestor_avmm_wr_writeresponsevalid;
	
	wire requestor_avmm_rd_waitrequest;
	wire [CCIP_AVMM_REQUESTOR_DATA_WIDTH-1:0]	requestor_avmm_rd_readdata;
	wire requestor_avmm_rd_readdatavalid;
	wire [CCIP_AVMM_REQUESTOR_DATA_WIDTH-1:0]	requestor_avmm_rd_writedata;
	wire [CCIP_AVMM_REQUESTOR_RD_ADDR_WIDTH-1:0]	requestor_avmm_rd_address;
	wire requestor_avmm_rd_write;
	wire requestor_avmm_rd_read;
	wire [CCIP_AVMM_REQUESTOR_BURST_WIDTH-1:0]	requestor_avmm_rd_burstcount;
	
	wire mmio_avmm_waitrequest;
	wire [CCIP_AVMM_MMIO_DATA_WIDTH-1:0]	mmio_avmm_readdata;
	wire mmio_avmm_readdatavalid;
	wire [CCIP_AVMM_MMIO_DATA_WIDTH-1:0]	mmio_avmm_writedata;
	wire [CCIP_AVMM_MMIO_ADDR_WIDTH-1:0]	mmio_avmm_address;
	wire mmio_avmm_write;
	wire mmio_avmm_read;
	wire [(CCIP_AVMM_MMIO_DATA_WIDTH/8)-1:0]	mmio_avmm_byteenable;

	wire m2s_dma_irq;
  wire s2m_dma_irq;
	
	streaming_dma_test_system the_streaming_dma_test_system (
    .emif_a_avmm_waitrequest       (DDR4a_waitrequest),   // dma_master.waitrequest
		.emif_a_avmm_readdata          (DDR4a_readdata),      //           .readdata
		.emif_a_avmm_readdatavalid     (DDR4a_readdatavalid), //           .readdatavalid
		.emif_a_avmm_burstcount        (DDR4a_burstcount[2:0]),    //           .burstcount
		.emif_a_avmm_writedata         (DDR4a_writedata),     //           .writedata
		.emif_a_avmm_address           (DDR4a_byte_address),       //           .address
		.emif_a_avmm_write             (DDR4a_write),         //           .write
		.emif_a_avmm_read              (DDR4a_read),          //           .read
		.emif_a_avmm_byteenable        (DDR4a_byteenable),    //           .byteenable
		.emif_a_avmm_debugaccess       (),   //           .debugaccess
		
		.emif_b_avmm_waitrequest       (DDR4b_waitrequest),   // dma_master.waitrequest
		.emif_b_avmm_readdata          (DDR4b_readdata),      //           .readdata
		.emif_b_avmm_readdatavalid     (DDR4b_readdatavalid), //           .readdatavalid
		.emif_b_avmm_burstcount        (DDR4b_burstcount[2:0]),    //           .burstcount
		.emif_b_avmm_writedata         (DDR4b_writedata),     //           .writedata
		.emif_b_avmm_address           (DDR4b_byte_address),       //           .address
		.emif_b_avmm_write             (DDR4b_write),         //           .write
		.emif_b_avmm_read              (DDR4b_read),          //           .read
		.emif_b_avmm_byteenable        (DDR4b_byteenable),    //           .byteenable
		.emif_b_avmm_debugaccess       (),   //           .debugaccess
        
    .mmio_avmm_waitrequest         (mmio_avmm_waitrequest),         //       ccip_avmm_mmio.waitrequest
    .mmio_avmm_readdata            (mmio_avmm_readdata),            //                    .readdata
    .mmio_avmm_readdatavalid       (mmio_avmm_readdatavalid),       //                    .readdatavalid
    .mmio_avmm_burstcount          (1'b1),          //                    .burstcount
    .mmio_avmm_writedata           (mmio_avmm_writedata),           //                    .writedata
    .mmio_avmm_address             (mmio_avmm_address),             //                    .address
    .mmio_avmm_write               (mmio_avmm_write),               //                    .write
    .mmio_avmm_read                (mmio_avmm_read),                //                    .read
    .mmio_avmm_byteenable          (mmio_avmm_byteenable),          //                    .byteenable
    .mmio_avmm_debugaccess         (),         //                    .debugaccess
        
    .host_write_address            (requestor_avmm_wr_address[CCIP_AVMM_REQUESTOR_WR_ADDR_WIDTH-2:0]),            //  host_write.address
    .host_write_writedata          (requestor_avmm_wr_writedata),                                                 //            .writedata
    .host_write_write              (requestor_avmm_wr_write),                                                     //            .write
    .host_write_byteenable         (),                                                                            //            .byteenable
    .host_write_burstcount         (requestor_avmm_wr_burstcount),                                                //            .burstcount
    .host_write_response           (requestor_avmm_wr_write_response),                                            //            .response
    .host_write_waitrequest        (requestor_avmm_wr_waitrequest),                                               //            .waitrequest
    .host_write_writeresponsevalid (requestor_avmm_wr_writeresponsevalid),                                        //            .writeresponsevalid

    .host_read_waitrequest    (requestor_avmm_rd_waitrequest),   // ccip_avmm_requestor.waitrequest
    .host_read_readdata       (requestor_avmm_rd_readdata),      //                    .readdata
    .host_read_readdatavalid  (requestor_avmm_rd_readdatavalid), //                    .readdatavalid
    .host_read_burstcount     (requestor_avmm_rd_burstcount),    //                    .burstcount
    .host_read_writedata      (),     //                    .writedata
    .host_read_address        (requestor_avmm_rd_address),       //                    .address
    .host_read_write          (),         //                    .write
    .host_read_read           (requestor_avmm_rd_read),          //                    .read
    .host_read_byteenable     (),    //                    .byteenable
    .host_read_debugaccess    (),   //                    .debugaccess
        
    .m2s_irq_irq                   (m2s_dma_irq),
    .s2m_irq_irq                   (s2m_dma_irq),
        
		.emif_a_clock_clk              (DDR4a_USERCLK),
		.emif_b_clock_clk              (DDR4b_USERCLK),
		.dma_clock_clk                 (afu_clk),
		.reset_reset                   (reset)         // reset.reset
	);

	avmm_ccip_host_rd avmm_ccip_host_rd_inst (
		.clk                  (afu_clk),            //   clk.clk
		.reset                (reset),         // reset.reset
		
		.avmm_waitrequest     (requestor_avmm_rd_waitrequest),
		.avmm_readdata        (requestor_avmm_rd_readdata),
		.avmm_readdatavalid   (requestor_avmm_rd_readdatavalid),
		.avmm_address         (requestor_avmm_rd_address),
		.avmm_read            (requestor_avmm_rd_read),
		.avmm_burstcount      (requestor_avmm_rd_burstcount),
		
		.c0TxAlmFull          (cp2af_sRxPort.c0TxAlmFull),
		.c0rx                 (cp2af_sRxPort.c0),
		.c0tx                 (af2cp_sTxPort.c0)
	);
	
  // tied off write address MSB to avoid issuing write fences to host memory
  assign requestor_avmm_wr_address[CCIP_AVMM_REQUESTOR_WR_ADDR_WIDTH-1] = 1'b0;

	avmm_ccip_host_wr #(
		.ENABLE_INTR(1)
	) avmm_ccip_host_wr_inst (
		.clk                      (afu_clk),            //   clk.clk
		.reset                    (reset),         // reset.reset
		
		.irq                      ({2'b00, s2m_dma_irq, m2s_dma_irq}),
		
		.avmm_waitrequest         (requestor_avmm_wr_waitrequest),
		.avmm_writedata           (requestor_avmm_wr_writedata),
		.avmm_address             (requestor_avmm_wr_address),
		.avmm_write               (requestor_avmm_wr_write),
		.avmm_burstcount          (requestor_avmm_wr_burstcount),
      .avmm_write_response      (requestor_avmm_wr_write_response),
      .avmm_write_responsevalid (requestor_avmm_wr_writeresponsevalid),
		
		.c1TxAlmFull              (cp2af_sRxPort.c1TxAlmFull),
		.c1rx                     (cp2af_sRxPort.c1),
		.c1tx                     (af2cp_sTxPort.c1)
	);
	
	ccip_avmm_mmio ccip_avmm_mmio_inst (
		.avmm_waitrequest        (mmio_avmm_waitrequest),
		.avmm_readdata           (mmio_avmm_readdata),
		.avmm_readdatavalid      (mmio_avmm_readdatavalid),
		.avmm_writedata          (mmio_avmm_writedata),
		.avmm_address            (mmio_avmm_address),
		.avmm_write              (mmio_avmm_write),
		.avmm_read               (mmio_avmm_read),
		.avmm_byteenable         (mmio_avmm_byteenable),
	
		.clk                     (afu_clk),            //   clk.clk
		.reset                   (reset),         // reset.reset
		
		.c0rx                    (cp2af_mmio_c0rx),
		.c2tx                    (af2cp_sTxPort.c2)
	);
	
endmodule

