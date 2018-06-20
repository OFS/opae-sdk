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
// Module Name:         arbiter.v
// Project:             NLB AFU 
// Description:
//
// ***************************************************************************
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//                                         Arbiter
//  ------------------------------------------------------------------------------------------------------------------------------------------------
//
// This module instantiates different test AFUs, and connect them up to the arbiter.

`default_nettype none
module arbiter #(parameter PEND_THRESH=1, ADDR_LMT=20, MDATA=14)
(

       // ---------------------------global signals-------------------------------------------------
       Clk_400               ,        // in    std_logic;  -- Core clock

       ab2re_WrAddr,                   // [ADDR_LMT-1:0]        app_cnt:           write address
       ab2re_WrTID,                    // [15:0]                app_cnt:           meta data
       ab2re_WrDin,                    // [511:0]               app_cnt:           Cache line data
       ab2re_WrFence,                  //                       app_cnt:           write fence
       ab2re_WrEn,                     //                       app_cnt:           write enable
       re2ab_WrSent,                   //                       app_cnt:           write issued
       re2ab_WrAlmFull,                //                       app_cnt:           write fifo almost full
       
       ab2re_RdAddr,                   // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
       ab2re_RdTID,                    // [15:0]                app_cnt:           meta data
       ab2re_RdEn,                     //                       app_cnt:           read enable
       re2ab_RdSent,                   //                       app_cnt:           read issued

       re2ab_RdRspValid,               //                       app_cnt:           read response valid
       re2ab_UMsgValid,                //                       arbiter:           UMsg valid
       re2ab_CfgValid,                 //                       arbiter:           Cfg valid
       re2ab_RdRsp,                    // [15:0]                app_cnt:           read response header
       re2ab_RdData,                   // [511:0]               app_cnt:           read data
       re2ab_stallRd,                  //                       app_cnt:           stall read requests FOR LPBK1

       re2ab_WrRspValid,               //                       app_cnt:           write response valid
       re2ab_WrRsp,                    // [ADDR_LMT-1:0]        app_cnt:           write response header
       re2xy_go,                       //                       requestor:         start the test
       re2xy_src_addr,                 // [31:0]                requestor:         src address
       re2xy_dst_addr,                 // [31:0]                requestor:         destination address
       re2xy_NumLines,                 // [31:0]                requestor:         number of cache lines
       re2xy_stride,                   // [31:0]              requestor:      stride value
       re2xy_Cont,                     //                       requestor:         continuous mode
       re2xy_wrdin_msb,                //                     requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns
        re2xy_test_cfg,                 // [7:0]                 requestor:         8-bit test cfg register.
       re2ab_Mode,                     // [2:0]                 requestor:         test mode
       ab2re_TestCmp,                  //                       arbiter:           Test completion flag
       ab2re_ErrorInfo,                // [255:0]               arbiter:           error information
       ab2re_ErrorValid,               //                       arbiter:           test has detected an error
       cr2s1_csr_write,
       test_Resetb,                    //                       requestor:         rest the app
     
       ab2re_RdLen,
       ab2re_RdSop,
       ab2re_WrLen,
       ab2re_WrSop,
          
       re2ab_RdRspFormat,
       re2ab_RdRspCLnum,
       re2ab_WrRspFormat,
       re2ab_WrRspCLnum,
       re2xy_multiCL_len                          // Default is 0 which implies single CL  
);
   
   input  logic                   Clk_400;               //                      ccip_intf:            Clk_400
   
   output logic  [ADDR_LMT-1:0]   ab2re_WrAddr;           // [ADDR_LMT-1:0]        app_cnt:           Writes are guaranteed to be accepted
   output logic  [15:0]           ab2re_WrTID;            // [15:0]                app_cnt:           meta data
   output logic  [511:0]          ab2re_WrDin;            // [511:0]               app_cnt:           Cache line data
   output logic                   ab2re_WrFence;          //                       app_cnt:           write fence.
   output logic                   ab2re_WrEn;             //                       app_cnt:           write enable
   input  logic                   re2ab_WrSent;           //                       app_cnt:           write issued
   input  logic                   re2ab_WrAlmFull;        //                       app_cnt:           write fifo almost full
   
   output logic  [ADDR_LMT-1:0]   ab2re_RdAddr;           // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
   output logic  [15:0]           ab2re_RdTID;            // [15:0]                app_cnt:           meta data
   output logic                   ab2re_RdEn;             //                       app_cnt:           read enable
   input  logic                   re2ab_RdSent;           //                       app_cnt:           read issued
   
   input  logic                   re2ab_RdRspValid;       //                       app_cnt:           read response valid
   input  logic                   re2ab_UMsgValid;        //                       arbiter:           UMsg valid
   input  logic                   re2ab_CfgValid;         //                       arbiter:           Cfg valid
   input  logic [15:0]            re2ab_RdRsp;            // [15:0]                app_cnt:           read response header
   input  logic [511:0]           re2ab_RdData;           // [511:0]               app_cnt:           read data
   input  logic                   re2ab_stallRd;          //                       app_cnt:           stall read requests FOR LPBK1
   
   input  logic                   re2ab_WrRspValid;       //                       app_cnt:           write response valid
   input  logic [15:0]            re2ab_WrRsp;            // [15:0]                app_cnt:           write response header
   
   input  logic                   re2xy_go;               //                       requestor:         start of frame recvd
   input  logic [31:0]            re2xy_src_addr;         // [31:0]                requestor:         src address
   input  logic [31:0]            re2xy_dst_addr;         // [31:0]                requestor:         destination address
   input  logic [31:0]            re2xy_NumLines;         // [31:0]                requestor:         number of cache lines
   input  logic [31:0]             re2xy_stride;          // [31:0]              requestor:      stride value
   input  logic                   re2xy_Cont;             //                       requestor:         continuous mode
   input  logic [7:0]             re2xy_test_cfg;         // [7:0]                 requestor:         8-bit test cfg register.
   input  logic [2:0]             re2ab_Mode;             // [2:0]                 requestor:         test mode
   input  logic                re2xy_wrdin_msb;        //                       requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns
   
   output logic                   ab2re_TestCmp;          //                       arbiter:           Test completion flag
   output logic  [255:0]          ab2re_ErrorInfo;        // [255:0]               arbiter:           error information
   output logic                   ab2re_ErrorValid;       //                       arbiter:           test has detected an error
   
   input  logic                   cr2s1_csr_write;
   input  logic                   test_Resetb;
   
   output logic  [1:0]            ab2re_RdLen;
   output logic                   ab2re_RdSop;
   output logic  [1:0]            ab2re_WrLen;
   output logic                   ab2re_WrSop;
        
   input  logic                   re2ab_RdRspFormat; // TODO: This is not applicable. Even Multi CL Rds return individual unpacked response always
   input  logic [1:0]             re2ab_RdRspCLnum;  // For unpacked rd rsp, OoO
   input  logic                   re2ab_WrRspFormat; // Packed or unpacked response for multi CL Writes.
   input  logic [1:0]             re2ab_WrRspCLnum;  // for unpacked wr rsp, could be OoO
   input  logic [1:0]             re2xy_multiCL_len; 
   
   //------------------------------------------------------------------------------------------------------------------------
   
   // Test Modes
   //--------------------------------------------------------
   localparam              M_LPBK1         = 3'b000;
   localparam              M_READ          = 3'b001;
   localparam              M_WRITE         = 3'b010;
   localparam              M_TRPUT         = 3'b011;
   localparam              M_SW1           = 3'b111;
   //--------------------------------------------------------

  
   //------------------------------------------------------------------------------------------------------------------------
   //      test_lpbk1 signal declarations
   //------------------------------------------------------------------------------------------------------------------------
   
   wire [ADDR_LMT-1:0]     l12ab_WrAddr;           // [ADDR_LMT-1:0]        app_cnt:           write address
   wire [15:0]             l12ab_WrTID;            // [15:0]                app_cnt:           meta data
   wire [511:0]            l12ab_WrDin;            // [511:0]               app_cnt:           Cache line data
   wire                    l12ab_WrEn;             //                       app_cnt:           write enable
   reg                     ab2l1_WrSent;           //                       app_cnt:           write issued
   reg                     ab2l1_WrAlmFull;        //                       app_cnt:           write fifo almost full
   
   wire [ADDR_LMT-1:0]     l12ab_RdAddr;           // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
   wire [15:0]             l12ab_RdTID;            // [15:0]                app_cnt:           meta data
   wire                    l12ab_RdEn;             //                       app_cnt:           read enable
   reg                     ab2l1_RdSent;           //                       app_cnt:           read issued
   
   reg                     ab2l1_RdRspValid;       //                       app_cnt:           read response valid
   reg                     ab2l1_UMsgValid;        //                       app_cnt:           UMsg valid
   reg                     ab2l1_CfgValid;         //                       app_cnt:           Cfg valid
   reg [15:0]              ab2l1_RdRsp;            // [15:0]                app_cnt:           read response header
   reg [ADDR_LMT-1:0]      ab2l1_RdRspAddr;        // [ADDR_LMT-1:0]        app_cnt:           read response address
   reg [511:0]             ab2l1_RdData;           // [511:0]               app_cnt:           read data
   reg                     ab2l1_stallRd;          //                       app_cnt:           read stall
   
   reg                     ab2l1_WrRspValid;       //                       app_cnt:           write response valid
   reg [15:0]              ab2l1_WrRsp;            // [15:0]                app_cnt:           write response header
   reg [ADDR_LMT-1:0]      ab2l1_WrRspAddr;        // [Addr_LMT-1:0]        app_cnt:           write response address
   
   wire                    l12ab_TestCmp;          //                       arbiter:           Test completion flag
   wire [255:0]            l12ab_ErrorInfo;        // [255:0]               arbiter:           error information
   wire                    l12ab_ErrorValid;       //                       arbiter:           test has detected an error
   
   logic                   ab2l1_RdRspFormat;
   logic [1:0]             ab2l1_RdRspCLnum; 
   logic                   ab2l1_WrRspFormat;
   logic [1:0]             ab2l1_WrRspCLnum;
   
   logic [1:0]             l12ab_RdLen;
   logic                   l12ab_RdSop;
   logic [1:0]             l12ab_WrLen;
   logic                   l12ab_WrSop;
   
   //------------------------------------------------------------------------------------------------------------------------
   //      test_trput signal declarations
   //------------------------------------------------------------------------------------------------------------------------
   reg  [1:0]              ab2rw_Mode;           //                       arb:               1- reads only test, 0- writes only test
   wire [ADDR_LMT-1:0]     rw2ab_WrAddr;           // [ADDR_LMT-1:0]        app_cnt:           write address
   wire [15:0]             rw2ab_WrTID;            // [15:0]                app_cnt:           meta data
   wire [511:0]            rw2ab_WrDin;            // [511:0]               app_cnt:           Cache line data
   wire                    rw2ab_WrEn;             //                       app_cnt:           write enable
   reg                     ab2rw_WrSent;           //                       app_cnt:           write issued
   reg                     ab2rw_WrAlmFull;        //                       app_cnt:           write fifo almost full
   
   wire [ADDR_LMT-1:0]     rw2ab_RdAddr;           // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
   wire [15:0]             rw2ab_RdTID;            // [15:0]                app_cnt:           meta data
   wire                    rw2ab_RdEn;             //                       app_cnt:           read enable
   reg                     ab2rw_RdSent;           //                       app_cnt:           read issued
   
   reg                     ab2rw_RdRspValid;       //                       app_cnt:           read response valid
   reg                     ab2rw_UMsgValid;        //                       app_cnt:           UMsg valid
   reg                     ab2rw_CfgValid;         //                       app_cnt:           Cfg valid
   reg [15:0]              ab2rw_RdRsp;            // [15:0]                app_cnt:           read response header
   reg [ADDR_LMT-1:0]      ab2rw_RdRspAddr;        // [ADDR_LMT-1:0]        app_cnt:           read response address
   reg [511:0]             ab2rw_RdData;           // [511:0]               app_cnt:           read data
   
   reg                     ab2rw_WrRspValid;       //                       app_cnt:           write response valid
   reg [15:0]              ab2rw_WrRsp;            // [15:0]                app_cnt:           write response header
   reg [ADDR_LMT-1:0]      ab2rw_WrRspAddr;        // [Addr_LMT-1:0]        app_cnt:           write response address
   
   wire                    rw2ab_TestCmp;          //                       arbiter:           Test completion flag
   wire [255:0]            rw2ab_ErrorInfo;        // [255:0]               arbiter:           error information
   wire                    rw2ab_ErrorValid;       //                       arbiter:           test has detected an error
   
   logic                   ab2rw_RdRspFormat;
   logic [1:0]             ab2rw_RdRspCLnum; 
   logic                   ab2rw_WrRspFormat;
   logic [1:0]             ab2rw_WrRspCLnum;
   
   logic [1:0]             rw2ab_RdLen;
   logic                   rw2ab_RdSop;
   logic [1:0]             rw2ab_WrLen;
   logic                   rw2ab_WrSop;
    
   //------------------------------------------------------------------------------------------------------------------------
   //      test_sw1 signal declarations
   //------------------------------------------------------------------------------------------------------------------------
   
   wire [ADDR_LMT-1:0]     s12ab_WrAddr;           // [ADDR_LMT-1:0]        app_cnt:           write address
   wire [15:0]             s12ab_WrTID;            // [15:0]                app_cnt:           meta data
   wire [511:0]            s12ab_WrDin;            // [511:0]               app_cnt:           Cache line data
   wire                    s12ab_WrEn;             //                       app_cnt:           write enable
   wire                    s12ab_WrFence;          //                       app_cnt:           write fence 
   reg                     ab2s1_WrSent;           //                       app_cnt:           write issued
   reg                     ab2s1_WrAlmFull;        //                       app_cnt:           write fifo almost full
   
   wire [ADDR_LMT-1:0]     s12ab_RdAddr;           // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
   wire [15:0]             s12ab_RdTID;            // [15:0]                app_cnt:           meta data
   wire                    s12ab_RdEn;             //                       app_cnt:           read enable
   reg                     ab2s1_RdSent;           //                       app_cnt:           read issued
   
   reg                     ab2s1_RdRspValid;       //                       app_cnt:           read response valid
   reg                     ab2s1_UMsgValid;        //                       app_cnt:           UMsg valid
   reg                     ab2s1_CfgValid;         //                       app_cnt:           Cfg valid
   reg [15:0]              ab2s1_RdRsp;            // [15:0]                app_cnt:           read response header
   reg [ADDR_LMT-1:0]      ab2s1_RdRspAddr;        // [ADDR_LMT-1:0]        app_cnt:           read response address
   reg [511:0]             ab2s1_RdData;           // [511:0]               app_cnt:           read data
   
   reg                     ab2s1_WrRspValid;       //                       app_cnt:           write response valid
   reg [15:0]              ab2s1_WrRsp;            // [15:0]                app_cnt:           write response header
   reg [ADDR_LMT-1:0]      ab2s1_WrRspAddr;        // [Addr_LMT-1:0]        app_cnt:           write response address
   
   wire                    s12ab_TestCmp;          //                       arbiter:           Test completion flag
   wire [255:0]            s12ab_ErrorInfo;        // [255:0]               arbiter:           error information
   wire                    s12ab_ErrorValid;       //                       arbiter:           test has detected an error
   
   // local variables
   reg                     re2ab_RdRspValid_q, re2ab_RdRspValid_qq;
   reg                     re2ab_WrRspValid_q, re2ab_WrRspValid_qq;
   reg                     re2ab_UMsgValid_q, re2ab_UMsgValid_qq;
   reg                     re2ab_CfgValid_q, re2ab_CfgValid_qq; 
   reg [15:0]              re2ab_RdRsp_q, re2ab_RdRsp_qq;
   reg [15:0]              re2ab_WrRsp_q, re2ab_WrRsp_qq;
   reg [511:0]             re2ab_RdData_q, re2ab_RdData_qq;
   
   logic                   re2ab_RdRspFormat_q, re2ab_RdRspFormat_qq;
   logic [1:0]             re2ab_RdRspCLnum_q,  re2ab_RdRspCLnum_qq;
   logic                   re2ab_WrRspFormat_q, re2ab_WrRspFormat_qq;
   logic [1:0]             re2ab_WrRspCLnum_q, re2ab_WrRspCLnum_qq;

   //------------------------------------------------------------------------------------------------------------------------
   // Arbitrataion Memory instantiation
   //------------------------------------------------------------------------------------------------------------------------
   wire [ADDR_LMT-1:0]     arbmem_rd_dout;
   wire [ADDR_LMT-1:0]     arbmem_wr_dout;
   
   nlb_gram_sdp #(.BUS_SIZE_ADDR(MDATA),
              .BUS_SIZE_DATA(ADDR_LMT),
              .GRAM_MODE(2'd3)
              )arb_rd_mem 
            (
                .clk  (Clk_400),
                .we   (ab2re_RdEn),        
                .waddr(ab2re_RdTID[MDATA-1:0]),     
                .din  (ab2re_RdAddr),       
                .raddr(re2ab_RdRsp[MDATA-1:0]),     
                .dout (arbmem_rd_dout )
            );     
   
   nlb_gram_sdp #(.BUS_SIZE_ADDR(MDATA),
              .BUS_SIZE_DATA(ADDR_LMT),
              .GRAM_MODE(2'd3)
             )arb_wr_mem 
            (
                .clk  (Clk_400),
                .we   (ab2re_WrEn),        
                .waddr(ab2re_WrTID[MDATA-1:0]),     
                .din  (ab2re_WrAddr),       
                .raddr(re2ab_WrRsp[MDATA-1:0]),     
                .dout (arbmem_wr_dout )
            );     
   
   //------------------------------------------------------------------------------------------------------------------------
   always @(posedge Clk_400)
     begin
        re2ab_RdData_q          <= re2ab_RdData;
        re2ab_RdRsp_q           <= re2ab_RdRsp;
        re2ab_WrRsp_q           <= re2ab_WrRsp;
        re2ab_RdData_qq         <= re2ab_RdData_q;
        re2ab_RdRsp_qq          <= re2ab_RdRsp_q;
        re2ab_WrRsp_qq          <= re2ab_WrRsp_q;
        if(~test_Resetb)
          begin
             re2ab_RdRspValid_q      <= 0;
             re2ab_UMsgValid_q       <= 0;
             re2ab_CfgValid_q        <= 0;
             re2ab_WrRspValid_q      <= 0;
             re2ab_RdRspValid_qq     <= 0;
             re2ab_UMsgValid_qq      <= 0;
             re2ab_CfgValid_qq       <= 0;
             re2ab_WrRspValid_qq     <= 0;
             re2ab_RdRspFormat_q     <= 0;
             re2ab_RdRspFormat_qq    <= 0;
             re2ab_RdRspCLnum_q      <= 0;
             re2ab_RdRspCLnum_qq     <= 0;
             re2ab_WrRspFormat_q     <= 0;
             re2ab_WrRspFormat_qq    <= 0;
             re2ab_WrRspCLnum_q      <= 0;
             re2ab_WrRspCLnum_qq     <= 0;
          end
        else
          begin
             re2ab_RdRspValid_q      <= re2ab_RdRspValid;
             re2ab_UMsgValid_q       <= re2ab_UMsgValid;
             re2ab_CfgValid_q        <= re2ab_CfgValid;
             re2ab_WrRspValid_q      <= re2ab_WrRspValid;
             re2ab_RdRspValid_qq     <= re2ab_RdRspValid_q;
             re2ab_UMsgValid_qq      <= re2ab_UMsgValid_q;
             re2ab_CfgValid_qq       <= re2ab_CfgValid_q;
             re2ab_WrRspValid_qq     <= re2ab_WrRspValid_q;
             re2ab_RdRspFormat_q     <= re2ab_RdRspFormat;
             re2ab_RdRspFormat_qq    <= re2ab_RdRspFormat_q;
             re2ab_RdRspCLnum_q      <= re2ab_RdRspCLnum;
             re2ab_RdRspCLnum_qq     <= re2ab_RdRspCLnum_q;
             re2ab_WrRspFormat_q     <= re2ab_WrRspFormat;
             re2ab_WrRspFormat_qq    <= re2ab_WrRspFormat_q;
             re2ab_WrRspCLnum_q      <= re2ab_WrRspCLnum;
             re2ab_WrRspCLnum_qq     <= re2ab_WrRspCLnum_q;
          end
     end
   
   always @(*)
     begin
        // OUTPUTs
        ab2re_WrAddr    = 0;
        ab2re_WrTID     = 0;
        ab2re_WrDin     = 'hx;
        ab2re_WrFence   = 0;
        ab2re_WrEn      = 0;
        ab2re_RdAddr    = 0;
        ab2re_RdTID     = 0;
        ab2re_RdEn      = 0;
        ab2re_TestCmp   = 0;
        ab2re_ErrorInfo = 'h0;
        ab2re_ErrorValid= 0;
    
        ab2re_RdLen     = 0;
        ab2re_RdSop     = 0;
        ab2re_WrLen     = 0;
        ab2re_WrSop     = 0; 
    
        // M_LPBK1
        ab2l1_WrSent    = 0;
        ab2l1_WrAlmFull = 0;
        ab2l1_RdSent    = 0;
        ab2l1_RdRspValid= 0;
        ab2l1_RdRsp     = 0;
        ab2l1_RdRspAddr = 0;
        ab2l1_RdData    = 'hx;
        ab2l1_stallRd   = 0;
        ab2l1_WrRspValid= 0;
        ab2l1_WrRsp     = 0;
        ab2l1_WrRspAddr = 0;
    
        ab2l1_RdRspFormat  = 0;
        ab2l1_RdRspCLnum   = 0; 
        ab2l1_WrRspFormat  = 0;
        ab2l1_WrRspCLnum   = 0;  

        // M_TRPUT
        ab2rw_Mode      = 0;
        ab2rw_WrSent    = 0;
        ab2rw_WrAlmFull = 0;
        ab2rw_RdSent    = 0;
        ab2rw_RdRspValid= 0;
        ab2rw_RdRsp     = 0;
        ab2rw_RdRspAddr = 0;
        ab2rw_RdData    = 'hx;
        ab2rw_WrRspValid= 0;
        ab2rw_WrRsp     = 0;
        ab2rw_WrRspAddr = 0;
    
        ab2rw_RdRspFormat  = 0;
        ab2rw_RdRspCLnum   = 0;
        ab2rw_WrRspFormat  = 0;
        ab2rw_WrRspCLnum   = 0;
            
        // M_SW1
        ab2s1_WrSent    = 0;
        ab2s1_WrAlmFull = 0;
        ab2s1_RdSent    = 0;
        ab2s1_RdRspValid= 0;
        ab2s1_CfgValid  = 0;
        ab2s1_UMsgValid = 0;
        ab2s1_RdRsp     = 0;
        ab2s1_RdRspAddr = 0;
        ab2s1_RdData    = 'hx;
        ab2s1_WrRspValid= 0;
        ab2s1_WrRsp     = 0;
        ab2s1_WrRspAddr = 0;

        // ---------------------------------------------------------------------------------------------------------------------
        //      Input to tests        
        // ---------------------------------------------------------------------------------------------------------------------
    `ifdef SIM_MODE
        if(re2ab_Mode==M_LPBK1)
          begin
              // Input
             ab2l1_WrSent       = re2ab_WrSent;
             ab2l1_WrAlmFull    = re2ab_WrAlmFull;
             ab2l1_RdSent       = re2ab_RdSent;
             ab2l1_RdRspValid   = re2ab_RdRspValid_qq;
             ab2l1_UMsgValid    = re2ab_UMsgValid_qq;
             ab2l1_CfgValid     = re2ab_CfgValid_qq;
             ab2l1_RdRsp        = re2ab_RdRsp_qq;
             ab2l1_RdRspAddr    = arbmem_rd_dout;
             ab2l1_RdData       = re2ab_RdData_qq;
             ab2l1_stallRd      = re2ab_stallRd;
             ab2l1_WrRspValid   = re2ab_WrRspValid_qq;
             ab2l1_WrRsp        = re2ab_WrRsp_qq;
             ab2l1_WrRspAddr    = arbmem_wr_dout;
       
             ab2l1_RdRspFormat  = re2ab_RdRspFormat_qq;
             ab2l1_RdRspCLnum   = re2ab_RdRspCLnum_qq; 
             ab2l1_WrRspFormat  = re2ab_WrRspFormat_qq;
             ab2l1_WrRspCLnum   = re2ab_WrRspCLnum_qq;       
             // Output
             ab2re_WrAddr       = l12ab_WrAddr;
             ab2re_WrTID        = l12ab_WrTID;
             ab2re_WrDin        = l12ab_WrDin;
             ab2re_WrFence      = 1'b0;
             ab2re_WrEn         = l12ab_WrEn;
             ab2re_RdAddr       = l12ab_RdAddr;
             ab2re_RdTID        = l12ab_RdTID;
             ab2re_RdEn         = l12ab_RdEn;
             ab2re_TestCmp      = l12ab_TestCmp;
             ab2re_ErrorInfo    = l12ab_ErrorInfo;
             ab2re_ErrorValid   = l12ab_ErrorValid;
       
             ab2re_RdLen        = l12ab_RdLen;
             ab2re_RdSop        = l12ab_RdSop;
             ab2re_WrLen        = l12ab_WrLen;
             ab2re_WrSop        = l12ab_WrSop; 
 
          end
         if(re2ab_Mode==M_TRPUT || re2ab_Mode==M_READ || re2ab_Mode==M_WRITE)
          begin
              // Input
             ab2rw_Mode         = re2ab_Mode[1:0];
             ab2rw_WrSent       = re2ab_WrSent;
             ab2rw_WrAlmFull    = re2ab_WrAlmFull;
             ab2rw_RdSent       = re2ab_RdSent;
             ab2rw_RdRspValid   = re2ab_RdRspValid_qq;
             ab2rw_UMsgValid    = re2ab_UMsgValid_qq;
             ab2rw_CfgValid     = re2ab_CfgValid_qq;
             ab2rw_RdRsp        = re2ab_RdRsp_qq;
             ab2rw_RdRspAddr    = arbmem_rd_dout;
             ab2rw_RdData       = re2ab_RdData_qq;
             ab2rw_WrRspValid   = re2ab_WrRspValid_qq;
             ab2rw_WrRsp        = re2ab_WrRsp_q;
             ab2rw_WrRspAddr    = arbmem_wr_dout;      
             
             ab2rw_RdRspFormat  = re2ab_RdRspFormat_qq;
             ab2rw_RdRspCLnum   = re2ab_RdRspCLnum_qq; 
             ab2rw_WrRspFormat  = re2ab_WrRspFormat_qq;
             ab2rw_WrRspCLnum   = re2ab_WrRspCLnum_qq;       
             // Output
             ab2re_WrAddr       = rw2ab_WrAddr;
             ab2re_WrTID        = rw2ab_WrTID;
             ab2re_WrDin        = rw2ab_WrDin;
             ab2re_WrFence      = 1'b0;
             ab2re_WrEn         = rw2ab_WrEn;
             ab2re_RdAddr       = rw2ab_RdAddr;
             ab2re_RdTID        = rw2ab_RdTID;
             ab2re_RdEn         = rw2ab_RdEn;
             ab2re_TestCmp      = rw2ab_TestCmp;
             ab2re_ErrorInfo    = rw2ab_ErrorInfo;
             ab2re_ErrorValid   = rw2ab_ErrorValid;
       
             ab2re_RdLen        = rw2ab_RdLen;
             ab2re_RdSop        = rw2ab_RdSop;
             ab2re_WrLen        = rw2ab_WrLen;
             ab2re_WrSop        = rw2ab_WrSop;
          end
        if(re2ab_Mode==M_SW1)
          begin
              // Input
             ab2s1_WrSent       = re2ab_WrSent;
             ab2s1_WrAlmFull    = re2ab_WrAlmFull;
             ab2s1_RdSent       = re2ab_RdSent;
             ab2s1_RdRspValid   = re2ab_RdRspValid_qq;
             ab2s1_UMsgValid    = re2ab_UMsgValid_qq;
             ab2s1_CfgValid     = re2ab_CfgValid_qq;
             ab2s1_RdRsp        = re2ab_RdRsp_qq;
             ab2s1_RdRspAddr    = arbmem_rd_dout;
             ab2s1_RdData       = re2ab_RdData_qq;
             ab2s1_WrRspValid   = re2ab_WrRspValid_qq;
             ab2s1_WrRsp        = re2ab_WrRsp_qq;
             ab2s1_WrRspAddr    = arbmem_wr_dout;
             // Output
             ab2re_WrAddr       = s12ab_WrAddr;
             ab2re_WrTID        = s12ab_WrTID;
             ab2re_WrDin        = s12ab_WrDin;
             ab2re_WrFence      = s12ab_WrFence;
             ab2re_WrEn         = s12ab_WrEn;
             ab2re_RdAddr       = s12ab_RdAddr;
             ab2re_RdTID        = s12ab_RdTID;
             ab2re_RdEn         = s12ab_RdEn;
             ab2re_TestCmp      = s12ab_TestCmp;
             ab2re_ErrorInfo    = s12ab_ErrorInfo;
             ab2re_ErrorValid   = s12ab_ErrorValid;
       
             ab2re_RdLen        = 0;
             ab2re_RdSop        = 1;
             ab2re_WrLen        = 0;
             ab2re_WrSop        = 1;
         end

     `else  // NOT SIM_MODE
       // PAR MODE
      `ifdef NLB400_MODE_0
              // Input
             ab2l1_WrSent       = re2ab_WrSent;
             ab2l1_WrAlmFull    = re2ab_WrAlmFull;
             ab2l1_RdSent       = re2ab_RdSent;
             ab2l1_RdRspValid   = re2ab_RdRspValid_qq;
             ab2l1_UMsgValid    = re2ab_UMsgValid_qq;
             ab2l1_CfgValid     = re2ab_CfgValid_qq;
             ab2l1_RdRsp        = re2ab_RdRsp_qq;
             ab2l1_RdRspAddr    = arbmem_rd_dout;
             ab2l1_RdData       = re2ab_RdData_qq;
             ab2l1_stallRd      = re2ab_stallRd;
             ab2l1_WrRspValid   = re2ab_WrRspValid_qq;
             ab2l1_WrRsp        = re2ab_WrRsp_qq;
             ab2l1_WrRspAddr    = arbmem_wr_dout;
       
             ab2l1_RdRspFormat  = re2ab_RdRspFormat_qq;
             ab2l1_RdRspCLnum   = re2ab_RdRspCLnum_qq; 
             ab2l1_WrRspFormat  = re2ab_WrRspFormat_qq;
             ab2l1_WrRspCLnum   = re2ab_WrRspCLnum_qq;
       
            // Output
             ab2re_WrAddr       = l12ab_WrAddr;
             ab2re_WrTID        = l12ab_WrTID;
             ab2re_WrDin        = l12ab_WrDin;
             ab2re_WrFence      = 1'b0;
             ab2re_WrEn         = l12ab_WrEn;
             ab2re_RdAddr       = l12ab_RdAddr;
             ab2re_RdTID        = l12ab_RdTID;
             ab2re_RdEn         = l12ab_RdEn;
             ab2re_TestCmp      = l12ab_TestCmp;
             ab2re_ErrorInfo    = l12ab_ErrorInfo;
             ab2re_ErrorValid   = l12ab_ErrorValid;
             
             ab2re_RdLen        = l12ab_RdLen;
             ab2re_RdSop        = l12ab_RdSop;
             ab2re_WrLen        = l12ab_WrLen;
             ab2re_WrSop        = l12ab_WrSop; 

      `elsif NLB400_MODE_3
              // Input
             ab2rw_Mode         = re2ab_Mode[1:0];
             ab2rw_WrSent       = re2ab_WrSent;
             ab2rw_WrAlmFull    = re2ab_WrAlmFull;
             ab2rw_RdSent       = re2ab_RdSent;
             ab2rw_RdRspValid   = re2ab_RdRspValid_qq;
             ab2rw_UMsgValid    = re2ab_UMsgValid_qq;
             ab2rw_CfgValid     = re2ab_CfgValid_qq;
             ab2rw_RdRsp        = re2ab_RdRsp_qq;
             ab2rw_RdRspAddr    = arbmem_rd_dout;
             ab2rw_RdData       = re2ab_RdData_qq;
             ab2rw_WrRspValid   = re2ab_WrRspValid_qq;
             ab2rw_WrRsp        = re2ab_WrRsp_qq;
             ab2rw_WrRspAddr    = arbmem_wr_dout;    

             ab2rw_RdRspFormat  = re2ab_RdRspFormat_qq;
             ab2rw_RdRspCLnum   = re2ab_RdRspCLnum_qq; 
             ab2rw_WrRspFormat  = re2ab_WrRspFormat_qq;
             ab2rw_WrRspCLnum   = re2ab_WrRspCLnum_qq;
       
             // Output
             ab2re_WrAddr       = rw2ab_WrAddr;
             ab2re_WrTID        = rw2ab_WrTID;
             ab2re_WrDin        = rw2ab_WrDin;
             ab2re_WrFence      = 1'b0;
             ab2re_WrEn         = rw2ab_WrEn;
             ab2re_RdAddr       = rw2ab_RdAddr;
             ab2re_RdTID        = rw2ab_RdTID;
             ab2re_RdEn         = rw2ab_RdEn;
             ab2re_TestCmp      = rw2ab_TestCmp;
             ab2re_ErrorInfo    = rw2ab_ErrorInfo;
             ab2re_ErrorValid   = rw2ab_ErrorValid;
       
             ab2re_RdLen        = rw2ab_RdLen;
             ab2re_RdSop        = rw2ab_RdSop;
             ab2re_WrLen        = rw2ab_WrLen;
             ab2re_WrSop        = rw2ab_WrSop;

      `elsif NLB400_MODE_7
              // Input
             ab2s1_WrSent       = re2ab_WrSent;
             ab2s1_WrAlmFull    = re2ab_WrAlmFull;
             ab2s1_RdSent       = re2ab_RdSent;
             ab2s1_RdRspValid   = re2ab_RdRspValid_qq;
             ab2s1_UMsgValid    = re2ab_UMsgValid_qq;
             ab2s1_CfgValid     = re2ab_CfgValid_qq;
             ab2s1_RdRsp        = re2ab_RdRsp_qq;
             ab2s1_RdRspAddr    = arbmem_rd_dout;
             ab2s1_RdData       = re2ab_RdData_qq;
             ab2s1_WrRspValid   = re2ab_WrRspValid_qq;
             ab2s1_WrRsp        = re2ab_WrRsp_qq;
             ab2s1_WrRspAddr    = arbmem_wr_dout;
             // Output
             ab2re_WrAddr       = s12ab_WrAddr;
             ab2re_WrTID        = s12ab_WrTID;
             ab2re_WrDin        = s12ab_WrDin;
             ab2re_WrFence      = s12ab_WrFence;
             ab2re_WrEn         = s12ab_WrEn;
             ab2re_RdAddr       = s12ab_RdAddr;
             ab2re_RdTID        = s12ab_RdTID;
             ab2re_RdEn         = s12ab_RdEn;
             ab2re_TestCmp      = s12ab_TestCmp;
             ab2re_ErrorInfo    = s12ab_ErrorInfo;
             ab2re_ErrorValid   = s12ab_ErrorValid;
       
             ab2re_RdLen        = 0;
             ab2re_RdSop        = 1;
             ab2re_WrLen        = 0;
             ab2re_WrSop        = 1;
      `else
          *** In PAR Mode, Select a valid NBL400_MODE: 0, 3, 7
      `endif
`endif
     end

    test_lpbk1 #(.PEND_THRESH(PEND_THRESH),
                 .ADDR_LMT   (ADDR_LMT),
                 .MDATA      (MDATA)
                 )
    test_lpbk1(
           Clk_400               ,        // in    std_logic;  -- Core clock
    
           l12ab_WrAddr,                   // [ADDR_LMT-1:0]        app_cnt:           write address
           l12ab_WrTID,                    // [ADDR_LMT-1:0]        app_cnt:           meta data
           l12ab_WrDin,                    // [511:0]               app_cnt:           Cache line data
           l12ab_WrEn,                     //                       app_cnt:           write enable
           ab2l1_WrSent,                   //                       app_cnt:           write issued
           ab2l1_WrAlmFull,                //                       app_cnt:           write fifo almost full
           
           l12ab_RdAddr,                   // [ADDR_LMT-1:0]        app_cnt:           Reads may yield to writes
           l12ab_RdTID,                    // [15:0]                app_cnt:           meta data
           l12ab_RdEn,                     //                       app_cnt:           read enable
           ab2l1_RdSent,                   //                       app_cnt:           read issued
    
           ab2l1_RdRspValid,               //                       app_cnt:           read response valid
           ab2l1_RdRsp,                    // [15:0]                app_cnt:           read response header
           ab2l1_RdRspAddr,                // [ADDR_LMT-1:0]        app_cnt:           read response address
           ab2l1_RdData,                   // [511:0]               app_cnt:           read data
           ab2l1_stallRd,                  //                       app_cnt:           stall read requests FOR LPBK1
    
           ab2l1_WrRspValid,               //                       app_cnt:           write response valid
           ab2l1_WrRsp,                    // [15:0]                app_cnt:           write response header
           ab2l1_WrRspAddr,                // [ADDR_LMT-1:0]        app_cnt:           write response address
           re2xy_go,                       //                       requestor:         start the test
           re2xy_NumLines,                 // [31:0]                requestor:         number of cache lines
           re2xy_Cont,                     //                       requestor:         continuous mode
    
           l12ab_TestCmp,                  //                       arbiter:           Test completion flag
           l12ab_ErrorInfo,                // [255:0]               arbiter:           error information
           l12ab_ErrorValid,               //                       arbiter:           test has detected an error
           test_Resetb,                    //                       requestor:         rest the app
       
           l12ab_RdLen,
           l12ab_RdSop,
           l12ab_WrLen,
           l12ab_WrSop,
          
           ab2l1_RdRspFormat,
           ab2l1_RdRspCLnum,
           ab2l1_WrRspFormat,
           ab2l1_WrRspCLnum,
           re2xy_multiCL_len
    );

    test_rdwr #(.PEND_THRESH(PEND_THRESH),
                .ADDR_LMT   (ADDR_LMT),
                .MDATA      (MDATA)
                )
    
    test_rdwr(
    
    //      ---------------------------global signals-------------------------------------------------
           Clk_400               ,        // in    std_logic;  -- Core clock
           ab2rw_Mode           ,        //                       arb:               1- reads only test, 0- writes only test
    
           rw2ab_WrAddr,                   // [ADDR_LMT-1:0]        arb:               write address
           rw2ab_WrTID,                    // [ADDR_LMT-1:0]        arb:               meta data
           rw2ab_WrDin,                    // [511:0]               arb:               Cache line data
           rw2ab_WrEn,                     //                       arb:               write enable
           ab2rw_WrSent,                   //                       arb:               write issued
           ab2rw_WrAlmFull,                //                       arb:               write fifo almost full
           re2xy_wrdin_msb,               //                       requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns
           
           rw2ab_RdAddr,                   // [ADDR_LMT-1:0]        arb:               Reads may yield to writes
           rw2ab_RdTID,                    // [15:0]                arb:               meta data
           rw2ab_RdEn,                     //                       arb:               read enable
           ab2rw_RdSent,                   //                       arb:               read issued
    
           ab2rw_RdRspValid,               //                       arb:               read response valid
           ab2rw_RdRsp,                    // [15:0]                arb:               read response header
           ab2rw_RdRspAddr,                // [ADDR_LMT-1:0]        arb:               read response address
           ab2rw_RdData,                   // [511:0]               arb:               read data
    
           ab2rw_WrRspValid,               //                       arb:               write response valid
           ab2rw_WrRsp,                    // [15:0]                arb:               write response header
           ab2rw_WrRspAddr,                // [ADDR_LMT-1:0]        arb:               write response address
           re2xy_go,                       //                       requestor:         start the test
           re2xy_NumLines,                 // [31:0]                requestor:         number of cache lines
           re2xy_Cont,                     //                       requestor:         continuous mode
           re2xy_stride,                  // [31:0]                 requestor:         stride value
           
    
           rw2ab_TestCmp,                  //                       arb:               Test completion flag
           rw2ab_ErrorInfo,                // [255:0]               arb:               error information
           rw2ab_ErrorValid,               //                       arb:               test has detected an error
           test_Resetb,                    //                       requestor:         rest the app
       
           rw2ab_RdLen,
           rw2ab_RdSop,
           rw2ab_WrLen,
           rw2ab_WrSop,
          
           ab2rw_RdRspFormat,
           ab2rw_RdRspCLnum,
           ab2rw_WrRspFormat,
           ab2rw_WrRspCLnum,
           re2xy_multiCL_len
    );

    test_sw1  #(.PEND_THRESH(PEND_THRESH),
                .ADDR_LMT   (ADDR_LMT),
                .MDATA      (MDATA)
                )
    
    test_sw1 (
    
    //      ---------------------------global signals-------------------------------------------------
           Clk_400               ,        // in    std_logic;  -- Core clock
    
           s12ab_WrAddr,                   // [ADDR_LMT-1:0]        arb:               write address
           s12ab_WrTID,                    // [ADDR_LMT-1:0]        arb:               meta data
           s12ab_WrDin,                    // [511:0]               arb:               Cache line data
           s12ab_WrFence,                  //                       arb:               write fence 
           s12ab_WrEn,                     //                       arb:               write enable
           ab2s1_WrSent,                   //                       arb:               write issued
           ab2s1_WrAlmFull,                //                       arb:               write fifo almost full
           
           s12ab_RdAddr,                   // [ADDR_LMT-1:0]        arb:               Reads may yield to writes
           s12ab_RdTID,                    // [15:0]                arb:               meta data
           s12ab_RdEn,                     //                       arb:               read enable
           ab2s1_RdSent,                   //                       arb:               read issued
    
           ab2s1_RdRspValid,               //                       arb:               read response valid
           ab2s1_UMsgValid,                //                       arb:               UMsg valid
           ab2s1_CfgValid,                 //                       arb:               Cfg valid
           ab2s1_RdRsp,                    // [15:0]                arb:               read response header
           ab2s1_RdRspAddr,                // [ADDR_LMT-1:0]        arb:               read response address
           ab2s1_RdData,                   // [511:0]               arb:               read data
    
           ab2s1_WrRspValid,               //                       arb:               write response valid
           ab2s1_WrRsp,                    // [15:0]                arb:               write response header
           ab2s1_WrRspAddr,                // [ADDR_LMT-1:0]        arb:               write response address
           re2xy_go,                       //                       requestor:         start the test
           re2xy_NumLines,                 // [31:0]                requestor:         number of cache lines
           re2xy_Cont,                     //                       requestor:         continuous mode
           re2xy_test_cfg,                 // [7:0]                 requestor:         8-bit test cfg register.
    
           s12ab_TestCmp,                  //                       arb:               Test completion flag
           s12ab_ErrorInfo,                // [255:0]               arb:               error information
           s12ab_ErrorValid,               //                       arb:               test has detected an error
           cr2s1_csr_write,
           test_Resetb                     //                       requestor:         rest the app
    );  
endmodule
