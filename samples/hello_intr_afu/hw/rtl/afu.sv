// ***************************************************************************
// Copyright (c) 2013-2017, Intel Corporation All Rights Reserved.
// The source code contained or described herein and all  documents related to
// the  source  code  ("Material")  are  owned by  Intel  Corporation  or  its
// suppliers  or  licensors.    Title  to  the  Material  remains  with  Intel
// Corporation or  its suppliers  and licensors.  The Material  contains trade
// secrets and  proprietary  and  confidential  information  of  Intel or  its
// suppliers and licensors.  The Material is protected  by worldwide copyright
// and trade secret laws and treaty provisions. No part of the Material may be
// copied,    reproduced,    modified,    published,     uploaded,     posted,
// transmitted,  distributed,  or  disclosed  in any way without Intel's prior
// express written permission.
// ***************************************************************************
//
// Module Name:         afu.sv
// Project:             Interrupt AFU
// Modified:            PSG - ADAPT
// Description:         Simple AFU that demonstrates user-generated interrupt.
//                      Write to AFU CSR word address 0x28 (byte address 0xA0)
//                      generates a CCIP interrupt request packet
// 

`include "platform_if.vh"

// Generate interrupt
function automatic t_if_ccip_c1_Tx ccip_genInterrupt(
    input [1:0] usr_intr_id
);
    // use c1 to write interrupt ccip packets
    t_if_ccip_c1_Tx c1;
    t_ccip_c1_ReqIntrHdr intrHdr;

    intrHdr.rsvd1 = 0;
    intrHdr.req_type = eREQ_INTR;
    intrHdr.rsvd0 = 0;
    intrHdr.id = usr_intr_id;

    c1.hdr = t_ccip_c1_ReqMemHdr'(intrHdr);
    c1.valid = 1;
    c1.data = 0;

    return c1;
endfunction

module afu (
        // ---------------------------global signals-------------------------------------------------
        input	Clk_400,	  //              in    std_logic;           Core clock. CCI interface is synchronous to this clock.
        input	SoftReset,	  //              in    std_logic;           CCI interface reset. The Accelerator IP must use this Reset. ACTIVE HIGH
        // ---------------------------IF signals between CCI and AFU  --------------------------------
`ifdef INCLUDE_DDR4
        input	wire		DDR4_USERCLK,
        input	wire		DDR4a_waitrequest,
        input	wire [511:0]	DDR4a_readdata,
        input	wire		DDR4a_readdatavalid,
        output	wire [6:0]	DDR4a_burstcount,
        output	reg  [511:0]	DDR4a_writedata,
        output	reg  [25:0]	DDR4a_address,
        output	reg		DDR4a_write,
        output	reg		DDR4a_read,
        output	wire [63:0]	DDR4a_byteenable,
        input	wire		DDR4b_waitrequest,
        input	wire [511:0]	DDR4b_readdata,
        input	wire		DDR4b_readdatavalid,
        output	wire [6:0]	DDR4b_burstcount,
        output	reg  [511:0]	DDR4b_writedata,
        output	reg  [25:0]	DDR4b_address,
        output	reg		DDR4b_write,
        output	reg		DDR4b_read,
        output	wire [63:0]	DDR4b_byteenable,
`endif
        input	t_if_ccip_Rx	cp2af_sRxPort,
        output	t_if_ccip_Tx	af2cp_sTxPort
);

        //Hello_AFU ID
        localparam HELLO_AFU_ID_H = 64'h850A_DCC2_6CEB_4B22;
        localparam HELLO_AFU_ID_L = 64'h9722_D433_75B6_1C66;

        logic  [63:0] scratch_reg = 0;
        // a write to the intr_reg triggers an interrupt request
        reg           intr_reg;
        reg [1:0]     usr_intr_id;

        // cast c0 header into ReqMmioHdr
        t_ccip_c0_ReqMmioHdr mmioHdr;
        t_ccip_c1_ReqIntrHdr intrHdr;
        assign mmioHdr = t_ccip_c0_ReqMmioHdr'(cp2af_sRxPort.c0.hdr);
        assign intrHdr = t_ccip_c1_ReqIntrHdr'(cp2af_sRxPort.c1.hdr);

        always@(posedge Clk_400) begin
            if(SoftReset) begin
                af2cp_sTxPort.c1.hdr        <= '0;
                af2cp_sTxPort.c1.valid      <= '0;
                af2cp_sTxPort.c1.data       <= '0;
                af2cp_sTxPort.c0.hdr        <= '0;
                af2cp_sTxPort.c0.valid      <= '0;
                af2cp_sTxPort.c2.hdr        <= '0;
                af2cp_sTxPort.c2.data       <= '0;
                af2cp_sTxPort.c2.mmioRdValid <= '0;
                scratch_reg    <= '0;
            end
            else begin
                af2cp_sTxPort.c2.mmioRdValid <= 0;
                // set the registers on MMIO write request
                // these are user-defined AFU registers at offset 0x40 and 0x41
                if(cp2af_sRxPort.c0.mmioWrValid == 1) begin
                    case(mmioHdr.address)
                        16'h0020: scratch_reg <= cp2af_sRxPort.c0.data[63:0];
                        16'h0028: af2cp_sTxPort.c1 <= ccip_genInterrupt(usr_intr_id);
                        16'h0030: usr_intr_id <= cp2af_sRxPort.c0.data[1:0];
                    endcase
                end
                else begin
                  af2cp_sTxPort.c1.hdr   <= '0;
                  af2cp_sTxPort.c1.valid <= '0;
                  af2cp_sTxPort.c1.data  <= '0;
                end
              // serve MMIO read requests
              if(cp2af_sRxPort.c0.mmioRdValid == 1) begin
                  af2cp_sTxPort.c2.hdr.tid <= mmioHdr.tid; // copy TID
                  case(mmioHdr.address)
                      // AFU header
                      16'h0000: af2cp_sTxPort.c2.data <= {
                         4'b0001, // Feature type = AFU
                         8'b0,    // reserved
                         4'b0,    // afu minor revision = 0
                         7'b0,    // reserved
                         1'b1,    // end of DFH list = 1
                         24'b0,   // next DFH offset = 0
                         4'b0,    // afu major revision = 0
                         12'b0    // feature ID = 0
                      };
                      16'h0002: af2cp_sTxPort.c2.data <= HELLO_AFU_ID_L; // afu id low
                      16'h0004: af2cp_sTxPort.c2.data <= HELLO_AFU_ID_H; // afu id hi
                      16'h0006: af2cp_sTxPort.c2.data <= 64'h0; // next AFU
                      16'h0008: af2cp_sTxPort.c2.data <= 64'h0; // reserved
                      16'h0020: af2cp_sTxPort.c2.data <= scratch_reg; // Scratch Register
                      16'h0028: af2cp_sTxPort.c2.data <= usr_intr_id; // User interrupt id register
                      default:  af2cp_sTxPort.c2.data <= 64'h0;
                  endcase
                  af2cp_sTxPort.c2.mmioRdValid <= 1; // post response
              end
          end
      end
`ifdef INCLUDE_DDR4
        always @(posedge DDR4_USERCLK) begin
            if(SoftReset) begin
                DDR4a_write <= 1'b0;
                DDR4a_read  <= 1'b0;
                DDR4b_write <= 1'b0;
                DDR4b_read  <= 1'b0;
            end
        end

assign DDR4a_burstcount = 7'b1;
assign DDR4a_byteenable = 64'hFFFF_FFFF_FFFF_FFFF;
assign DDR4b_burstcount = 7'b1;
assign DDR4b_byteenable = 64'hFFFF_FFFF_FFFF_FFFF;
`endif
endmodule
