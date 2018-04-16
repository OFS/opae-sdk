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
// Module Name:         test_lpbk1.v
// Project:             NLB AFU 
// Description:         memory copy test
//
// ***************************************************************************
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//                                         Loopback 1- memory copy test
//  ------------------------------------------------------------------------------------------------------------------------------------------------
//
// This is a memory copy test. It copies cache lines from source to destination buffer.
//
`default_nettype none
module test_lpbk1 #(parameter PEND_THRESH=1, ADDR_LMT=20, MDATA=14)
(

//      ---------------------------global signals-------------------------------------------------
       Clk_400               ,        // in    std_logic;  -- Core clock

       l12ab_WrAddr,                   // [ADDR_LMT-1:0]        arb:               write address
       l12ab_WrTID,                    // [ADDR_LMT-1:0]        arb:               meta data
       l12ab_WrDin,                    // [511:0]               arb:               Cache line data
       l12ab_WrEn,                     //                       arb:               write enable
       ab2l1_WrSent,                   //                       arb:               write issued
       ab2l1_WrAlmFull,                //                       arb:               write fifo almost full
       
       l12ab_RdAddr,                   // [ADDR_LMT-1:0]        arb:               Reads may yield to writes
       l12ab_RdTID,                    // [15:0]                arb:               meta data
       l12ab_RdEn,                     //                       arb:               read enable
       ab2l1_RdSent,                   //                       arb:               read issued

       ab2l1_RdRspValid_T0,            //                       arb:               read response valid
       ab2l1_RdRsp_T0,                 // [15:0]                arb:               read response header
       ab2l1_RdRspAddr_T0,             // [ADDR_LMT-1:0]        arb:               read response address
       ab2l1_RdData_T0,                // [511:0]               arb:               read data
       ab2l1_stallRd,                  //                       arb:               stall read requests FOR LPBK1

       ab2l1_WrRspValid_T0,            //                       arb:               write response valid
       ab2l1_WrRsp_T0,                 // [15:0]                arb:               write response header
       ab2l1_WrRspAddr_T0,             // [ADDR_LMT-1:0]        arb:               write response address
       re2xy_go,                       //                       requestor:         start the test
       re2xy_NumLines,                 // [31:0]                requestor:         number of cache lines
       re2xy_Cont,                     //                       requestor:         continuous mode

       l12ab_TestCmp,                  //                       arb:               Test completion flag
       l12ab_ErrorInfo,                // [255:0]               arb:               error information
       l12ab_ErrorValid,               //                       arb:               test has detected an error
       test_Resetb,                    //                       requestor:         rest the app
     
       l12ab_RdLen,
       l12ab_RdSop,
       l12ab_WrLen,
       l12ab_WrSop,
        
       ab2l1_RdRspFormat,
       ab2l1_RdRspCLnum_T0,
       ab2l1_WrRspFormat_T0,
       ab2l1_WrRspCLnum_T0,
       re2xy_multiCL_len
);
    //------------------------------------------------------------------------------------------------------------------------
    
    input  logic                  Clk_400;               //                                           Clk_400
          
    output logic  [ADDR_LMT-1:0]  l12ab_WrAddr;           // [ADDR_LMT-1:0]        arb:               write address
    output logic  [15:0]          l12ab_WrTID;            // [15:0]                arb:               meta data
    output logic  [511:0]         l12ab_WrDin;            // [511:0]               arb:               Cache line data
    output logic                  l12ab_WrEn;             //                       arb:               write enable
    input  logic                  ab2l1_WrSent;           //                       arb:               write issued
    input  logic                  ab2l1_WrAlmFull;        //                       arb:               write fifo almost full
         
    output logic  [ADDR_LMT-1:0]  l12ab_RdAddr;           // [ADDR_LMT-1:0]        arb:               Reads may yield to writes
    output logic  [15:0]          l12ab_RdTID;            // [15:0]                arb:               meta data
    output logic                  l12ab_RdEn;             //                       arb:               read enable
    input  logic                  ab2l1_RdSent;           //                       arb:               read issued
     
    input  logic                  ab2l1_RdRspValid_T0;    //                       arb:               read response valid
    input  logic [15:0]           ab2l1_RdRsp_T0;         // [15:0]                arb:               read response header
    input  logic [ADDR_LMT-1:0]   ab2l1_RdRspAddr_T0;     // [ADDR_LMT-1:0]        arb:               read response address
    input  logic [511:0]          ab2l1_RdData_T0;        // [511:0]               arb:               read data
    input  logic                  ab2l1_stallRd;          //                       arb:               stall read requests FOR LPBK1
          
    input  logic                  ab2l1_WrRspValid_T0;    //                       arb:               write response valid
    input  logic [15:0]           ab2l1_WrRsp_T0;         // [15:0]                arb:               write response header
    input  logic [ADDR_LMT-1:0]   ab2l1_WrRspAddr_T0;     // [Addr_LMT-1:0]        arb:               write response address
         
    input  logic                  re2xy_go;               //                       requestor:         start of frame recvd
    input  logic [31:0]           re2xy_NumLines;         // [31:0]                requestor:         number of cache lines
    input  logic                  re2xy_Cont;             //                       requestor:         continuous mode
        
    output logic                  l12ab_TestCmp;          //                       arb:               Test completion flag
    output logic [255:0]          l12ab_ErrorInfo;        // [255:0]               arb:               error information
    output logic                  l12ab_ErrorValid;       //                       arb:               test has detected an error
    input  logic                  test_Resetb;
          
    output logic [1:0]            l12ab_RdLen;
    output logic                  l12ab_RdSop;
    output logic [1:0]            l12ab_WrLen;
    output logic                  l12ab_WrSop;
      
    input  logic                  ab2l1_RdRspFormat;
    input  logic [1:0]            ab2l1_RdRspCLnum_T0;
    input  logic                  ab2l1_WrRspFormat_T0;
    input  logic [1:0]            ab2l1_WrRspCLnum_T0;
          
    input  logic [1:0]            re2xy_multiCL_len;
  
    //------------------------------------------------------------------------------------------------------------------------
   
    logic   [MDATA-1:0]     wr_mdata;
    logic   [1:0]           read_fsm;
    logic                   write_fsm, write_fsm_q;
    logic   [19:0]          Num_Read_req;
    logic   [19:0]          Num_Write_req;
    logic   [19:0]          Num_Write_rsp;  
    logic                   Wr_go;
    logic                   ab2l1_WrAlmFull_q, ab2l1_WrAlmFull_qq;
    logic   [1:0]           wrCLnum, wrCLnum_q, wrCLnum_qq, wrCLnum_qqq;
    logic                   ram_rdValid, ram_rdValid_q, ram_rdValid_qq, ram_rdValid_qqq;
    logic                   wrsop, wrsop_q, wrsop_qq, wrsop_qqq;
    logic   [6:0]           WrReq_tid, WrReq_tid_q;
    logic   [6:0]           WrReq_tid_mCL;
    logic   [6:0]           i;
    logic   [2:0]           multiCL_num;  
    logic                   rd_done;
    logic                   finite_test;
    logic   [4:0]           ram_max_index;
    logic   [7:0]           Num_ram_reads;
    logic   [20:0]          Total_ram_reads;                      
    logic   [15:0]          ab2l1_WrRsp;
    logic   [1:0]           ab2l1_WrRspCLnum;
    logic   [2:0]           pckd_num_wr_rsp;
    logic   [ADDR_LMT-1:0]  ab2l1_WrRspAddr;
    logic                   ab2l1_WrRspFormat;
    logic                   ab2l1_WrRspValid; 
    logic   [15:0]          ab2l1_RdRsp;
    logic   [1:0]           ab2l1_RdRspCLnum;
    logic   [ADDR_LMT-1:0]  ab2l1_RdRspAddr;  
    logic   [511:0]         ab2l1_RdData;       
    logic                   ab2l1_RdRspValid;   
    logic                   memwr_en_ram0, memwr_en_ram1;
    logic   [533:0]         rdrsp_mem_in;
    logic   [8:0]           memrd_addr;  
    logic   [8:0]           memwr_addr;
    logic   [533:0]         wrreq_mem0_out;
    logic   [533:0]         wrreq_mem0_out_q;
    logic   [533:0]         wrreq_mem1_out;
    logic   [533:0]         wrreq_mem1_out_q;
    logic   [1:0]           FSM_Rd_throttle;
    logic   [7:0]           Num_buff_RdReq;
    logic   [1:0]           fsm_read_ctrl;
    logic   [1:0]           fsm_write_ctrl;
    logic   [1:0]           buff0_status;
    logic   [1:0]           buff1_status;
    logic   [1:0]           buff1_status_T1;
    logic   [1:0]           buff1_status_T2;
    logic                   ram1_sel_T3;
    logic                   buff0_rd_cmplt;
    logic                   buff1_rd_cmplt;
    logic                   buff0_wr_cmplt;
    logic                   buff1_wr_cmplt;
    logic   [20:0]          Total_Num_RdRsp;
    logic   [9:0]           Num_RdRsp_buff0;
    logic   [9:0]           Num_RdRsp_buff1;
    logic                   trigger_rds_done;
    logic                   Wr_cmplt;
    logic                   Wr_cmplt_q;
    logic                   rd_cmplt;
    logic                   Read_Buffer_ID;
    logic                   buff0_serviced;
    logic                   buff1_serviced;
    
    (* maxfan=32 *) logic   ram1_sel_T4;
    (* maxfan=1  *) logic   [1:0] CL_ID;
       
    localparam              mCL1_depth_base2 = 7;
    localparam              mCL2_depth_base2 = 8;
    localparam              mCL4_depth_base2 = 9;
    localparam              READ       = 2'b00;
    localparam              READ_DONE  = 2'b01;
    localparam              WRITE      = 2'b10;
    localparam              WRITE_DONE = 2'b11;
    
    // ----------------------------------------------------------------------------
    // Registering Rsp inputs - Adds 1 additional cycle bw last Rsp to completion 
    // ----------------------------------------------------------------------------
    always@(posedge Clk_400)
    begin
      ab2l1_WrRsp        <= ab2l1_WrRsp_T0;
      ab2l1_WrRspCLnum   <= ab2l1_WrRspCLnum_T0;
      ab2l1_WrRspAddr    <= ab2l1_WrRspAddr_T0;
      ab2l1_WrRspFormat  <= ab2l1_WrRspFormat_T0; 
      ab2l1_WrRspValid   <= ab2l1_WrRspValid_T0;
      ab2l1_RdRsp        <= ab2l1_RdRsp_T0;
      ab2l1_RdRspCLnum   <= ab2l1_RdRspCLnum_T0;
      ab2l1_RdRspAddr    <= ab2l1_RdRspAddr_T0;
      ab2l1_RdData       <= ab2l1_RdData_T0;
      ab2l1_RdRspValid   <= ab2l1_RdRspValid_T0; 
    end 

    // ----------------------------------------------------------------------------
    // Static Update based on multi CL length
    // ----------------------------------------------------------------------------
    always@(posedge Clk_400)
    begin
      finite_test                    <= re2xy_go && !re2xy_Cont;
      if (!test_Resetb)
      begin
        ram_max_index                <= mCL1_depth_base2;
        finite_test                  <= 0;
      end
      
      else
      begin
        case (re2xy_multiCL_len)
          2'b00  :  ram_max_index    <= mCL1_depth_base2;
          2'b01  :  ram_max_index    <= mCL2_depth_base2;
          2'b11  :  ram_max_index    <= mCL4_depth_base2;
          default:  ram_max_index    <= mCL1_depth_base2;
        endcase     
      end   
    end

    // ------------------------------------------------------
    // Read FSM
    // ------------------------------------------------------  
    always @(posedge Clk_400)
    begin
            case(read_fsm)  /* synthesis parallel_case */
            2'h0:   
            begin  // Wait for re2xy_go
              l12ab_RdAddr                       <= 0;
              l12ab_RdLen                        <= re2xy_multiCL_len; 
              l12ab_RdSop                        <= 1'b1;
              Num_Read_req                       <= 20'h0 + re2xy_multiCL_len + 1'b1;       // Default is 1 req; implies single CL 
              
              if(re2xy_go)
                if(re2xy_NumLines!=0)
                  read_fsm                       <= 2'h1;
                else    
                  read_fsm                       <= 2'h2;
            end
            
            2'h1:   
            begin  // Send read requests
              if(ab2l1_RdSent)        
              begin   
                l12ab_RdAddr                     <= l12ab_RdAddr + re2xy_multiCL_len + 1'b1; // multiCL_len = {0/1/2/3}
                l12ab_RdLen                      <= l12ab_RdLen;                             // All reqs are uniform. Based on test cfg
                l12ab_RdSop                      <= 1'b1;                                    // All reqs are uniform. Based on test cfg
                Num_Read_req                     <= Num_Read_req + re2xy_multiCL_len + 1'b1; // final count will be same as re2xy_NumLines   
                                                                 
                if(Num_Read_req == re2xy_NumLines)
                if(re2xy_Cont)    read_fsm       <= 2'h0;
                else              read_fsm       <= 2'h2;
              end // ab2l1_RdSent
            end
            
            default:              read_fsm       <= read_fsm;
            endcase
            
            if(read_fsm==2'h2 && Num_Write_rsp==re2xy_NumLines)
            begin            
              l12ab_TestCmp                      <= 1'b1;
            end
    
            // Error logic
            if(l12ab_WrEn && ab2l1_WrSent==0)
            begin
              // WrFSM assumption is broken
              $display ("%m LPBK1 test WrEn asserted, but request Not accepted by requestor");
              l12ab_ErrorValid                   <= 1'b1;
              l12ab_ErrorInfo                    <= 1'b1;
            end
           
            if(!test_Resetb)
            begin
              l12ab_TestCmp                      <= 0;
              l12ab_ErrorInfo                    <= 0;
              l12ab_ErrorValid                   <= 0;
              read_fsm                           <= 0;
              Num_Read_req                       <= 20'h1;
              l12ab_RdLen                        <= 0;
              l12ab_RdSop                        <= 1;
            end
    end
    
    always @(*)
    begin
      l12ab_RdTID = 0;
      l12ab_RdTID[MDATA-1:0] = {Read_Buffer_ID, Num_buff_RdReq[7:0]};
      l12ab_RdEn = (read_fsm  ==2'h1) & !Num_buff_RdReq[7];
    end
    
    // ------------------------------------------------------
    // RAM to store RdRsp Data and Address
    // 2 cycle Read latency  
    // 2 cycle Rd2Wr latency
    // Unknown data returned if same address is read, written 
    // ------------------------------------------------------  
    
    // RAM0 Instance 
    lpbk1_RdRspRAM2PORT rdrsp_mem0 (
      .data      (rdrsp_mem_in),    //  ram_input.datain
      .wraddress (memwr_addr),      //           .wraddress
      .rdaddress (memrd_addr),      //           .rdaddress
      .wren      (memwr_en_ram0),   //           .wren
      .clock     (Clk_400),         //           .clock
      .q         (wrreq_mem0_out)   // ram_output.dataout
    );
    
    // RAM1 Instance  
    lpbk1_RdRspRAM2PORT rdrsp_mem1 (
      .data      (rdrsp_mem_in),    //  ram_input.datain
      .wraddress (memwr_addr),      //           .wraddress
      .rdaddress (memrd_addr),      //           .rdaddress
      .wren      (memwr_en_ram1),   //           .wren
      .clock     (Clk_400),         //           .clock
      .q         (wrreq_mem1_out)   // ram_output.dataout
    );

    // ----------------------------------------------------------------------------
    // Collect RdRsp in RAM0/1
    // Initiate Write Requests
    // Collect Write Responses
    // ----------------------------------------------------------------------------
    always @(posedge Clk_400)
    begin
      case (FSM_Rd_throttle)
        2'h0:
        begin
          if (ab2l1_RdSent)
            Num_buff_RdReq   <= Num_buff_RdReq + 1'b1;
          
          if (Num_buff_RdReq[7] && Read_Buffer_ID == 0)
            FSM_Rd_throttle  <= 2'h1;
          
          if (Num_buff_RdReq[7] && Read_Buffer_ID == 1)
            FSM_Rd_throttle  <= 2'h2;    
        end
        
        2'h1:
        begin
          if (buff1_serviced && buff1_status == READ)
          begin
            FSM_Rd_throttle  <= 2'h0;
            Read_Buffer_ID   <= 1;
            Num_buff_RdReq   <= 0;
            buff1_serviced   <= 0;
          end        
        end
        
        2'h2:
        begin
          if (buff0_serviced && buff0_status == READ)
          begin
            FSM_Rd_throttle  <= 2'h0;
            Read_Buffer_ID   <= 0;
            Num_buff_RdReq   <= 0;
            buff0_serviced   <= 0;
          end 
        end
        
        default:
          FSM_Rd_throttle    <= FSM_Rd_throttle;
      endcase
  
      case(fsm_read_ctrl)  /* synthesis parallel_case */
        2'h0:                             
        begin
          if (buff0_status == WRITE_DONE || buff0_status == READ) // all buffer 0 writes complete
          begin
            fsm_read_ctrl  <= 2'h1;
            buff0_status   <= READ;
          end
        end
        
        2'h1:
        begin
          if (buff0_rd_cmplt)           // all buffer 0 reads received 
          begin
            buff0_status   <= READ_DONE;
            fsm_read_ctrl  <= 2'h2;
            buff0_rd_cmplt <= 0;
          end
          
          if (buff1_status == WRITE_DONE) // all buffer 1 writes complete
          begin
            buff1_status   <= READ;
          end
        end
        
        2'h2:
        begin
          if (buff1_status == WRITE_DONE || buff1_status == READ) // all buffer 1 writes complete
          begin
            fsm_read_ctrl  <= 2'h3;
            buff1_status   <= READ;
          end
        end
        
        2'h3:
        begin
          if (buff1_rd_cmplt)           // all buffer 1 reads received  
          begin
            buff1_status   <= READ_DONE;
            fsm_read_ctrl  <= 2'h0;
            buff1_rd_cmplt <= 0;
          end
          
          if (buff0_status == WRITE_DONE) // all buffer 0 writes complete
          begin
            buff0_status   <= READ;
          end
        end
      endcase
    
      case(fsm_write_ctrl)  /* synthesis parallel_case */
        2'h0:
        begin
          if (buff0_status == READ_DONE) // buffer 0 data ready
          begin
            fsm_write_ctrl <= 2'h1;  
          end
        end
      
        2'h1:      
        begin
          buff0_status     <= WRITE;
          if (buff0_wr_cmplt)          // all buffer 0 writes complete 
          begin
            buff0_status   <= WRITE_DONE;
            buff0_serviced <= 1;
            fsm_write_ctrl <= 2'h2;
          end
        end
        
        2'h2:
        begin
          if (buff1_status == READ_DONE) // buffer 1 data ready
          begin
            fsm_write_ctrl <= 2'h3;
          end
        end
        
        2'h3:
        begin
          buff1_status     <= WRITE;
          if (buff1_wr_cmplt)          // all buffer 1 writes complete  
          begin
            buff1_status   <= WRITE_DONE;
            buff1_serviced <= 1;
            fsm_write_ctrl <= 2'h0;
          end
        end
      endcase

      // Store RdResponses in RAM0 and RAM1 
      memwr_en_ram0                     <= 0; 
      memwr_en_ram1                     <= 0; 
      memwr_addr                        <= {ab2l1_RdRsp[6:0],ab2l1_RdRspCLnum[1:0]}; 
      rdrsp_mem_in                      <= {ab2l1_RdData[511:0],ab2l1_RdRspAddr[19:0],ab2l1_RdRspCLnum[1:0]}; 
      
      // Count Total Rd Responses
      if (ab2l1_RdRspValid)
      begin
        Total_Num_RdRsp                 <= Total_Num_RdRsp + 1'b1;
      end
      
      // Compute RdDone for non-cont test
      trigger_rds_done                  <= 0;
      if ((Total_Num_RdRsp == re2xy_NumLines) & finite_test & !rd_cmplt)
      begin
        trigger_rds_done                <= 1;
        rd_cmplt                        <= 1;
      end
      
      // Count RdRsps to buffer0
      if(ab2l1_RdRspValid && !ab2l1_RdRsp[8])
      begin
        memwr_en_ram0                   <= 1;
        Num_RdRsp_buff0                 <= Num_RdRsp_buff0 + 1'b1;
      end
      
      // Count RdRsps to buffer1
      if(ab2l1_RdRspValid && ab2l1_RdRsp[8])
      begin
        memwr_en_ram1                   <= 1;
        Num_RdRsp_buff1                 <= Num_RdRsp_buff1 + 1'b1;
      end
      
      // Buffer0 has all data ready
      if (trigger_rds_done || Num_RdRsp_buff0[ram_max_index])
      begin
        buff0_rd_cmplt  <= 1;
        Num_RdRsp_buff0 <= 0;
      end
      
      // Buffer1 has all data ready
      if (trigger_rds_done || Num_RdRsp_buff1[ram_max_index])
      begin
        buff1_rd_cmplt  <= 1;
        Num_RdRsp_buff1 <= 0;
      end
                      
      // Track Reads from RAM
      if (!write_fsm & Wr_go & !ab2l1_WrAlmFull_qq)
      begin
        Num_ram_reads   <= Num_ram_reads + 1'b1; 
      end
      
      // If all reads done: stop WrFSM, trigger write complete
      Wr_cmplt          <= 0; 
      Wr_cmplt_q        <= Wr_cmplt;
      if ( Num_ram_reads[7] || ((Total_ram_reads > re2xy_NumLines) && finite_test) )
      begin
        Wr_go           <= 0;
        Wr_cmplt        <= 1;
      end
      
      else if (buff1_status == WRITE || buff0_status == WRITE) 
      begin
        Wr_go           <= 1;
      end  
      
      buff0_wr_cmplt    <= 0;
      if (Wr_cmplt && buff0_status == WRITE) 
      begin
        buff0_wr_cmplt  <= 1;
      end
      
      buff1_wr_cmplt    <= 0;
      if (Wr_cmplt && buff1_status == WRITE)
      begin
        buff1_wr_cmplt  <= 1;
      end
      
      if (Wr_cmplt_q)
      Num_ram_reads     <= 0;
      
      // ----------------------------------------------------------------------------
      // WrFSM: Requestor Stores Tx Writes in a FIFO
      // TxFIFO is sized in such a way that writes are guaranteed to be accepted
      // So, ab2l1_WrSent = 0 when WrEn=1 is an error condition
      // ----------------------------------------------------------------------------
      ab2l1_WrAlmFull_q   <= ab2l1_WrAlmFull;
      ab2l1_WrAlmFull_qq  <= ab2l1_WrAlmFull_q;
      case (write_fsm)   /* synthesis parallel_case */
      1'h0:
        begin
          if (Wr_go & !ab2l1_WrAlmFull_qq)
          begin
            // Read first CL of 'num_multi_CL' memWrite requests from RAM
            write_fsm                        <= 1'h1;
            CL_ID                            <= CL_ID + 1'b1;
            memrd_addr                       <= {WrReq_tid[6:0], CL_ID};
            ram_rdValid                      <= 1;
            Total_ram_reads                  <= Total_ram_reads + 1'b1;
            wrsop                            <= 1;
            wrCLnum                          <= re2xy_multiCL_len[1:0];
          end
        end
      
      1'h1:
        begin
          if (|wrCLnum[1:0])
          begin
          // Read remaining CLs of 're2xy_multiCL_len' memWrite requests from RAM
          write_fsm                        <= 1'h1;
          CL_ID                            <= CL_ID + 1'b1;
          memrd_addr                       <= {WrReq_tid[6:0], CL_ID};
          ram_rdValid                      <= 1;
          Total_ram_reads                  <= Total_ram_reads + 1'b1;
          wrsop                            <= 0;
          wrCLnum                          <= wrCLnum - 1'b1;
          end  
                          
          else
          begin         
          // Goto next set of multiCL requests; One cycle bubble between each set of multi CL writes. 
          write_fsm                        <= 1'h0;
          CL_ID                            <= 0;
          ram_rdValid                      <= 0;
          wrsop                            <= 1;
          wrCLnum                          <= re2xy_multiCL_len[1:0];
          WrReq_tid                        <= WrReq_tid + 1'b1;
          end
        end
      
      default:
      begin
        write_fsm                            <= write_fsm;
      end
      endcase  

      // ----------------------------------------------------------------------------
      // Pipeline WrReq parameters till RAM output is valid
      // ----------------------------------------------------------------------------
      ram_rdValid_q                            <= ram_rdValid;
      ram_rdValid_qq                           <= ram_rdValid_q;
      ram_rdValid_qqq                          <= ram_rdValid_qq;
      wrsop_q                                  <= wrsop;
      wrsop_qq                                 <= wrsop_q;
      wrsop_qqq                                <= wrsop_qq;
      wrCLnum_q                                <= wrCLnum;
      wrCLnum_qq                               <= wrCLnum_q;
      wrCLnum_qqq                              <= wrCLnum_qq;
      wrreq_mem0_out_q                         <= wrreq_mem0_out;
      wrreq_mem1_out_q                         <= wrreq_mem1_out;
      buff1_status_T1                          <= buff1_status;
      buff1_status_T2                          <= buff1_status_T1;
      
      if (buff1_status_T2 == WRITE)
        ram1_sel_T3 <= 1;
      else
        ram1_sel_T3 <= 0;
      ram1_sel_T4   <= ram1_sel_T3;
      
      // ----------------------------------------------------------------------------      
      // send Multi CL Write Requests                                                
      // ----------------------------------------------------------------------------      
      l12ab_WrEn                               <= (ram_rdValid_qqq == 1'b1);                                                         
      l12ab_WrSop                              <= wrsop_qqq;
      l12ab_WrLen                              <= wrCLnum_qqq;
      l12ab_WrAddr                             <= wrreq_mem0_out_q[21:2] + wrreq_mem0_out_q[1:0] ;   
      l12ab_WrDin                              <= wrreq_mem0_out_q[533:22];                         
      l12ab_WrTID[15:0]                        <= wrreq_mem0_out_q[17:2]; 
          
      if(ram1_sel_T4)
      begin
        l12ab_WrAddr                           <= wrreq_mem1_out_q[21:2] + wrreq_mem1_out_q[1:0] ;   
        l12ab_WrDin                            <= wrreq_mem1_out_q[533:22];                         
        l12ab_WrTID[15:0]                      <= wrreq_mem1_out_q[17:2]; 
      end
      
      // ----------------------------------------------------------------------------
      // Track Num Write requests
      // ----------------------------------------------------------------------------
      if (l12ab_WrEn)
      begin
        Num_Write_req                          <= Num_Write_req   + 1'b1;
      end    
        
      // ----------------------------------------------------------------------------
      // Track Num Write responses
      // ----------------------------------------------------------------------------
      pckd_num_wr_rsp                          <= ab2l1_WrRspCLnum_T0 + 1'b1;
      if(ab2l1_WrRspValid && ab2l1_WrRspFormat)   // Packed write response
      begin
        Num_Write_rsp                          <= Num_Write_rsp + pckd_num_wr_rsp; 
      end
      
      if (ab2l1_WrRspValid && !ab2l1_WrRspFormat) // unpacked write response
      begin
        Num_Write_rsp                          <= Num_Write_rsp + 1'b1;   
      end
      
      if (!test_Resetb)
      begin
        Wr_go                                  <= 0;
        memwr_en_ram0                          <= 0;       
        memwr_en_ram1                          <= 0;       
        multiCL_num                            <= 1;
        write_fsm                              <= 1'h0;
        WrReq_tid                              <= 0;
        WrReq_tid_mCL                          <= 0;
        CL_ID                                  <= 0;
        ram_rdValid                            <= 0;
        wrsop                                  <= 1;
        wrCLnum                                <= 0;
        l12ab_WrEn                             <= 0;
        l12ab_WrSop                            <= 1;
        l12ab_WrLen                            <= 0;
        Num_Write_req                          <= 20'h1;
        Num_Write_rsp                          <= 0;
        pckd_num_wr_rsp                        <= 0;
        Total_ram_reads                        <= 20'h1;
        Num_ram_reads                          <= 0;
        rd_done                                <= 0;
        Total_Num_RdRsp                        <= 0;
        trigger_rds_done                       <= 0;
        Num_RdRsp_buff0                        <= 0;
        Num_RdRsp_buff1                        <= 0;
        Wr_cmplt                               <= 0;
        Wr_cmplt_q                             <= 0;
        rd_cmplt                               <= 0;
        buff1_status_T1                        <= 0;
        buff1_status_T2                        <= 0;
        ram1_sel_T3                            <= 0; 
        ab2l1_WrAlmFull_q                      <= 0;
        ab2l1_WrAlmFull_qq                     <= 0;
        Read_Buffer_ID                         <= 0;
        Num_buff_RdReq                         <= 0;
        FSM_Rd_throttle                        <= 0;
        fsm_read_ctrl                          <= 0;
        fsm_write_ctrl                         <= 0;
        buff0_status                           <= WRITE_DONE;
        buff1_status                           <= WRITE_DONE;
        buff0_serviced                         <= 0;
        buff1_serviced                         <= 1;
        buff1_rd_cmplt                         <= 0;
        buff0_rd_cmplt                         <= 0;
      end 
    end

   // synthesis translate_off
   logic numCL_error = 0;
   always @(posedge Clk_400)
   begin
     if( re2xy_go && ((re2xy_NumLines)%(re2xy_multiCL_len + 1) != 0)  )
     begin
       $display("%m \m ERROR: Total Num Lines should be exactly divisible by multiCL length");
       $display("\m re2xy_NumLines = %d and re2xy_multiCL_len = %d",re2xy_NumLines,re2xy_multiCL_len);
       numCL_error <= 1'b1;
     end
  
     if(numCL_error)
     $finish();
   end
   // synthesis translate_on
endmodule

