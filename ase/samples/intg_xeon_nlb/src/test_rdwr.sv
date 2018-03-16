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
// Module Name:         test_rdwr.v
// Project:             NLB AFU 
// Description:         streaming read/write test
//
// ***************************************************************************
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//                                         Read Write test
//  ------------------------------------------------------------------------------------------------------------------------------------------------
// Test bandwidth for read and write

`default_nettype none
module test_rdwr #(parameter PEND_THRESH=1, ADDR_LMT=20, MDATA=14)
(
   //---------------------------global signals-------------------------------------------------
   Clk_400,                // input -- Core clock
        
   ab2rw_Mode,              // input [1:0]
  
   rw2ab_WrAddr,            // output   [ADDR_LMT-1:0]    
   rw2ab_WrTID,             // output   [ADDR_LMT-1:0]      
   rw2ab_WrDin,             // output   [511:0]             
   rw2ab_WrEn,              // output                       
   ab2rw_WrSent,            // input                          
   ab2rw_WrAlmFull,         // input  
   re2xy_wrdin_msb,         // input                        
  
   rw2ab_RdAddr,            // output   [ADDR_LMT-1:0]
   rw2ab_RdTID,             // output   [15:0]
   rw2ab_RdEn,              // output 
   ab2rw_RdSent,            // input
   
   ab2rw_RdRspValid,        // input                    
   ab2rw_RdRsp,             // input    [15:0]          
   ab2rw_RdRspAddr,         // input    [ADDR_LMT-1:0]  
   ab2rw_RdData,            // input    [511:0]         
    
   ab2rw_WrRspValid,        // input                  
   ab2rw_WrRsp,             // input    [15:0]            
   ab2rw_WrRspAddr,         // input    [ADDR_LMT-1:0]    

   re2xy_go,                // input                 
   re2xy_NumLines,          // input    [31:0]            
   re2xy_Cont,              // input  
   re2xy_stride,          // input [7:0]             
    
   rw2ab_TestCmp,           // output           
   rw2ab_ErrorInfo,         // output   [255:0] 
   rw2ab_ErrorValid,        // output
   test_Resetb,             // input            
   
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

   input   logic                    Clk_400;               // csi_top:    Clk_400
   
   input   logic  [1:0]             ab2rw_Mode;             // arb:        01 - reads only test, 10 - writes only test, 11 - read/write
   
   output  logic  [ADDR_LMT-1:0]    rw2ab_WrAddr;           // arb:        write address
   output  logic  [15:0]            rw2ab_WrTID;            // arb:        meta data
   output  logic  [511:0]           rw2ab_WrDin;            // arb:        Cache line data
   output  logic                    rw2ab_WrEn;             // arb:        write enable
   input   logic                    ab2rw_WrSent;           // arb:        write issued
   input   logic                    ab2rw_WrAlmFull;        // arb:        write fifo almost full
   input   logic                  re2xy_wrdin_msb;        // requestor:    modifies msb(1) of wrdata to differntiate b/n different multiple afu write patterns     

   
   output  logic  [ADDR_LMT-1:0]    rw2ab_RdAddr;           // arb:        Reads may yield to writes
   output  logic  [15:0]            rw2ab_RdTID;            // arb:        meta data
   output  logic                    rw2ab_RdEn;             // arb:        read enable
   input   logic                    ab2rw_RdSent;           // arb:        read issued
   
   input   logic                    ab2rw_RdRspValid;       // arb:        read response valid
   input   logic  [15:0]            ab2rw_RdRsp;            // arb:        read response header
   input   logic  [ADDR_LMT-1:0]    ab2rw_RdRspAddr;        // arb:        read response address
   input   logic  [511:0]           ab2rw_RdData;           // arb:        read data
   
   input   logic                    ab2rw_WrRspValid;       // arb:        write response valid
   input   logic  [15:0]            ab2rw_WrRsp;            // arb:        write response header
   input   logic  [ADDR_LMT-1:0]    ab2rw_WrRspAddr;        // arb:        write response address

   input   logic                    re2xy_go;               // requestor:  start of frame recvd
   input   logic  [31:0]            re2xy_NumLines;         // requestor:  number of cache lines
   input   logic                    re2xy_Cont;             // requestor:  continuous mode
   input   logic  [31:0]            re2xy_stride;         // requestor:  8-bit test cfg register
   
   output  logic                    rw2ab_TestCmp;          // arb:        Test completion flag
   output  logic  [255:0]           rw2ab_ErrorInfo;        // arb:        error information
   output  logic                    rw2ab_ErrorValid;       // arb:        test has detected an error
   input   logic                    test_Resetb;
   
   output  logic  [1:0]             rw2ab_RdLen;
   output  logic                    rw2ab_RdSop;
   output  logic  [1:0]             rw2ab_WrLen;
   output  logic                    rw2ab_WrSop;

   input   logic                    ab2rw_RdRspFormat;
   input   logic  [1:0]             ab2rw_RdRspCLnum;
   input   logic                    ab2rw_WrRspFormat;
   input   logic  [1:0]             ab2rw_WrRspCLnum;
   input   logic  [1:0]             re2xy_multiCL_len;
   
   //------------------------------------------------------------------------------------------------------------------------

    
   reg      [19:0]            Num_RdReqs;
   reg      [10:0]            Num_RdPend;
   reg      [1:0]             RdFSM;
   reg      [MDATA-2:0]       Rdmdata;

   reg      [19:0]            Num_WrReqs;
   reg      [10:0]            Num_WrPend;
   reg      [1:0]             WrFSM;
   reg      [MDATA-2:0]       Wrmdata;
   
  
   logic                      rw2ab_RdEn_q;
   logic                      ab2rw_RdSent_q;
   logic                      ab2rw_RdRspValid_q;
   logic    [1:0]             RdFSM_q;  
   
   logic                      rw2ab_WrEn_q;       
   logic                      ab2rw_WrSent_q;
   logic                      ab2rw_WrRspValid_q;
   logic                      ab2rw_WrRspFormat_q; 
   logic    [1:0]             ab2rw_WrRspCLnum_q;
   logic    [1:0]             WrFSM_q;
   
   assign rw2ab_RdTID = {Rdmdata, 1'b1};
   assign rw2ab_WrTID = {Wrmdata, 1'b0};

   assign rw2ab_RdEn  = (RdFSM == 2'h1);
   assign rw2ab_WrEn  = (WrFSM == 2'h1);
   
   logic [6:0] stride;
   assign stride = re2xy_stride[6:0];

   always @(posedge Clk_400)
   begin
     rw2ab_ErrorInfo  <= 'hx;   
     if (!test_Resetb)
       begin
         rw2ab_ErrorValid <= 0;
         rw2ab_TestCmp    <= 0;
       end
     else
       begin
         rw2ab_ErrorValid <= 0;
         rw2ab_TestCmp    <= (((WrFSM_q == 2'h2 && Num_WrPend == 0) && (RdFSM_q == 2'h2 && Num_RdPend == 0)));
       end
   end
   
   always @(posedge Clk_400)
   begin
         RdFSM_q          <= RdFSM;
         case(RdFSM)       /* synthesis parallel_case */
         2'h0:
         begin
           rw2ab_RdAddr   <= 0;
           rw2ab_RdLen    <= re2xy_multiCL_len; 
           rw2ab_RdSop    <= 1'b1;
           Num_RdReqs     <= 20'h0 + re2xy_multiCL_len + 1'b1;       // Default is 1 req; implies single CL 

           if(re2xy_go)
             begin
               if ((re2xy_NumLines!=0)&&(ab2rw_Mode[0]==1'b1))
                 RdFSM   <= 2'h1;
               else
                 RdFSM   <= 2'h2;
             end
         end
                     
         2'h1:
         begin
           if(ab2rw_RdSent)
             begin
               rw2ab_RdAddr        <= rw2ab_RdAddr + re2xy_multiCL_len + 16'b1 + stride;
               rw2ab_RdLen         <= rw2ab_RdLen; 
               rw2ab_RdSop         <= 1'b1;
               Num_RdReqs          <= Num_RdReqs + re2xy_multiCL_len + 1'b1;        
               
               if(Num_RdReqs == re2xy_NumLines)
                 if(re2xy_Cont)
                   RdFSM     <= 2'h0;
                 else
                   RdFSM     <= 2'h2;
             end
         end
         
         default:
         begin
           RdFSM     <= RdFSM;
         end
         endcase         

         if ((rw2ab_RdEn && ab2rw_RdSent))
           Rdmdata           <= Rdmdata + 1'b1;
	 
         // Track read responses
         // Timing Fix: Update Num_RdPend - one cycle delayed 
         // Delays Test completion by 1 cycle in non-cont mode
         rw2ab_RdEn_q         <= rw2ab_RdEn;
         ab2rw_RdSent_q       <= ab2rw_RdSent;
         ab2rw_RdRspValid_q   <= ab2rw_RdRspValid;
		 
         if  ((rw2ab_RdEn_q && ab2rw_RdSent_q) && !ab2rw_RdRspValid_q)
           Num_RdPend        <= Num_RdPend + 1'b1 + re2xy_multiCL_len;
         else if ((rw2ab_RdEn_q && ab2rw_RdSent_q) &&  ab2rw_RdRspValid_q)
           Num_RdPend        <= Num_RdPend + re2xy_multiCL_len;
         else if(!(rw2ab_RdEn_q && ab2rw_RdSent_q) &&  ab2rw_RdRspValid_q && ((rw2ab_TestCmp == 1'b0)))
           Num_RdPend        <= Num_RdPend - 1'b1;
           
 
       if (!test_Resetb)
       begin
         rw2ab_RdAddr   <= 0;
         Rdmdata        <= 0;
         Num_RdReqs     <= 20'h1;
         Num_RdPend     <= 0;
         RdFSM          <= 0;
         rw2ab_RdLen    <= 0;
         rw2ab_RdSop    <= 0;
       end
       
   end
   
   always @(posedge Clk_400)
   begin
         WrFSM_q          <= WrFSM;
         case(WrFSM)       /* synthesis parallel_case */
         2'h0:
         begin
           rw2ab_WrAddr   <= 0;
           rw2ab_WrLen    <= re2xy_multiCL_len;
           rw2ab_WrSop    <= 1;
           Num_WrReqs     <= 20'h1;
		              
           if(re2xy_go)
           begin
             if((re2xy_NumLines != 0) && (ab2rw_Mode[1] == 1'b1))
             begin
               WrFSM   <= 2'h1;
             end
               
             else
             begin
               WrFSM   <= 2'h2;
             end
           end
         end

         2'h1:
         begin
           if(ab2rw_WrSent)
             begin
                 if ((re2xy_multiCL_len == 0) || (rw2ab_WrLen == 2'b0))

                  begin
                   // Next request starts a new packet. Use the stride.
                   rw2ab_WrAddr <= rw2ab_WrAddr + 16'b1 + stride;
                   end
               else
                  begin
                    // In the middle of a multi-line packet
                    rw2ab_WrAddr <= rw2ab_WrAddr + 16'b1;
               end
               Num_WrReqs       <= Num_WrReqs + 1'b1;
               
               if (rw2ab_WrLen == 2'b00)
               begin
                 rw2ab_WrLen    <= re2xy_multiCL_len; 
                 rw2ab_WrSop    <= 1;
               end
			   
               else
               begin
                 rw2ab_WrLen    <= rw2ab_WrLen - 1'b1; 
                 rw2ab_WrSop    <= 0;
               end
			   
               if(Num_WrReqs == re2xy_NumLines)
                 if(re2xy_Cont)
                   WrFSM     <= 2'h0;
                 else
                   WrFSM     <= 2'h2;
             end
         end
		         
         default:
         begin
           WrFSM     <= WrFSM;
         end
         endcase

        rw2ab_WrDin         <= {re2xy_wrdin_msb,31'h0,{13{32'h0000_0000}},~rw2ab_WrAddr,rw2ab_WrAddr};

         if ((rw2ab_WrEn && ab2rw_WrSent))
           Wrmdata           <= Wrmdata + 1'b1;
		     
         rw2ab_WrEn_q        <= rw2ab_WrEn;
         ab2rw_WrSent_q      <= ab2rw_WrSent;
         ab2rw_WrRspValid_q  <= ab2rw_WrRspValid;
         ab2rw_WrRspFormat_q <= ab2rw_WrRspFormat;
         ab2rw_WrRspCLnum_q  <= ab2rw_WrRspCLnum;
         
         // Track write responses 
         if (rw2ab_WrEn_q && ab2rw_WrSent_q)                              // One write sent
         begin
           if(!ab2rw_WrRspValid_q)                                        // No write response
             Num_WrPend <= Num_WrPend + 1'b1;               
           else if (ab2rw_WrRspValid_q && ab2rw_WrRspFormat_q) 
             Num_WrPend <= Num_WrPend - ab2rw_WrRspCLnum_q;               // Packed write response
           //else
           //Num_WrPend <= Num_WrPend;                                    // Unpacked write response
         end		 
         
         else if( (rw2ab_TestCmp == 1'b0) )                               // no write sent and test is live
         begin
           if (ab2rw_WrRspValid_q && ab2rw_WrRspFormat_q)                 // Packed write response
             Num_WrPend <= Num_WrPend - (ab2rw_WrRspCLnum_q + 1'b1);
           else if (ab2rw_WrRspValid_q)                                   // unpacked write response
             Num_WrPend <= Num_WrPend - 1'b1;
           //else 
           //Num_WrPend <= Num_WrPend;                                    // No write response
         end

       if (!test_Resetb)
       begin
//         rw2ab_WrAddr   <= 0;
 //        rw2ab_WrDin    <= 0;
         Wrmdata        <= 0;
         Num_WrReqs     <= 20'h1;
         Num_WrPend     <= 0;
         WrFSM          <= 0;	
         rw2ab_WrLen    <= 0;
         rw2ab_WrSop    <= 0;		 
       end
   end
   
endmodule
