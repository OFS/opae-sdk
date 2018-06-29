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
// Module Name :	  nlb_csr.v
// Project :        NLB AFU update for CCI-P
// Description:     Implements 64-bits read/write port a CSR file
//                  capable of doing 32 and 64 bit rd/wr the register file.
//
// ***************************************************************************
`default_nettype none
`include "vendor_defines.vh"
import ccip_if_pkg::*;
module nlb_csr #(parameter CCIP_VERSION_NUMBER=0)
(
    Clk_400,                       //                              clk_pll:    16UI clock
    SoftReset,                      //                              rst:        ACTIVE HIGH soft reset
    re2cr_wrlock_n,
    // MMIO Requests from CCI-P
    cp2cr_MmioHdr,                // [31:0]                       CSR Request Hdr 
    cp2cr_MmioDin,                   // [63:0]                       CSR read data
    cp2cr_MmioWrEn,                  //                              CSR write strobe
    cp2cr_MmioRdEn,                  //                              CSR read strobe
    // MMIO Responses to CCI-P
    cr2cp_MmioHdr,                // [11:0]                       CSR Response Hdr
    cr2cp_MmioDout,                  // [63:0]                       CSR read data
    cr2cp_MmioDout_v,                //                              CSR read data valid
    // connections to requestor
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
    cr2s1_csr_write,

    re2cr_num_reads,
    re2cr_num_writes,
    re2cr_num_Rdpend,
    re2cr_num_Wrpend,
    re2cr_error
);
input  wire          Clk_400;               // 400MHz clock
input  wire          SoftReset;
input  wire          re2cr_wrlock_n;
// MMIO Requests                           
input  t_ccip_c0_ReqMmioHdr  cp2cr_MmioHdr;        //   CSR Request Hdr
input  t_ccip_mmioData       cp2cr_MmioDin;           //   CSR read data
input  logic                 cp2cr_MmioWrEn;          //   CSR write enable
input  logic                 cp2cr_MmioRdEn;          //   CSR read enable
// MMIO Response                         
output t_ccip_c2_RspMmioHdr  cr2cp_MmioHdr;        //   CSR Response Hdr
output t_ccip_mmioData       cr2cp_MmioDout;          //   CSR read data
output logic                 cr2cp_MmioDout_v;        //   CSR read data valid
// Connections to requestor
(* `KEEP_WIRE *) output wire  [63:0]  cr2re_src_address;
(* `KEEP_WIRE *) output wire  [63:0]  cr2re_dst_address;
(* `KEEP_WIRE *) output wire  [31:0]  cr2re_num_lines;
(* `KEEP_WIRE *) output wire  [31:0]  cr2re_inact_thresh;
(* `KEEP_WIRE *) output wire  [31:0]  cr2re_interrupt0;
(* `KEEP_WIRE *) output wire  [63:0]  cr2re_cfg;
(* `KEEP_WIRE *) output wire  [31:0]  cr2re_ctl;
(* `KEEP_WIRE *) output wire  [31:0]  cr2re_stride;
(* `KEEP_WIRE *) output wire  [63:0]  cr2re_dsm_base;
(* `KEEP_WIRE *) output reg           cr2re_dsm_base_valid;
(* `KEEP_WIRE *) output reg           cr2s1_csr_write;

(* `KEEP_WIRE *) input wire [31:0]    re2cr_num_reads;
(* `KEEP_WIRE *) input wire [31:0]    re2cr_num_writes;
(* `KEEP_WIRE *) input wire [31:0]    re2cr_num_Rdpend;
(* `KEEP_WIRE *) input wire [31:0]    re2cr_num_Wrpend;
(* `KEEP_WIRE *) input wire [31:0]    re2cr_error;

// --------------------------------------------------------------------------
// BBB Attributes
// --------------------------------------------------------------------------
localparam       END_OF_LIST           = 1'h0;  // Set this to 0 if there is another DFH beyond this
localparam       NEXT_DFH_BYTE_OFFSET  = 24'h1000; // Next DFH Byte offset

//----------------------------------------------------------------------------
// CSR Attributes
//----------------------------------------------------------------------------
localparam       RO      = 3'h0;
localparam       RW      = 3'h1;
localparam       RsvdP   = 3'h6;
localparam       RsvdZ   = 3'h6;

//---------------------------------------------------------
// CSR Address Map ***** DO NOT MODIFY *****
//---------------------------------------------------------
localparam      CSR_AFH_DFH_BASE     = 16'h000;                 // RO - Start for the DFH info for this AFU
localparam      CSR_AFH_ID_L         = 16'h008;                 // RO - Lower 64 bits of the AFU ID
localparam      CSR_AFH_ID_H         = 16'h010;                 // RO - Upper 64 bits of the AFU ID
localparam      CSR_DFH_RSVD0        = 16'h018;                 // RO - Offset to next AFU
localparam      CSR_DFH_RSVD1        = 16'h020;                 // RO - Reserved space for DFH managment(?)

localparam      CSR_SCRATCHPAD0      = 16'h100;    // 32b
localparam      CSR_SCRATCHPAD1      = 16'h104;    // 32b
localparam      CSR_SCRATCHPAD2      = 16'h108;    // 64b

localparam      CSR_AFU_DSM_BASEL    = 16'h110;    // 32b             // RW - Lower 32-bits of AFU DSM base address. The lower 6-bbits are 4x00 since the address is cache aligned.
localparam      CSR_AFU_DSM_BASEH    = 16'h114;    // 32b             // RW - Upper 32-bits of AFU DSM base address.

localparam      CSR_SRC_ADDR         = 16'h120;    // 64b             // RW   Reads are targetted to this region 
localparam      CSR_DST_ADDR         = 16'h128;    // 64b             // RW   Writes are targetted to this region
localparam      CSR_NUM_LINES        = 16'h130;    // 32b             // RW   Numbers of cache lines to be read/write
localparam      CSR_CTL              = 16'h138;    // 32b             // RW   Control CSR to start n stop the test
localparam      CSR_CFG              = 16'h140;    // 32b             // RW   Configures test mode, wrthru, cont and delay mode
localparam      CSR_INACT_THRESH     = 16'h148;    // 32b             // RW   set the threshold limit for inactivity trigger

localparam      CSR_SWTEST_MSG       = 16'h158;    // 32b             // RW   Write to this serves as a notification to SW test   
localparam      CSR_STATUS0          = 16'h160;    // 32b                RO   num_read, num_writes
localparam      CSR_STATUS1          = 16'h168;    // 32b                RO   num_Rdpend, num_Wrpend 
localparam      CSR_ERROR            = 16'h170;    // 32b                RO   error
localparam      CSR_STRIDE           = 16'h178;    // 32b           //  stride value    
//---------------------------------------------------------
localparam      NO_STAGED_CSR  = 16'hXXX;       // used for NON late action CSRs
localparam      CFG_SEG_SIZE   = 16'h180>>3;    // Range specified in number of 8B CSRs
localparam[15:0]CFG_SEG_BEG    = 16'h0000;
localparam      CFG_SEG_END    = CFG_SEG_BEG+(CFG_SEG_SIZE<<3);
localparam      L_CFG_SEG_SIZE = $clog2(CFG_SEG_SIZE) == 0?1:$clog2(CFG_SEG_SIZE);

localparam      FEATURE_0_BEG  = 18'h0000;
//localparam      FEATURE_1_BEG  = 18'h1000;

//WARNING: The next localparam must match what is currently in the
//          requestor.v file.  This should be moved to a global package/file
//          that can be used, rather than in two files.  Future Work.  PKB
// PAR Mode
// Each Test implements a different functionality
// Therefore it should really be treated like a different AFU
// For ease of maintainability they are implemented in a single source tree
// At compile time, user can decide which test mode is synthesized.
`ifndef SIM_MODE // PAR_MODE
    `ifdef NLB400_MODE_0
    localparam       NLB_AFU_ID_H    = 64'hD842_4DC4_A4A3_C413;
    localparam       NLB_AFU_ID_L    = 64'hF89E_4336_83F9_040B;
            
    `elsif NLB400_MODE_3
    localparam       NLB_AFU_ID_H    = 64'hF7DF_405C_BD7A_CF72;
    localparam       NLB_AFU_ID_L    = 64'h22F1_44B0_B93A_CD18;
    `elsif NLB400_MODE_7
    localparam       NLB_AFU_ID_H    = 64'h7BAF_4DEA_A57C_E91E;
    localparam       NLB_AFU_ID_L    = 64'h168A_455D_9BDA_88A3;
    `elsif NLB400_MODE_5
    localparam       NLB_AFU_ID_H    = 64'hA0B8_4916_A8A2_12A1;
    localparam       NLB_AFU_ID_L    = 64'hA2EC_457C_84E7_47BC;
    `else
        ** Select a valid NLB Test Mode
    `endif	
`else   // SIM_MODE
    // Temporary Workaround
    // Simulation tests are always expecting same AFU ID
    // ** To be Fixed **
        localparam       NLB_AFU_ID_H        = 64'hC000_C966_0D82_4272;
        localparam       NLB_AFU_ID_L        = 64'h9AEF_FE5F_8457_0612;
`endif

//----------------------------------------------------------------------------------------------------------------------------------------------
reg             rw1c_pulse, rw1s_pulse;
reg  [63:0]     csr_reg [2**L_CFG_SEG_SIZE-1:0];            // register file
wire [15:0]     afu_csr_addr_4B   = cp2cr_MmioHdr.address;
wire [14:0]     afu_csr_addr_8B   = afu_csr_addr_4B[15:1];
wire [1:0]      afu_csr_length    = cp2cr_MmioHdr.length;
wire            ip_select         = afu_csr_addr_8B[14:L_CFG_SEG_SIZE]==CFG_SEG_BEG[15:L_CFG_SEG_SIZE+3];
reg             afu_csr_length_4B_T1, afu_csr_length_8B_T1;
reg             afu_csr_length_4B_T2, afu_csr_length_8B_T2;
reg             afu_csr_length_8B_T3;
t_ccip_mmioData afu_csr_wrdin_T1, afu_csr_dout_T3;
t_ccip_mmioData afu_csr_dout_T2 [1:0];
reg [1:0]       afu_csr_dw_enable_T1, afu_csr_dw_enable_T2, afu_csr_dw_enable_T3;
reg             afu_csr_wren_T1, afu_csr_rden_T1, afu_csr_dout_v_T2, afu_csr_dout_v_T3;
t_ccip_tid      afu_csr_tid_T1, afu_csr_tid_T2, afu_csr_tid_T3;
(* maxfan=1 *)  reg [14:0]      afu_csr_offset_8B_T1;
reg             range_valid;
integer i;
assign cr2re_interrupt0 = 0;

initial begin
    for (i=0;i<2**L_CFG_SEG_SIZE;i=i+1)
        csr_reg[i] = 64'h0;
end

assign     cr2re_ctl             = func_csr_connect_4B(CSR_CTL,csr_reg[CSR_CTL>>3]);
assign     cr2re_stride          = func_csr_connect_4B(CSR_STRIDE,csr_reg[CSR_STRIDE>>3]);
assign     cr2re_dsm_base[31:0]  = func_csr_connect_4B(CSR_AFU_DSM_BASEL,csr_reg[CSR_AFU_DSM_BASEL>>3]);
assign     cr2re_dsm_base[63:32] = func_csr_connect_4B(CSR_AFU_DSM_BASEH,csr_reg[CSR_AFU_DSM_BASEH>>3]);
assign     cr2re_src_address     = csr_reg[CSR_SRC_ADDR>>3];
assign     cr2re_dst_address     = csr_reg[CSR_DST_ADDR>>3];
assign     cr2re_num_lines       = func_csr_connect_4B(CSR_NUM_LINES, csr_reg[CSR_NUM_LINES>>3]);
assign     cr2re_inact_thresh    = func_csr_connect_4B(CSR_INACT_THRESH,csr_reg[CSR_INACT_THRESH>>3]);
assign     cr2re_cfg             = csr_reg[CSR_CFG>>3];

function automatic [31:0] func_csr_connect_4B;
    input [15:0]    address;
    input [63:0]    data_8B;
    begin
        if(address[2])
            func_csr_connect_4B = data_8B[63:32];
        else
            func_csr_connect_4B = data_8B[31:0];
    end
endfunction
//                                         [14:9]              , [8:0]
wire [14:0] feature_0_addr_offset_8B_T1 = {FEATURE_0_BEG[17:12], 3'h0, afu_csr_offset_8B_T1[5:0]};
//wire [14:0] feature_1_addr_offset_8B_T1 = {FEATURE_1_BEG[17:12], afu_csr_offset_8B_T1[8:0]};
reg  [1:0]  feature_id_T2;
always @(posedge Clk_400)
begin
        // -Stage T1-
        afu_csr_tid_T1 <= cp2cr_MmioHdr.tid;
        afu_csr_offset_8B_T1 <= afu_csr_addr_4B[15:1];

        if(cp2cr_MmioWrEn | cp2cr_MmioRdEn)
        begin
            afu_csr_length_4B_T1 <= afu_csr_length==2'b00;
            afu_csr_length_8B_T1 <= afu_csr_length==2'b01;
        end
        // DW enable is used when doing a 4B write
        case({afu_csr_length, afu_csr_addr_4B[0]})
            3'b000: begin afu_csr_dw_enable_T1 <= 2'b01;
                          afu_csr_wrdin_T1     <= cp2cr_MmioDin;
                    end
            3'b001: begin afu_csr_dw_enable_T1 <= 2'b10;
                          afu_csr_wrdin_T1     <= {cp2cr_MmioDin[31:0], cp2cr_MmioDin[31:0]};
                    end
            default:begin afu_csr_dw_enable_T1 <= 2'b11;
                          afu_csr_wrdin_T1     <= cp2cr_MmioDin;
                    end
        endcase

        afu_csr_wren_T1      <= 1'b0;
        afu_csr_rden_T1      <= 1'b0;
        if(ip_select)
        begin
            afu_csr_wren_T1 <= cp2cr_MmioWrEn;
            afu_csr_rden_T1 <= cp2cr_MmioRdEn;
        end

        // -Stage T2-
        afu_csr_dout_v_T2    <= afu_csr_rden_T1;
        afu_csr_dw_enable_T2 <= afu_csr_dw_enable_T1;
        afu_csr_length_4B_T2 <= afu_csr_length_4B_T1;
        afu_csr_length_8B_T2 <= afu_csr_length_8B_T1;
        afu_csr_tid_T2       <= afu_csr_tid_T1;

        // Read Feature 0 + addr offset
        afu_csr_dout_T2[0] <= csr_reg[feature_0_addr_offset_8B_T1];
        // Read Feature 1 + addr offset
//        afu_csr_dout_T2[1] <= csr_reg[feature_1_addr_offset_8B_T1];

        feature_id_T2 <= afu_csr_offset_8B_T1[10:9];

        // -Stage T3-
        afu_csr_dout_v_T3    <= afu_csr_dout_v_T2;
        afu_csr_dw_enable_T3 <= afu_csr_dw_enable_T2;
        afu_csr_length_8B_T3 <= afu_csr_length_8B_T2;
        afu_csr_tid_T3       <= afu_csr_tid_T2;

        case(feature_id_T2)
            2'h0    : afu_csr_dout_T3 <= afu_csr_dout_T2[0];
//            2'h1    : afu_csr_dout_T3 <= afu_csr_dout_T2[1];
            default : afu_csr_dout_T3 <= afu_csr_dout_T2[0];
            endcase

        // -Stage T4-
        case(afu_csr_dw_enable_T3)
            2'b10:  cr2cp_MmioDout <= afu_csr_dout_T3[63:32];
            default:cr2cp_MmioDout <= afu_csr_dout_T3;
        endcase
        cr2cp_MmioDout_v <= afu_csr_dout_v_T3;
        cr2cp_MmioHdr <= afu_csr_tid_T3;

        if(SoftReset)
        begin
            cr2cp_MmioDout_v <= 1'b0;
        end

        // AFH DFH Declarations:
        // The AFU-DFH must have the following mapping
        //      [63:60] 4'b0001
        //      [59:52] Rsvd
        //      [51:48] 4b User defined AFU mimor version #
        //      [47:41] Rsvd
        //      [40]    End of List
        //      [39:16] 24'h0 because no other DFHs
        //      [15:12] 4b User defined AFU major version #
        //      [11:0]  12'h001 CCI-P version #
        set_attr(CSR_AFH_DFH_BASE,
                 NO_STAGED_CSR,
                 1'b1,
                 {64{RW}},
                 {4'b0001,      // Type=AFU
                  8'h0, 
                  4'h0,         // AFU minor version #
                  7'h0,
                  END_OF_LIST,
                  NEXT_DFH_BYTE_OFFSET, 
                  4'h1,         // AFU major version #
                  CCIP_VERSION_NUMBER});    // CCI-P version #

        // The AFU ID
        set_attr(CSR_AFH_ID_L,
                 NO_STAGED_CSR,
                 1'b1,
                 {64{RO}},
                 NLB_AFU_ID_L);

        set_attr(CSR_AFH_ID_H,
                 NO_STAGED_CSR,
                 1'b1,
                 {64{RO}},
                 NLB_AFU_ID_H);
                                
       
        set_attr(CSR_DFH_RSVD0,
                 NO_STAGED_CSR,
                 1'b1,
                 {64{RsvdP}},
                 64'h0);

        // And set the Reserved AFU DFH 0x020 block to Reserved
        set_attr(CSR_DFH_RSVD1,
                 NO_STAGED_CSR,
                 1'b1,
                 {64{RsvdP}},
                 64'h0);

        // CSR Declarations
        // These are the parts of the CSR Register that are unique
        // for the NLB AFU.  They are not required for the FIU.
        // The are used by the SW that accesses this AFU.
         set_attr(CSR_SCRATCHPAD0,          // + CSR_SCRATCHPAD1
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RW}},
                  64'h0
                 );

         set_attr(CSR_SCRATCHPAD2,
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RW}},
                  64'h0
                 );


         set_attr(CSR_AFU_DSM_BASEL,        // + CSR_AFU_DSM_BASEH
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RW}},
                  64'h0
                 );

         if(SoftReset)
             cr2re_dsm_base_valid <= 1'b0;
         else if(afu_csr_wren_T1 
                && afu_csr_offset_8B_T1==CSR_AFU_DSM_BASEL[3+:L_CFG_SEG_SIZE] 
                && afu_csr_dw_enable_T1==2'b01 
                )
             cr2re_dsm_base_valid <= 1'b1;

         set_attr(CSR_SRC_ADDR,
                  NO_STAGED_CSR,
                  re2cr_wrlock_n,
                  {64{RW}},
                  64'h0
                 );

         set_attr(CSR_DST_ADDR,
                  NO_STAGED_CSR,
                  re2cr_wrlock_n,
                  {64{RW}},
                  64'h0
                 );

          set_attr(CSR_NUM_LINES,
                  NO_STAGED_CSR,
                  1'b1,
                  {
                   {44{RsvdP}},
                   {20{RW}}
                  },
                  64'h0
                 );

          set_attr(CSR_CTL,
                  NO_STAGED_CSR,
                  1'b1,
                  {{32{RW}},
                   {16{RsvdP}},
                   {16{RW}}
                  },
                  64'h0
                 );
          
                set_attr(CSR_STRIDE,
                   NO_STAGED_CSR,
                   1'b1,
                  {{58{RsvdP}},
                   {6{RW}}
                  },
                  64'h0
                 );

          set_attr(CSR_CFG,
                  NO_STAGED_CSR,
                  re2cr_wrlock_n,
                  {64{RW}},
                  64'h0
                 );

         set_attr(CSR_INACT_THRESH,
                  NO_STAGED_CSR,
                  re2cr_wrlock_n,
                  {64{RW}},
                  64'h0
                 );


         set_attr(CSR_SWTEST_MSG,
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RW}},
                  64'h0
                 );

         set_attr(CSR_STATUS0,
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RO}},
                  {re2cr_num_reads, re2cr_num_writes}
                 );

         set_attr(CSR_STATUS1,
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RO}},
                  {re2cr_num_Rdpend, re2cr_num_Wrpend}
                 );

         set_attr(CSR_ERROR,
                  NO_STAGED_CSR,
                  1'b1,
                  {64{RO}},
                  {32'h0, re2cr_error}
                 );

         if(SoftReset)
            cr2s1_csr_write <= 0;
        else
        begin
            if(  afu_csr_wren_T1 
              && afu_csr_offset_8B_T1==CSR_SWTEST_MSG[3+:L_CFG_SEG_SIZE]
              )
                cr2s1_csr_write <= 1'b1;
            else 
                cr2s1_csr_write <= 1'b0;
        end
end


//----------------------------------------------------------------------------------------------------------------------------------------------
task automatic set_attr; 
    input  [15:0]       csr_id;                           // byte aligned CSR address
    input  [15:0]       staged_csr_id;                    // byte aligned CSR address for late action staged register
    input               conditional_wr;                   // write condition for RW, RWS, RWDL attributes
    input  [3*64-1:0]   attr;                             // Attribute for each bit in the CSR
    input  [63:0]       default_val;                      // Initial value on Reset
    reg    [12:0]       csr_offset_8B;
    reg    [12:0]       staged_csr_offset_8B;
    reg    [1:0]        this_write;
    integer i,j;
    begin

        csr_offset_8B = csr_id[3+:L_CFG_SEG_SIZE];
        staged_csr_offset_8B = staged_csr_id[3+:L_CFG_SEG_SIZE];
        this_write[0] = afu_csr_wren_T1 && (csr_offset_8B==afu_csr_offset_8B_T1) && conditional_wr && afu_csr_dw_enable_T1[0];
        this_write[1] = afu_csr_wren_T1 && (csr_offset_8B==afu_csr_offset_8B_T1) && conditional_wr && afu_csr_dw_enable_T1[1];

        for(i=0; i<64; i=i+1)
        begin: foo
            if(i>31)
                j = 1'b1;
            else
                j = 1'b0;

            casex ({attr[i*3+:3]})
            RW: begin                                                   // - Read Write
                if(SoftReset)
                    csr_reg[csr_offset_8B][i]   <= default_val[i];
                else if(this_write[j])
                begin
                    csr_reg[csr_offset_8B][i]   <= afu_csr_wrdin_T1[i];
                end
            end

            RO: begin                                                   // - Read Only
                csr_reg[csr_offset_8B][i]      <= default_val[i];        // update status
            end

             /*RsvdZ*/ RsvdP: begin                                     // - Software must preserve these bits
                    csr_reg[csr_offset_8B][i]      <= default_val[i];    // set default value
            end

            endcase 
        end
    end
endtask

endmodule

