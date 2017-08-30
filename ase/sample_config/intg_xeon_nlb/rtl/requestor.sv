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
// Module Name:         requestor.v
// Project:             NLB AFU v1.1
//                      Compliant with CCI v2.1
// Description:         accepts requests from arbiter and formats it per cci
//                      spec. It also implements the flow control.
// ***************************************************************************
//
// The requestor accepts the address index from the arbiter, appends that to the source/destination base address and 
// sends out the request to the CCI module. It arbitrates between the read and the write requests, peforms the flow control,
// implements all the CSRs for source address, destination address, status address, wrthru enable, start and stop the test.
//
//
//
`default_nettype none
import ccip_if_pkg::*;
module requestor #(parameter PEND_THRESH=1, ADDR_LMT=20, TXHDR_WIDTH=61, RXHDR_WIDTH=18, DATA_WIDTH=512)
(

    //      ---------------------------global signals-------------------------------------------------
    Clk_400,        // in    std_logic;  -- Core clock
    SoftReset,      // in    std_logic;  -- Use SPARINGLY only for control. ACTIVE HIGH
    //      ---------------------------CCI IF signals between CCI and requestor  ---------------------
    af2cp_sTxPort,
    cp2af_sRxPort,

    cr2re_src_address,
    cr2re_dst_address,
    cr2re_num_lines,
    cr2re_inact_thresh,
    cr2re_interrupt0,
    cr2re_cfg,
    cr2re_ctl,
    cr2re_stride,
    cr2re_dsm_base,
    cr2re_dsm_base_valid,

    ab2re_WrAddr,            // [ADDR_LMT-1:0]      arbiter:        Writes are guaranteed to be accepted
    ab2re_WrTID,             // [15:0]              arbiter:        meta data
    ab2re_WrDin,             // [511:0]             arbiter:        Cache line data
    ab2re_WrFence,           //                     arbiter:        write fence.
    ab2re_WrEn,              //                     arbiter:        write enable
    re2ab_WrSent,            //                     arbiter:        can accept writes. Qualify with write enable
    re2ab_WrAlmFull,         //                     arbiter:        write fifo almost full

    ab2re_RdAddr,            // [ADDR_LMT-1:0]      arbiter:        Reads may yield to writes
    ab2re_RdTID,             // [15:0]              arbiter:        meta data
    ab2re_RdEn,              //                     arbiter:        read enable
    re2ab_RdSent,            //                     arbiter:        read issued

    re2ab_RdRspValid,        //                     arbiter:        read response valid
    re2ab_UMsgValid,         //                     arbiter:        UMsg valid
    re2ab_CfgValid,          //                     arbiter:        Cfg Valid
    re2ab_RdRsp,             // [ADDR_LMT-1:0]      arbiter:        read response header
    re2ab_RdData,            // [511:0]             arbiter:        read data
    re2ab_stallRd,           //                     arbiter:        stall read requests FOR LPBK1

    re2ab_WrRspValid,        //                     arbiter:        write response valid
    re2ab_WrRsp,             // [ADDR_LMT-1:0]      arbiter:        write response header
    re2xy_go,                //                     requestor:      start the test
    re2xy_NumLines,          // [31:0]              requestor:      number of cache lines
    re2xy_stride,             // [31:0]              requestor:      stride value
    re2xy_Cont,              //                     requestor:      continuous mode
    re2xy_wrdin_msb,         //                     requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns
    re2xy_src_addr,          // [31:0]              requestor:      src address
    re2xy_dst_addr,          // [31:0]              requestor:      destination address
    re2xy_test_cfg,          // [7:0]               requestor:      8-bit test cfg register.
    re2ab_Mode,              // [2:0]               requestor:      test mode

    ab2re_TestCmp,           //                     arbiter:        Test completion flag
    ab2re_ErrorInfo,         // [255:0]             arbiter:        error information
    ab2re_ErrorValid,        //                     arbiter:        test has detected an error
    test_Reset_n,             //                     requestor:      rest the app
    re2cr_wrlock_n,          //                     requestor:      when low, block csr writes
  
    ab2re_RdLen,
    ab2re_RdSop,
    ab2re_WrLen,
    ab2re_WrSop,
     
    re2ab_RdRspFormat,
    re2ab_RdRspCLnum,
    re2ab_WrRspFormat,
    re2ab_WrRspCLnum,
    re2xy_multiCL_len,

    re2cr_num_reads,
    re2cr_num_writes,
    re2cr_num_Rdpend,
    re2cr_num_Wrpend,
    re2cr_error
);
    //--------------------------------------------------------------------------------------------------------------
    input  logic            Clk_400;                //                      ccip_intf:        Clk_400
    input  logic            SoftReset;              //                      ccip_intf:        system SoftReset
    
    output t_if_ccip_Tx     af2cp_sTxPort;
    input  t_if_ccip_Rx     cp2af_sRxPort;

    input  t_ccip_mmioData  cr2re_src_address;
    input  t_ccip_mmioData  cr2re_dst_address;
    input  logic [31:0]     cr2re_num_lines;
    input  logic [31:0]     cr2re_inact_thresh;
    input  logic [31:0]     cr2re_interrupt0;
    input  t_ccip_mmioData  cr2re_cfg;
    input  logic [31:0]     cr2re_ctl;
    input  logic [31:0]     cr2re_stride;
    input  t_ccip_mmioData  cr2re_dsm_base;
    input  logic            cr2re_dsm_base_valid;
    
    input  logic [ADDR_LMT-1:0]   ab2re_WrAddr;      // [ADDR_LMT-1:0]        arbiter:       Writes are guaranteed to be accepted
    input  t_ccip_mdata     ab2re_WrTID;            // [15:0]                arbiter:       meta data
    input  t_ccip_clData    ab2re_WrDin;            // [511:0]               arbiter:       Cache line data
    input  logic            ab2re_WrFence;          //                       arbiter:       write fence 
    input  logic            ab2re_WrEn;             //                       arbiter:       write enable
    output logic            re2ab_WrSent;           //                       arbiter:       write issued
    output logic            re2ab_WrAlmFull;        //                       arbiter:       write fifo almost full
    
    input  logic [ADDR_LMT-1:0]   ab2re_RdAddr;           // [ADDR_LMT-1:0]        arbiter:       Reads may yield to writes
    input  t_ccip_mdata     ab2re_RdTID;            // [15:0]                arbiter:       meta data
    input  logic            ab2re_RdEn;             //                       arbiter:       read enable
    output logic            re2ab_RdSent;           //                       arbiter:       read issued
    
    output logic            re2ab_RdRspValid;       //                       arbiter:       read response valid
    output logic            re2ab_UMsgValid;        //                       arbiter:       UMsg valid
    output logic            re2ab_CfgValid;         //                       arbiter:       Cfg valid
    output t_ccip_mdata     re2ab_RdRsp;            // [15:0]                arbiter:       read response header
    output t_ccip_clData    re2ab_RdData;           // [511:0]               arbiter:       read data
    output logic            re2ab_stallRd;          //                       arbiter:       stall read requests FOR LPBK1
    
    output logic            re2ab_WrRspValid;       //                       arbiter:       write response valid
    output t_ccip_mdata     re2ab_WrRsp;            // [15:0]                arbiter:       write response header
    
    (* maxfan=1 *)output    logic re2xy_go;         //                       requestor:     start of frame recvd
    output logic [31:0]     re2xy_NumLines;         // [31:0]                requestor:     number of cache lines
    output logic [31:0]     re2xy_stride;            // [31:0]              requestor:      stride value
    output logic            re2xy_Cont;             //                       requestor:     continuous mode
    output logic [31:0]     re2xy_src_addr;         // [31:0]                requestor:     src address
    output logic [31:0]     re2xy_dst_addr;         // [31:0]                requestor:     destination address
    output logic [7:0]      re2xy_test_cfg;         // [7:0]                 requestor:     8-bit test cfg register.
    output logic [2:0]      re2ab_Mode;             // [2:0]                 requestor:     test mode
    input  logic            ab2re_TestCmp;          //                       arbiter:       Test completion flag
    input  logic [255:0]    ab2re_ErrorInfo;        // [255:0]               arbiter:       error information
    input  logic            ab2re_ErrorValid;       //                       arbiter:       test has detected an error
    output logic           re2xy_wrdin_msb;        //                       requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns     
    output logic            test_Reset_n;
    output logic            re2cr_wrlock_n;
  
    input logic [1:0]       ab2re_RdLen;
    input logic             ab2re_RdSop;
    input logic [1:0]       ab2re_WrLen;
    input logic             ab2re_WrSop;
    output logic            re2ab_RdRspFormat;
    output logic [1:0]      re2ab_RdRspCLnum;
    output logic            re2ab_WrRspFormat;
    output logic [1:0]      re2ab_WrRspCLnum;
    output logic [1:0]      re2xy_multiCL_len;

    output  logic [31:0]    re2cr_num_Rdpend;
    output  logic [31:0]    re2cr_num_Wrpend;
    output  logic [31:0]    re2cr_num_reads;
    output  logic [31:0]    re2cr_num_writes;
    output  logic [31:0]    re2cr_error;
    
    //----------------------------------------------------------------------------------------------------------------------
    //---------------------------------------------------------
    // Default Values ****** May be MODIFIED ******* 
    //---------------------------------------------------------
    localparam      DEF_SRC_ADDR         = 32'h0400_0000;           // Read data starting from here. Cache aligned Address
    localparam      DEF_DST_ADDR         = 32'h0500_0000;           // Copy data to here. Cache aligned Address
    localparam      DEF_DSM_BASE         = 32'h04ff_ffff;           // default status address
    
   
    //----------------------------------------------------------------------------------
    // Device Status Memory (DSM) Address Map ***** DO NOT MODIFY *****
    // This is a shared memory region where AFU writes and SW reads from. It is used for sharing status.
    // Physical address = value at CSR_AFU_DSM_BASE + Byte offset
    //----------------------------------------------------------------------------------
    //                                     Byte Offset                 Attribute    Width   Comments
    localparam      DSM_STATUS           = 32'h40;                  // RO           512b    test status and error info
    
    //----------------------------------------------------------------------------------------------------------------------
    
    reg  [31:0]             ErrorVector;
    reg  [31:0]             Num_Reads;                              // Number of reads performed
    reg  [31:0]             Num_Writes;                             // Number of writes performed
    reg  [19:0]             Num_ticks_low, Num_ticks_high;
    reg  [31:0]             Num_C0stall;                            // Number of clocks for which Channel0 was throttled
    reg  [31:0]             Num_C1stall;                            // Number of clocks for which channel1 was throttled
    reg  signed [31:0]      Num_RdCredits;                          // For LPBK1: number of read credits
    reg                     RdHdr_valid;
    reg                     WrHdr_valid_T1, WrHdr_valid_T2, WrHdr_valid_T3 , WrHdr_valid_T4;
    reg  [31:0]             wrfifo_addr;
    t_ccip_clData           wrfifo_data;
    reg                     txFifo_RdAck;
    wire                    txFifo_Dout_v;
    reg                     tx_c0_req_valid, tx_c1_req_valid;
    reg                     rx_c0_resp_valid, rx_c1_resp_valid;
    
    t_if_ccip_Rx            cp2af_sRxPort_T1;

    reg                     re2ab_CfgValid_d;
    reg                     status_write;
    reg                     interrupt_sent;
    reg                     send_interrupt;
    
    reg   [31:0]            inact_cnt;
    reg                     inact_timeout;
    reg   [5:0]             delay_lfsr;
    reg   [1:0]             delay_lfsr1,rnd_ch_sel;
    reg   [31:0]            cr_inact_thresh;
    reg                     penalty_start_f;
    reg   [7:0]             penalty_start;
    reg   [7:0]             penalty_end;
(* dont_merge, maxfan=256 *) reg dsm_status_wren_a;
(* dont_merge, maxfan=256 *) reg dsm_status_wren_b;
(* dont_merge, maxfan=256 *) reg dsm_status_wren_c;
    t_ccip_c0_req           rdreq_type;
    t_ccip_c0_req           rnd_rdreq_type;
    reg                     rnd_rdreq_sel;
    
    integer                 i;
    t_ccip_mmioData         cr_dsm_base;                            // a00h, a04h - DSM base address
    t_ccip_mmioData         cr_src_address;                         // a20h - source buffer address
    t_ccip_mmioData         cr_dst_address;                         // a24h - destn buffer address
    reg   [31:0]            cr_num_lines;                           // a28h - Number of cache lines
    reg   [31:0]            cr_stride;                              //stride value
    reg   [1:0]             cr_multiCL_len;   
    reg   [31:0]            cr_ctl = 0;                             // a2ch - control register to start and stop the test
    reg                     cr_wrlineI_en;                           // a34h - [0]    : test configuration- wrlineI_en
    reg                     cr_wrpushI_en;                           // a34h - [16]    : test configuration- wrlineI_en
    reg                     cr_cont;                                // a34h - [1]    : repeats the test sequence, NO end condition
    reg                     cr_wrdin_msb;                           // 
    reg   [2:0]             cr_mode;                                // a34h - [4:2]  : selects test mode
    reg                     cr_delay_en;                            // a34h - [8]    : use start delay
    reg   [1:0]             cr_rdsel, cr_rdsel_q;                   // a34h - [10:9] : read request type
    reg   [7:0]             cr_test_cfg;                            // a34h - [27:0] : configuration within a selected test mode
    reg   [31:0]            cr_interrupt0;                          // a3ch - SW allocates apic id & interrupt vector
    reg                     cr_interrupt_testmode;
    reg                     cr_interrupt_on_error;
    reg   [1:0]             rnd_ch_type,rnd_ch_type_q;          
    reg   [2:0]             rd_ch_type,cr_rd_chsel,cr_rd_chsel_q;
    reg   [2:0]             wr_ch_type,cr_wr_chsel,cr_wr_chsel_q;
    reg   [41:0]            ds_stat_address;                        // 040h - test status is written to this address
        
(* maxfan=512 *) wire       txFifo_Full;
    wire                    txFifo_AlmFull;
    wire                    txFifo_WrEn     = (ab2re_WrEn| ab2re_WrFence) && ~txFifo_Full;
    wire [15:0]             txFifo_WrTID;
    reg [ADDR_LMT-1:0]     txFifo_WrAddr,txFifo_WrAddr_q;
    wire                    txFifo_WrFence;
    wire                    txFifo_WrSop;
    wire [1:0]              txFifo_WrLen;
    
    logic [41:0]            RdAddr;      
    logic                   RdHdr_valid_q;
    logic [1:0]             ab2re_RdLen_q;
    logic                   ab2re_RdEn_q;
    logic [15:0]            ab2re_RdTID_q;
    logic [41:0]            WrAddr;      
        
    t_ccip_c1_req           wrreq_type;  
    t_ccip_clData           txFifo_WrDin;
    t_ccip_clData           WrData_dsm;

    reg                     test_go;
    reg                     sop;
  
    wire                    rnd_delay        = ~cr_delay_en || (delay_lfsr[0] || delay_lfsr[2] || delay_lfsr[3]);
    wire                    tx_errorValid    = ErrorVector!=0;
    reg    [14:0]           dsm_number=0;
    
    logic [15:0]            txFifo_WrTID_q,txFifo_WrTID_qq;
    logic                   txFifo_WrFence_q, txFifo_WrFence_qq;
    logic                   txFifo_WrSop_q, txFifo_WrSop_qq;
    logic [1:0]             txFifo_WrLen_q, txFifo_WrLen_qq;
    logic                   txFifo_cxEn_q;
    logic [2:0]             txFifo_cxQword_q;
    t_ccip_clData  txFifo_WrDin_q , txFifo_WrDin_qq;
  
    logic                   test_stop;
    logic                   WrFence_sent;
    logic                   read_only_test;
    logic                   test_cmplt;
    logic [1:0]             tx_rd_req_len;
    logic                   rx_wr_resp_fmt;
    logic [1:0]             rx_wr_resp_cl_num;
   
    logic                   tx_c0_req_valid_q; 
    logic                   rx_c0_resp_valid_q;
    logic                   tx_c1_req_valid_q;
    logic                   rx_c1_resp_valid_q;
    
    (* noprune *) logic [2:0]   num_rd_sent;
    (* maxfan=1 *) logic [2:0]  num_wr_recvd;
    (* noprune *) logic [11:0]  Num_WrPend;
    (* noprune *) logic [11:0]  Num_RdPend;
        
    // NLB supports 64MB data transfers   
    // RdAddr computation takes one cycle :- Delay Rd valid generation from req to upstream by 1 clk
    // WrAddr computation takes two cycle :- Delay Wr valid popped from FIFO by 1 cycle before fwd'ing to upstream
    always @(posedge Clk_400)
    begin
      RdAddr               <= (cr_src_address[41:0] + ab2re_RdAddr[19:0]);
      ab2re_RdLen_q        <= ab2re_RdLen;
      ab2re_RdTID_q        <= ab2re_RdTID;
      ab2re_RdEn_q         <= ab2re_RdEn;
      RdHdr_valid_q        <= RdHdr_valid;

      txFifo_WrAddr_q      <= txFifo_WrAddr;
      WrAddr               <= {(cr_dst_address[41:0] + txFifo_WrAddr_q[19:0])};
      txFifo_WrLen_q       <= txFifo_WrLen;
      txFifo_WrSop_q       <= txFifo_WrSop;
      txFifo_WrFence_q     <= txFifo_WrFence;
      txFifo_WrDin_q       <= txFifo_WrDin; 
      txFifo_WrTID_q       <= txFifo_WrTID;
      txFifo_WrLen_qq      <= txFifo_WrLen_q;
      txFifo_WrSop_qq      <= txFifo_WrSop_q;
      txFifo_WrFence_qq    <= txFifo_WrFence_q;
      txFifo_WrDin_qq      <= txFifo_WrDin_q; 
      txFifo_WrTID_qq      <= txFifo_WrTID_q;  
    end
    
    always @(posedge Clk_400)
    begin
      re2cr_wrlock_n       <= cr_ctl[0] & ~cr_ctl[1];
      test_Reset_n         <= cr_ctl[0];                // Clears all the states. Either is one then test is out of Reset.
      test_go              <= cr_ctl[1];                // When 0, it allows reconfiguration of test parameters.
      re2ab_Mode           <= cr_mode;
      re2xy_test_cfg       <= cr_test_cfg;
      re2xy_NumLines       <= cr_num_lines;
      re2xy_stride         <= cr_stride;
      re2xy_multiCL_len    <= cr_multiCL_len;
      re2xy_Cont           <= cr_cont;
      re2xy_wrdin_msb      <= cr_wrdin_msb;
      re2xy_src_addr       <= cr_src_address[31:0];
      re2xy_dst_addr       <= cr_dst_address[31:0];
    end

    always_comb 
    begin
      re2ab_RdRspFormat=0;
      re2ab_WrAlmFull  = txFifo_AlmFull;
      re2ab_WrSent     = !txFifo_Full;
      re2ab_RdRspValid = cp2af_sRxPort_T1.c0.rspValid && (cp2af_sRxPort_T1.c0.hdr.resp_type==eRSP_RDLINE);
      re2ab_UMsgValid  = cp2af_sRxPort_T1.c0.rspValid && cp2af_sRxPort_T1.c0.hdr.resp_type==eRSP_UMSG;
      re2ab_RdRsp      = cp2af_sRxPort_T1.c0.hdr.mdata[15:0];
      re2ab_RdRspCLnum = cp2af_sRxPort_T1.c0.hdr.cl_num[1:0]; 
      re2ab_RdData     = cp2af_sRxPort_T1.c0.data;
      re2ab_WrRspValid = cp2af_sRxPort_T1.c1.rspValid && cp2af_sRxPort_T1.c1.hdr.resp_type==eRSP_WRLINE;
      re2ab_WrRsp      = cp2af_sRxPort_T1.c1.hdr.mdata[15:0];
      re2ab_WrRspFormat= cp2af_sRxPort_T1.c1.hdr.format;
      re2ab_WrRspCLnum = cp2af_sRxPort_T1.c1.hdr.cl_num[1:0];
      re2ab_CfgValid   = re2ab_CfgValid_d;
      sop = txFifo_WrFence_qq ? 0 : txFifo_WrSop_qq;
      
    end
     
    always @(*)
    begin
        cr_ctl                = cr2re_ctl;
        cr_stride             = cr2re_stride;
        cr_dsm_base           = cr2re_dsm_base;
        cr_src_address        = cr2re_src_address;
        cr_dst_address        = cr2re_dst_address;
        cr_num_lines          = cr2re_num_lines;
        cr_inact_thresh       = cr2re_inact_thresh;
        cr_interrupt0         = cr2re_interrupt0;
        
        cr_wrlineI_en          = cr2re_cfg[0];
        cr_cont               = cr2re_cfg[1];
        cr_mode               = cr2re_cfg[4:2];
        cr_multiCL_len        = cr2re_cfg[6:5];          
        cr_delay_en           = 1'b0;
        cr_rdsel              = cr2re_cfg[10:9];
        cr_test_cfg           = cr2re_cfg[27:20];
        cr_interrupt_on_error = cr2re_cfg[28];
        cr_interrupt_testmode = cr2re_cfg[29];
        cr_rd_chsel              = cr2re_cfg[14:12]; 
        cr_wrdin_msb             = cr2re_cfg[15];
       cr_wrpushI_en         = cr2re_cfg[16];
       cr_wr_chsel              = cr2re_cfg[19:17];
        

    end

    always @(posedge Clk_400)
    begin    
        re2cr_num_Rdpend       <= 0;
        re2cr_num_Wrpend       <= 0;

        // So, Max pending write transactions that requestor needs to track = 256
        // So, there could be maximum of 256 * 4 = 1024 reqs that are tracked as Num_Wr/Rd_pend
        // Num_Wr/Rd_Pend[11] indicates an underflow -- recorded as unexpected WrRsp Err
        // Num_Wr/Rd_Pend[10:9] == 2'b11 indicates Test has to be stalled to prevent an overflow 
        
        re2cr_num_Rdpend[11:0] <= Num_RdPend[11:0];   
        re2cr_num_Wrpend[11:0] <= Num_WrPend[11:0];
        re2cr_num_reads        <= Num_Reads;
        re2cr_num_writes       <= Num_Writes;
        re2cr_error      <= ErrorVector;
        ds_stat_address  <= dsm_offset2addr(DSM_STATUS,cr_dsm_base);
        cr_rdsel_q       <= cr_rdsel;
        cr_rd_chsel_q       <= cr_rd_chsel;
        cr_wr_chsel_q       <= cr_wr_chsel;
        delay_lfsr <= {delay_lfsr[4:0], (delay_lfsr[5] ^ delay_lfsr[4]) };
                delay_lfsr1 <= {delay_lfsr1[0], (delay_lfsr1[0] ^ delay_lfsr1[1]) };
                rnd_ch_sel  <= delay_lfsr1;
                case(rnd_ch_sel)
                   2'h1:   rnd_ch_type <= eVC_VL0;
                   2'h2:   rnd_ch_type <= eVC_VH0;
                   2'h3:   rnd_ch_type <= eVC_VH1;
                   default: rnd_ch_type <=eVC_VL0;
                endcase
                
                case(cr_rd_chsel_q)
             3'h0:   rd_ch_type <= eVC_VA;
             3'h1:   rd_ch_type <= eVC_VL0;
             3'h2:   rd_ch_type <= eVC_VH0;
             3'h3:   rd_ch_type <= eVC_VH1;
             3'h4:   rd_ch_type <= rnd_ch_type;
             default: rd_ch_type <= eVC_VA;
              endcase 
              
                  case(cr_wr_chsel_q)
             3'h0:   wr_ch_type <= eVC_VA;
             3'h1:   wr_ch_type <= eVC_VL0;
             3'h2:   wr_ch_type <= eVC_VH0;
             3'h3:   wr_ch_type <= eVC_VH1;
             3'h4:   wr_ch_type <= rnd_ch_type;
             default: wr_ch_type <= eVC_VA;
              endcase 
     
        case(cr_rdsel_q)
            2'h0:   rdreq_type <= eREQ_RDLINE_S;
            2'h1:   rdreq_type <= eREQ_RDLINE_I;
            2'h2:   rdreq_type <= eREQ_RDLINE_I;
            2'h3:   rdreq_type <= rnd_rdreq_type;
        endcase
        rnd_rdreq_sel  <= 0;
        if(rnd_rdreq_sel)
            rnd_rdreq_type <= eREQ_RDLINE_I;
        else
            rnd_rdreq_type <= eREQ_RDLINE_S;

        if(test_go )                                             
            re2xy_go    <= 1'b1;
        if(status_write)
            re2xy_go    <= 1'b0;
        
        send_interrupt <= status_write && ((cr_interrupt_on_error & tx_errorValid) | cr_interrupt_testmode);
        dsm_status_wren_a<= ab2re_TestCmp | test_stop;              // Update Status upon test completion
        dsm_status_wren_b<= ab2re_TestCmp | test_stop;              // Update Status upon test completion
        dsm_status_wren_c<= ab2re_TestCmp | test_stop;              // Update Status upon test completion

        // Wait for multi CL request to complete 
        // If Error detected or SW forced test termination
        // Make sure that multiCL request is completed before sending out DSM Write
        
        if (re2ab_Mode[2:0] == 3'b001)
        read_only_test   <= 1;
        
        if (cr_ctl[2] | tx_errorValid)
        test_cmplt       <= 1;        
        
        if (test_stop == 0)
        test_stop        <= test_cmplt & (read_only_test | (!(|txFifo_WrLen_qq) & WrHdr_valid_T4));

        WrData_dsm <={ ab2re_ErrorInfo,                             // [511:256] upper half cache line
                       24'h00_0000,penalty_end,                     // [255:224] test end overhead in # clks
                       24'h00_0000,penalty_start,                   // [223:192] test start overhead in # clks
                       Num_Writes,                                  // [191:160] Total number of Writes sent / Total Num CX sent
                       Num_Reads,                                   // [159:128] Total number of Reads sent
                       24'h00_0000,Num_ticks_high, Num_ticks_low,   // [127:64]  number of clks
                       ErrorVector,                                 // [63:32]   errors detected            
                       16'h0000, dsm_number,                        // [15:1]    unique id for each dsm status write
                       1'h1                                         // [0]       test completion flag
                 };

        
        //Tx Path
        //--------------------------------------------------------------------------
        af2cp_sTxPort.c1.hdr        <= 0;
        af2cp_sTxPort.c1.valid      <= 0;
        af2cp_sTxPort.c0.hdr        <= 0;
        af2cp_sTxPort.c0.valid      <= 0;
	        		
        af2cp_sTxPort.c1.data[511:256] <= dsm_status_wren_a ? WrData_dsm[511:256] : txFifo_WrDin_qq[511:256]; 
        af2cp_sTxPort.c1.data[255:0]   <= dsm_status_wren_b ? WrData_dsm[255:0]   : txFifo_WrDin_qq[255:0]; 
    
            // Channel 1
            if ( send_interrupt
                 & !interrupt_sent
                 & !cp2af_sRxPort_T1.c1TxAlmFull
               )
            begin
                interrupt_sent                     <= 1'b1;
                af2cp_sTxPort.c1.hdr.vc_sel        <= t_ccip_vc'(wr_ch_type);
                af2cp_sTxPort.c1.hdr.req_type      <= eREQ_INTR;
                af2cp_sTxPort.c1.hdr.address[31:0] <= cr_interrupt0;
                af2cp_sTxPort.c1.hdr.mdata[15:0]   <= 16'hfffc;
                af2cp_sTxPort.c1.valid             <= 1'b1;
            end

            else if (re2xy_go & rnd_delay )
            begin
                  if( dsm_status_wren_c                                          // Write Fence
                     & !cp2af_sRxPort_T1.c1TxAlmFull
                     & !WrFence_sent
                  )
                  begin                                                         //-----------------------------------
                    if(WrFence_sent==0 ) 
                    begin
                        af2cp_sTxPort.c1.valid         <= 1'b1;
                    end
                    WrFence_sent                       <= 1'b1;
                    af2cp_sTxPort.c1.hdr.vc_sel        <= t_ccip_vc'(wr_ch_type);
                    af2cp_sTxPort.c1.hdr.req_type      <= eREQ_WRFENCE;        
                    af2cp_sTxPort.c1.hdr.address[41:0] <= '0;
                    af2cp_sTxPort.c1.hdr.mdata[15:0]   <= '0;
                    af2cp_sTxPort.c1.hdr.sop           <= 1'b0;                 
                    af2cp_sTxPort.c1.hdr.cl_len        <= eCL_LEN_1;
                  end
        
                  if(                                                           // Write DSM Status
                     !cp2af_sRxPort_T1.c1TxAlmFull
                     & WrFence_sent
                  )
                  begin                                                         //-----------------------------------
                    if(status_write==0)
                    begin
                        dsm_number                     <= dsm_number + 1'b1;
                        af2cp_sTxPort.c1.valid         <= 1'b1;
                    end
                    status_write                       <= 1'b1;
                    af2cp_sTxPort.c1.hdr.vc_sel        <= t_ccip_vc'(wr_ch_type);
                    af2cp_sTxPort.c1.hdr.req_type      <= eREQ_WRLINE_M;
                    af2cp_sTxPort.c1.hdr.address[41:0] <= ds_stat_address;
                    af2cp_sTxPort.c1.hdr.mdata[15:0]   <= 16'hffff;
                    af2cp_sTxPort.c1.hdr.sop           <= 1'b1;                 // DSM Write is single CL write
                    af2cp_sTxPort.c1.hdr.cl_len        <= eCL_LEN_1;
                  end
        
                else if( WrHdr_valid_T4 & !test_stop )                         // Write to Destination Workspace
                begin                                                          //-------------------------------------
                    af2cp_sTxPort.c1.hdr.vc_sel        <= sop? t_ccip_vc'(wr_ch_type) : af2cp_sTxPort.c1.hdr.vc_sel ; //for multi-cl write dont randomise vc within a packet
                    af2cp_sTxPort.c1.hdr.req_type      <= wrreq_type;
                    af2cp_sTxPort.c1.hdr.address[41:0] <= WrAddr;
                    af2cp_sTxPort.c1.hdr.mdata[15:0]   <= txFifo_WrTID_qq;
                    af2cp_sTxPort.c1.hdr.sop           <= sop;
                    af2cp_sTxPort.c1.hdr.cl_len        <= t_ccip_clLen'(txFifo_WrLen_qq);
                    af2cp_sTxPort.c1.valid             <= 1'b1;
                    Num_Writes                         <= (wrreq_type == eREQ_WRFENCE) ? Num_Writes : Num_Writes + 1'b1;
                end
            end // re2xy_go

        // Channel 0
        if(  re2xy_go && rnd_delay 
          && RdHdr_valid_q)                                                     // Read from Source Workspace
        begin                                                                   //----------------------------------
            af2cp_sTxPort.c0.hdr.vc_sel        <= t_ccip_vc'(rd_ch_type);
            af2cp_sTxPort.c0.hdr.req_type      <= rdreq_type;
            af2cp_sTxPort.c0.hdr.address[41:0] <= RdAddr;
            af2cp_sTxPort.c0.hdr.mdata[15:0]   <= ab2re_RdTID_q;
            af2cp_sTxPort.c0.valid             <= 1'b1;
            af2cp_sTxPort.c0.hdr.cl_len        <= t_ccip_clLen'(ab2re_RdLen_q);
            Num_Reads                          <= Num_Reads + re2xy_multiCL_len + 1'b1;   
        end

        //--------------------------------------------------------------------------
        // Rx Response Path
        //--------------------------------------------------------------------------
        cp2af_sRxPort_T1       <= cp2af_sRxPort;

        // Counters
        //--------------------------------------------------------------------------
        if(re2xy_go)                                                // Count #clks after test start
        begin
            Num_ticks_low   <= Num_ticks_low + 1'b1;
            if(&Num_ticks_low)
                Num_ticks_high  <= Num_ticks_high + 1'b1;
        end

        if(re2xy_go & cp2af_sRxPort.c0TxAlmFull )                   
            Num_C0stall     <= Num_C0stall + 1'b1;

        if(re2xy_go & cp2af_sRxPort.c1TxAlmFull)
            Num_C1stall     <= Num_C1stall + 1'b1;
        
        // Read Request
        tx_c0_req_valid        <= af2cp_sTxPort.c0.valid && (af2cp_sTxPort.c0.hdr.req_type==eREQ_RDLINE_I || af2cp_sTxPort.c0.hdr.req_type==eREQ_RDLINE_S); 
        tx_rd_req_len          <= af2cp_sTxPort.c0.hdr.cl_len[1:0];
        // Read Response
        rx_c0_resp_valid       <= cp2af_sRxPort_T1.c0.rspValid && cp2af_sRxPort_T1.c0.hdr.resp_type==eRSP_RDLINE; 
        
        // Write Request                    
        tx_c1_req_valid        <= af2cp_sTxPort.c1.valid && (af2cp_sTxPort.c1.hdr.req_type==eREQ_WRLINE_I || af2cp_sTxPort.c1.hdr.req_type==eREQ_WRLINE_M || af2cp_sTxPort.c1.hdr.req_type== eREQ_WRPUSH_I ); 
        // Write Response
        rx_c1_resp_valid       <= cp2af_sRxPort_T1.c1.rspValid && cp2af_sRxPort_T1.c1.hdr.resp_type==eRSP_WRLINE; 
        rx_wr_resp_fmt         <= cp2af_sRxPort_T1.c1.hdr.format;
        rx_wr_resp_cl_num      <= cp2af_sRxPort_T1.c1.hdr.cl_num[1:0];
        
        num_rd_sent            <= tx_rd_req_len + 1'b1;
        tx_c0_req_valid_q      <= tx_c0_req_valid;
        rx_c0_resp_valid_q     <= rx_c0_resp_valid;
        tx_c1_req_valid_q      <= tx_c1_req_valid;
        rx_c1_resp_valid_q     <= rx_c1_resp_valid;
                
        // Track number of pending Reads
        case({rx_c0_resp_valid_q , tx_c0_req_valid_q})
          2'b00: Num_RdPend    <= Num_RdPend;
          2'b01: Num_RdPend    <= Num_RdPend + num_rd_sent;
          2'b10: Num_RdPend    <= Num_RdPend - 1'h1;
          2'b11: Num_RdPend    <= Num_RdPend + num_rd_sent - 1'h1;
        endcase 
        
        case (rx_wr_resp_fmt)
          1'b0:  num_wr_recvd  <= 1'h1;
          1'b1:  num_wr_recvd  <= rx_wr_resp_cl_num + 1'h1;
        endcase
        
        // Track number of pending Writes
        case({rx_c1_resp_valid_q, tx_c1_req_valid_q})
          2'b00: Num_WrPend    <= Num_WrPend;
          2'b01: Num_WrPend    <= Num_WrPend + 1'h1; 
          2'b10: Num_WrPend    <= Num_WrPend - num_wr_recvd;
          2'b11: Num_WrPend    <= Num_WrPend - num_wr_recvd + 1'h1;         
        endcase 

        // For LPBK1 (memory copy): stall reads  if Num_RdCredits less than 0. Read credits are limited by the depth of Write fifo
        // Wr fifo depth in requestor is 128. Therefore max num write pending should be less than 128.

        case ({af2cp_sTxPort.c0.valid,af2cp_sTxPort.c1.valid })               
          2'b01: 
          begin
            if (af2cp_sTxPort.c1.hdr.sop) 
            Num_RdCredits <= Num_RdCredits + 1'b1;                                // 1Wr sent
            end
      
          2'b10:
          begin      
            Num_RdCredits <= Num_RdCredits - 1'b1;                                // 1Rd sent
          end  

          2'b11: 
          begin
            if (!af2cp_sTxPort.c1.hdr.sop) 
            Num_RdCredits <= Num_RdCredits - 1'b1;                                // 1Rd + 1Wr sent
          end
          
          default: 
          begin  
            Num_RdCredits <= Num_RdCredits;
          end
        endcase
        re2ab_stallRd     <= ($signed(Num_RdCredits)<=0);
        
        // Error Detection Logic
        //--------------------------
        // synthesis translate_off
        if(|ErrorVector)
            $finish();
        // synthesis translate_on

        if(Num_RdPend[11]==1'b1)
        begin
            ErrorVector[0]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: unexpected RxRead response");
            /*synthesis translate_on */
        end
        
        if(Num_WrPend[11]==1'b1)
        begin
            ErrorVector[0]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: unexpected RxWrite response");
            /*synthesis translate_on */
        end

        if(txFifo_Full & txFifo_WrEn)
        begin
            ErrorVector[2]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: wr fifo overflow");
            /*synthesis translate_on */
        end

        if(ErrorVector[3]==0)
            ErrorVector[3]  <= ab2re_ErrorValid;

        /* synthesis translate_off */
        // if(af2cp_sTxPort.c1.valid )
        //     $display("*Req Type: %x \t Addr: %x \n Data: %x", af2cp_sTxPort.c1.hdr.req_type, af2cp_sTxPort.c1.hdr.address, af2cp_sTxPort.c1.data);

        // if(af2cp_sTxPort.c0.valid)
        //     $display("*Req Type: %x \t Addr: %x", af2cp_sTxPort.c0.hdr.req_type, af2cp_sTxPort.c0.hdr.address);

        /* synthesis translate_on */


        // Use for Debug- if no transactions going across the CCI interface # clks > inactivity threshold 
        // than set the flag. You may use this as a trigger signal in logic analyzer
        if(af2cp_sTxPort.c1.valid  || af2cp_sTxPort.c0.valid)
            inact_cnt  <= cr_inact_thresh;
        else if(re2xy_go)
            inact_cnt  <= inact_cnt - 1'b1;

        if(inact_timeout==0)
        begin
            if(inact_cnt==0)
                inact_timeout   <= 1'b1;
        end
        else if(af2cp_sTxPort.c1.valid  || af2cp_sTxPort.c0.valid)
        begin
            inact_timeout   <= 0;
        end

        if(!test_Reset_n)
        begin
            Num_Reads               <= 0;
            Num_Writes              <= 0;
            Num_RdPend              <= 0;
            Num_WrPend              <= 0;
            Num_ticks_low           <= 0;
            Num_ticks_high          <= 0;
            re2xy_go                <= 0;

            re2ab_CfgValid_d        <= 0;
            ErrorVector             <= 0;
            status_write            <= 0;
            interrupt_sent          <= 0;
            send_interrupt          <= 0;
            inact_cnt               <= 0;
            inact_timeout           <= 0;
            delay_lfsr              <= 1;
            delay_lfsr1             <= 1;
            Num_C0stall             <= 0;
            Num_C1stall             <= 0;
            Num_RdCredits           <= (2**PEND_THRESH-8);           // Max num rdcredits is 128. But 128 multiCL Reads could in turn lead to 512 writes
                                                                     // So, TxWriteFIFO is made 512 deep and RdCredit return is adjusted accordingly
            dsm_status_wren_a       <= 0;     
            dsm_status_wren_b       <= 0;     
            dsm_status_wren_c       <= 0;     
            test_stop               <= 0;
            WrFence_sent            <= 0;  
            test_cmplt              <= 0;
            read_only_test          <= 0;            
        end
    end

    always @(posedge Clk_400)                                              // Computes NLB start and end overheads
    begin                                                                  //-------------------------------------
        if(!test_go)
        begin
            penalty_start   <= 0;
            penalty_start_f <= 0;
            penalty_end     <= 8'h2;
        end
        else
        begin
            if(!penalty_start_f & (af2cp_sTxPort.c0.valid | af2cp_sTxPort.c1.valid ))
            begin
                penalty_start_f   <= 1'b1;
                penalty_start     <= Num_ticks_low[7:0];                    /* synthesis translate_off */
                $display ("NLB_INFO : start penalty = %d ", Num_ticks_low); /* synthesis translate_on */
            end

            penalty_end <= penalty_end + 1'b1;
            if( cp2af_sRxPort.c0.rspValid 
              | cp2af_sRxPort.c1.rspValid | cp2af_sRxPort.c0.mmioWrValid
              )
            begin
                penalty_end     <= 8'h2;
            end

            if(ab2re_TestCmp
              && !cp2af_sRxPort.c1TxAlmFull
              && !status_write)
            begin                                                       /* synthesis translate_off */
                $display ("NLB_INFO : end penalty = %d ", penalty_end); /* synthesis translate_on */
            end

        end
    end

    
    logic rd_pend_thresh;
    logic wr_pend_thresh;
    always @(*)
    begin
        
        RdHdr_valid = re2xy_go
        && !status_write
        && rnd_delay
        && !cp2af_sRxPort.c0TxAlmFull    
        && ab2re_RdEn;

        re2ab_RdSent= RdHdr_valid & !rd_pend_thresh; 

        txFifo_RdAck = re2xy_go && rnd_delay  && !cp2af_sRxPort.c1TxAlmFull && txFifo_Dout_v && !wr_pend_thresh;
        wrreq_type   = txFifo_WrFence_qq ? eREQ_WRFENCE
                      :cr_wrlineI_en     ? eREQ_WRLINE_I 
                      : cr_wrpushI_en    ? eREQ_WRPUSH_I
                                         : eREQ_WRLINE_M;

    end
    always @(posedge Clk_400)
    begin
        rd_pend_thresh <= Num_RdPend[10] && Num_RdPend[9];
        wr_pend_thresh <= Num_WrPend[10] && Num_WrPend[9];
        WrHdr_valid_T1 <= txFifo_RdAck;
        WrHdr_valid_T2 <= WrHdr_valid_T1 & re2xy_go;
        WrHdr_valid_T3 <= WrHdr_valid_T2;
        WrHdr_valid_T4 <= WrHdr_valid_T3;
    if(!test_Reset_n)
        begin
            WrHdr_valid_T1 <= 0;
            WrHdr_valid_T2 <= 0;
            WrHdr_valid_T3 <= 0;
            WrHdr_valid_T4 <= 0;
        end
    end

    //----------------------------------------------------------------------------------------------------------------------------------------------
    //                                                              Instances
    //----------------------------------------------------------------------------------------------------------------------------------------------
    // Tx Write request fifo. Some tests may have writes dependent on reads, i.e. a read response will generate a write request
    // If the CCI-S write channel is stalled, then the write requests will be queued up in this Tx fifo.

    // NOTE: RAM inside the FIFO is currently sized to handle 556 bits (din/dout) and 512 deep 
    // Regenerate the RAM with additional bits if you increase the width/depth of this FIFO
    
    // FIFO Bitmap - 556 bits wide and 512 bits deep
    
    // [551:550]   - ab2re_WrLen
    // [549]       - ab2re_WrSop
    // [548]       - ab2re_WrFence
    // [547:36]    - ab2re_WrDin
    // [35:16]     - ab2re_WrAddr
    // [15:0]      - ab2re_WrTID
    wire [3+1+2+1+1+512+ADDR_LMT+15:0]txFifo_Din= { ab2re_WrLen,
                                                    ab2re_WrSop,
                                                    ab2re_WrFence,
                                                    ab2re_WrDin,
                                                    ab2re_WrAddr, 
                                                    ab2re_WrTID
                                                  };
    wire [3+1+2+1+1+512+ADDR_LMT+15:0]txFifo_Dout;
    assign                  txFifo_WrLen    = txFifo_Dout[2+1+1+DATA_WIDTH+ADDR_LMT+16-1: 1+1+1+DATA_WIDTH+ADDR_LMT+16-1];
    assign                  txFifo_WrSop    = txFifo_Dout[1+1+DATA_WIDTH+ADDR_LMT+16-1];
    assign                  txFifo_WrFence  = txFifo_Dout[1+DATA_WIDTH+ADDR_LMT+16-1];
    assign                  txFifo_WrDin    = txFifo_Dout[ADDR_LMT+16+:DATA_WIDTH];
    assign                  txFifo_WrAddr   = txFifo_Dout[16+:ADDR_LMT];
    assign                  txFifo_WrTID    = txFifo_Dout[15:0];
  
    wire  [9-1:0] txFifo_count;                
    nlb_C1Tx_fifo #(.DATA_WIDTH  (3+1+2+1+1+DATA_WIDTH+ADDR_LMT+16),
                     .CTL_WIDTH   (0),
                     .DEPTH_BASE2 (9),         
                     .GRAM_MODE   (3),
                     .FULL_THRESH (2**9-8)     
    )nlb_writeTx_fifo
    (                                          //--------------------- Input  ------------------
        .Resetb            (test_Reset_n),
        .Clk               (Clk_400),    
        .fifo_din          (txFifo_Din),          
        .fifo_ctlin        (),
        .fifo_wen          (txFifo_WrEn),      
        .fifo_rdack        (txFifo_RdAck),
                                               //--------------------- Output  ------------------
        .T2_fifo_dout      (txFifo_Dout),        
        .T0_fifo_ctlout    (),
        .T0_fifo_dout_v    (txFifo_Dout_v),
        .T0_fifo_empty     (),
        .T0_fifo_full      (txFifo_Full),
        .T0_fifo_count     (txFifo_count),
        .T0_fifo_almFull   (txFifo_AlmFull),
        .T0_fifo_underflow (),
        .T0_fifo_overflow  ()
    ); 
    
    // Function: Returns physical address for a DSM register
    function automatic [41:0] dsm_offset2addr;
        input    [9:0]  offset_b;
        input    [63:0] base_b;
        begin
            dsm_offset2addr = base_b[47:6] + offset_b[9:6];
        end
    endfunction

    //----------------------------------------------------------------
    // For signal tap
    //----------------------------------------------------------------
/*

        (* noprune *) reg [3:0]                 DEBUG_nlb_error;
        (* noprune *) reg [31:0]                DEBUG_Num_Reads;
        (* noprune *) reg [31:0]                DEBUG_Num_Writes;
        (* noprune *) reg                       DEBUG_inact_timeout;
        (* noprune *) reg [9:0]                 DEBUG_C0TxHdrID;
        (* noprune *) reg [31:0]                DEBUG_C0TxHdrAddr;
        (* noprune *) reg [9:0]                 DEBUG_C1TxHdrID;
        (* noprune *) reg [31:0]                DEBUG_C1TxHdrAddr;
        (* noprune *) reg [16:0]                DEBUG_C1TxData;
        (* noprune *) reg [9:0]                 DEBUG_C0RxHdrID;
        (* noprune *) reg [8:0]                 DEBUG_C0RxData;
        (* noprune *) reg [9:0]                 DEBUG_C1RxHdrID;
        (* noprune *) reg                       DEBUG_C0TxValid;
        (* noprune *) reg                       DEBUG_C0RxValid;
        (* noprune *) reg                       DEBUG_C1TxValid;
        (* noprune *) reg                       DEBUG_C1RxValid;
        (* noprune *) reg                       DEBUG_txFifo_Dout_v;
        (* noprune *) reg                       DEBUG_txFifo_RdAck;
        (* noprune *) reg                       DEBUG_txFifo_WrEn;
        (* noprune *) reg                       DEBUG_txFifo_Full;
        (* noprune *) reg [4:0]                 DEBUG_txFifo_Din, DEBUG_txFifo_Dout;
        (* noprune *) reg [15:0]                DEBUG_txFifo_WrCount, DEBUG_txFifo_RdCount;
        (* noprune *) reg [9-1:0]               DEBUG_txFifo_count;                            // TODO: was PEND_THRESH (7)


        always @(posedge Clk_400)
        begin
            DEBUG_nlb_error[3:0]    <= ErrorVector[3:0];
            DEBUG_Num_Reads         <= Num_Reads;
            DEBUG_Num_Writes        <= Num_Writes;
            DEBUG_inact_timeout     <= inact_timeout;
            DEBUG_C0TxHdrID         <= af2cp_sTxPort.c0.hdr.mdata[9:0];
            DEBUG_C0TxHdrAddr       <= af2cp_sTxPort.c0.hdr.address[31:0];
            DEBUG_C1TxHdrID         <= af2cp_sTxPort.c1.hdr.mdata[9:0];
            DEBUG_C1TxHdrAddr       <= af2cp_sTxPort.c1.hdr.address[31:0];
            DEBUG_C1TxData          <= af2cp_sTxPort.c1.data[16:0];
            DEBUG_C0RxHdrID         <= cp2af_sRxPort.c0.hdr.mdata[9:0];
            DEBUG_C0RxData          <= cp2af_sRxPort.c0.data[8:0];
            DEBUG_C1RxHdrID         <= cp2af_sRxPort.c1.hdr.mdata[9:0];
            DEBUG_C0TxValid         <= af2cp_sTxPort.c0.valid;
            DEBUG_C1TxValid         <= af2cp_sTxPort.c1.valid;
            DEBUG_C0RxValid         <= cp2af_sRxPort.c0.rspValid;
            DEBUG_C1RxValid         <= cp2af_sRxPort.c1.rspValid;

            DEBUG_txFifo_Dout_v     <= txFifo_Dout_v;
            DEBUG_txFifo_RdAck      <= txFifo_RdAck;
            DEBUG_txFifo_WrEn       <= txFifo_WrEn;
            DEBUG_txFifo_Full       <= txFifo_Full;
            DEBUG_txFifo_Din        <= txFifo_Din[4:0];
            DEBUG_txFifo_Dout       <= txFifo_Dout[4:0];
            DEBUG_txFifo_count      <= txFifo_count;
            if(txFifo_WrEn)
                DEBUG_txFifo_WrCount <= DEBUG_txFifo_WrCount+1'b1;
            if(txFifo_RdAck)
                DEBUG_txFifo_RdCount <= DEBUG_txFifo_RdCount+1'b1;

            if(!test_Reset_n)
            begin
                DEBUG_txFifo_WrCount<= 0;
                DEBUG_txFifo_RdCount<= 0;
            end
        end
*/

endmodule
