/* ****************************************************************************
 * Copyright(c) 2011-2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * **************************************************************************
 *
 * Module Info:
 * Language   : System{Verilog} | C/C++
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * ASE generics (SystemVerilog header file)
 *
 * Description:
 * This file contains definitions and parameters for the DPI
 * module. The intent of this file is that the user should not modify
 * the DPI source files. **Only** this header file must be modified if
 * any DPI parameters need to be changed.
 *
 */

package ase_pkg;

   import ccip_if_pkg::*;

   // ASE modes
   parameter ASE_MODE_DAEMON          = 1;
   parameter ASE_MODE_TIMEOUT_SIMKILL = 2;
   parameter ASE_MODE_SW_SIMKILL      = 3;
   parameter ASE_MODE_REGRESSION      = 4;


   // Include platform.vh if not already
 `ifndef _PLATFORM_VH_
  `include "platform.vh"
 `endif

   // Address widths
   parameter PHYSCLADDR_WIDTH   =  42;


   /*
    * CCI specifications
    */
   parameter CCIP_DATA_WIDTH       = 512;
   parameter CCIP_CFG_RDDATA_WIDTH = 64;

   /*
    * Sub-structures
    * Request type, response types, VC types
    */
   // Request types {channel_id, ccip_if_pkg::req_type}
   typedef enum logic [3:0] {
 `ifdef ASE_ENABLE_INTR_FEATURE
			     ASE_INTR_REQ   = 4'h7,
 `endif
			     ASE_RDLINE_S   = 4'h1,
			     ASE_RDLINE_I   = 4'h2,
			     ASE_WRLINE_I   = 4'h3,
			     ASE_WRLINE_M   = 4'h4,
			     ASE_WRPUSH     = 4'h5,
			     ASE_WRFENCE    = 4'h6
			     // ASE_ATOMIC_REQ = 4'h8
			     } ccip_reqtype_t;

   // Response types
   typedef enum logic [3:0] {
 `ifdef ASE_ENABLE_INTR_FEATURE
			     ASE_INTR_RSP    = 4'h3,
 `endif
 `ifdef ASE_ENABLE_UMSG_FEATURE
			     ASE_UMSG        = 4'h6,
 `endif
			     ASE_RD_RSP      = 4'h1,
			     ASE_WR_RSP      = 4'h2,
			     ASE_WRFENCE_RSP = 4'h4
			     // ASE_ATOMIC_RSP  = 4'h5,
			     } ccip_resptype_t;

   // Virtual channel type
   typedef enum logic [1:0] {
			     VC_VA  = 2'b00,
			     VC_VL0 = 2'b01,
			     VC_VH0 = 2'b10,
			     VC_VH1 = 2'b11
			     } ccip_vc_t;

   // Length type
   typedef enum logic [1:0] {
			     ASE_1CL = 2'b00,
			     ASE_2CL = 2'b01,
			     ASE_3CL = 2'b10,
			     ASE_4CL = 2'b11
			     } ccip_len_t;


   /* ***********************************************************
    * CCI-P headers
    * RxHdr, TxHdr, CCIP Packets
    * ***********************************************************/
   // RxHdr
   typedef struct packed {
      //--------- CCIP standard header --------- //
      ccip_vc_t       vc_used;  // 27:26  // Virtual channel select
      logic           rsvd25;   // 25     // Poison bit
      logic           hitmiss;  // 24     // Hit/miss indicator
      logic           format;   // 23     // Multi-CL enable (write packing only)
      logic           rsvd22;   // 22     // X
      ccip_len_t      clnum;    // 21:20  // Cache line number
      ccip_resptype_t resptype; // 19:16  // Response type
      logic [15:0]    mdata;    // 15:0   // Metadata
   } RxHdr_t;
   parameter CCIP_RX_HDR_WIDTH     = $bits(RxHdr_t);

   typedef logic [CCIP_RX_HDR_WIDTH-1:0] logic_cast_RxHdr_t;


   // TxHdr
   typedef struct packed {
      //--------- CCIP standard header --------- //
      t_ccip_clByteIdx       byte_len;    // 79:74
      ccip_vc_t              vc;          // 73:72  // Virtual channel select
      logic                  sop;         // 71     // Start of packet
      t_ccip_mem_access_mode mode;        // 70
      ccip_len_t             len;         // 69:68  // Length
      ccip_reqtype_t         reqtype;     // 67:64  // Request Type
      t_ccip_clByteIdx       byte_start;  // 63:58
      logic [41:0]           addr;        // 57:16  // Address
      logic [15:0]           mdata;       // 15:0   // Metadata
   } TxHdr_t;
   parameter CCIP_TX_HDR_WIDTH     = $bits(TxHdr_t);

   typedef logic [CCIP_TX_HDR_WIDTH-1:0] logic_cast_TxHdr_t;


   /*
    * Config MMIO Header
    */
   // MMIO Header specifics
   parameter int      CCIP_CFGHDR_ADDR_WIDTH  = 18;
   parameter int      CCIP_CFGHDR_INDEX_WIDTH = 16;
   parameter int      CCIP_CFGHDR_TID_WIDTH   = 9;

   // CfgHdr
   typedef struct packed {
      logic [CCIP_CFGHDR_INDEX_WIDTH-1:0] index;  // 27:12
      logic [1:0] 			  len;    // 11:10
      logic 				  rsvd9;  // 9
      logic [CCIP_CFGHDR_TID_WIDTH-1:0]   tid;    // 8:0
      } CfgHdr_t;
   parameter CCIP_CFG_HDR_WIDTH    = $bits(CfgHdr_t);

   typedef logic [CCIP_CFG_HDR_WIDTH-1:0] logic_cast_CfgHdr_t;


   // MMIO header
   typedef struct packed {
      logic [8:0] tid;
      } MMIOHdr_t;
   parameter CCIP_MMIO_TID_WIDTH    = $bits(MMIOHdr_t);

   typedef logic [CCIP_MMIO_TID_WIDTH-1:0] logic_cast_MMIOHdr_t;


   /*
    * Umsg header (received when UMsg is received)
    * Enabled only for integrated
    */
 `ifdef ASE_ENABLE_UMSG_FEATURE
   typedef struct 			   packed {
      logic [1:0] rsvd_27_26;  // 27:26 // Reserved
      logic 	  rsvd25;      // 25    // Poison bit
      logic [4:0] rsvd_24_20;  // 24:20 // Reserved
      logic [3:0] resp_type;   // 19:16 // Response type
      logic       umsg_type;   // 15    // Umsg type
      logic [8:0] rsvd_14_6;   // 14:6  // Reserved
      logic [5:0] umsg_id;     // 5:0   // Umsg Id
   } UMsgHdr_t;
   parameter ASE_UMSG_HDR_WIDTH    = $bits(UMsgHdr_t);

   typedef logic [ASE_UMSG_HDR_WIDTH-1:0] logic_cast_UMsgHdr_t;
 `endif


   // CmpXchg header (received from a Compare-Exchange operation)
   // typedef struct packed {
   //    ccip_vc_t       vc_used;    // 27:26
   //    logic 	      rsvd25;     // 25
   //    logic 	      hitmiss;    // 24
   //    logic 	      success;    // 23
   //    logic [2:0]     rsvd_22_20; // 22:20
   //    ccip_resptype_t resptype;   // 19:16
   //    logic [15:0]    mdata;      // 15:0
   // } Atomics_t;
   // parameter CCIP_CMPXCHG_HDR_WIDTH = $bits(Atomics_t);

   // Config channel
   parameter CCIP_MMIO_ADDR_WIDTH   = 16;
   parameter CCIP_MMIO_INDEX_WIDTH  = 14;
   parameter CCIP_MMIO_RDDATA_WIDTH = 64;


   /*
    * Interrupt request and response headers specific to ASE
    */
 // `ifdef ASE_ENABLE_INTR_FEATURE
 //   // Interrupt request header
 //   typedef struct     packed {
 //      logic [11:0]    rsvd_79_68; // 79:68 // Reserved
 //      ccip_reqtype_t  req_type;   // 67:64 // Type
 //      logic [60:0]    rsvd_63_3;  // 63:3  // reserved
 //      logic [2:0]     id;         // 2:0   // Intr vector
 //   } IntrReq_t;

 //   // Interrupt response header
 //   typedef struct     packed {
 //      logic [7:0]     rsvd1;      // 27:20 // reserved, don't care
 //      ccip_resptype_t resp_type;  // 19:16 // Response type
 //      logic [12:0]    rsvd_15_3;  // 15:3  // reserved, don't care
 //      logic [2:0]     id;         // 2:0   // Vector
 //   } IntrRsp_t;
 // `endif


   /*
    * Wrapped headers with channel Id
    * ASE's internal datatype used for bookkeeping
    */
   // Wrap TxHdr_t with channel_id
   typedef struct packed {
      logic 	  channel_id;
      TxHdr_t     txhdr;
   } ASETxHdr_t;
   parameter ASE_TX_HDR_WIDTH     = $bits(ASETxHdr_t);

   // Wrap RxHdr_t with channel_id
   typedef struct packed {
      logic 	  channel_id;
      RxHdr_t     rxhdr;
   } ASERxHdr_t;
   parameter ASE_RX_HDR_WIDTH     = $bits(ASERxHdr_t);


   /*
    * FIFO depth bit-width
    * Enter 'n' here, where n = log_2(FIFO_DEPTH) & n is an integer
    */
   parameter ASE_FIFO_DEPTH_NUMBITS = 8;


   /*
    * Latency Scoreboard generics
    */
   // Number of transactions in latency scoreboard
   parameter LATBUF_NUM_TRANSACTIONS = 32;
   // Radix of latency scoreboard radix
   parameter LATBUF_COUNT_WIDTH      = $clog2(LATBUF_NUM_TRANSACTIONS) + 1;
   // ASE_fifo full threshold inside latency scoreboard
   parameter LATBUF_FULL_THRESHOLD   = LATBUF_NUM_TRANSACTIONS - 5;
   // Radix of ASE_fifo (subcomponent in latency scoreboard)
   parameter LATBUF_DEPTH_BASE2      = $clog2(LATBUF_NUM_TRANSACTIONS);
   // Wait station timer width
   parameter LATBUF_TIMER_WIDTH      = 9;
   // Latency buffer TID width
   parameter LATBUF_TID_WIDTH        = 32;

   // ASE Response FIFO specifics
   parameter ASE_RSPFIFO_DEPTH           = 256;
   parameter ASE_RSPFIFO_COUNT_WIDTH     = $clog2(ASE_RSPFIFO_DEPTH);
   parameter ASE_RSPFIFO_ALMFULL_THRESH  = ASE_RSPFIFO_DEPTH - 10;


   /*
    * CCI Transaction packet
    */
   typedef struct {
      int 	  mode;
      int 	  qw_start;
      int 	  mdata;
      int         intr_id;
      longint 	  cl_addr;
      longint     qword[8];
      int 	  resp_channel;
      int 	  success;

      int         byte_en;         // Access limited to byte range within line when non-zero
      int         byte_start;      // Index of first byte update
      int         byte_len;        // Number of bytes to update, starting with byte_start
   } cci_pkt;

   parameter CCIPKT_WRITE_MODE   = 32'h1010;
   parameter CCIPKT_READ_MODE    = 32'h2020;
   parameter CCIPKT_WRFENCE_MODE = 32'hFFFF;
   parameter CCIPKT_ATOMIC_MODE  = 32'h8080;
   parameter CCIPKT_INTR_MODE    = 32'h4040;


   /*
    * ASE config structure
    * This will reflect ase.cfg
    */
   typedef struct {
      int         ase_mode;
      int 	  ase_timeout;
      int 	  ase_num_tests;
      int 	  enable_reuse_seed;
      int 	  ase_seed;
      int 	  enable_cl_view;
      int 	  usr_tps;
      int 	  phys_memory_available_gb;
   } ase_cfg_t;
   ase_cfg_t cfg;

   /*
    * MMIO packet
    */
   typedef struct {
      int 	  tid;
      int 	  write_en;
      int 	  width;
      int 	  addr;
      longint 	  qword[8];
      int 	  resp_en;
      } mmio_t;

   // Request types
   parameter int  MMIO_WRITE_REQ    = 32'hAA88;
   parameter int  MMIO_READ_REQ     = 32'hBB88;

   // Length
   parameter int  MMIO_WIDTH_32 = 32;
   parameter int  MMIO_WIDTH_64 = 64;


   /*
    * UMSG Hint/Data state machine
    * - Data structure defined to control UMsg behavior
    * Enabled only for integrated configuration
    */
 `ifdef ASE_ENABLE_UMSG_FEATURE
   // Number of UMSGs per AFU
   parameter int  NUM_UMSG_PER_AFU = 8;

   // Umsg command packet
   typedef struct {
      int 	  id;
      int 	  hint;
      longint 	  qword[8] 		;
   } umsgcmd_t;


   // UMSG control states
   typedef enum   {UMsgIdle, UMsgHintWait, UMsgSendHint, UMsgDataWait, UMsgSendData}
		  UMsg_StateEnum;

   // UMSG control structure
   typedef struct {
      logic [`UMSG_DELAY_TIMER_LOG2-1:0] hint_timer;
      logic [`UMSG_DELAY_TIMER_LOG2-1:0] data_timer;
      logic 				 line_accessed;
      logic 				 hint_enable;
      logic 				 hint_ready;
      logic 				 hint_pop;
      logic 				 data_ready;
      logic 				 data_pop;
      UMsg_StateEnum                     state;
   } umsg_t;

 `endif


   /*
    * FUNCTION: Unpack qwords[0:7]       to data vector
    */
   function logic [CCIP_DATA_WIDTH-1:0] unpack_ccipkt_to_vector (input cci_pkt pkt);
      logic [CCIP_DATA_WIDTH-1:0] 	 ret;
      int 				 i;
      begin
   	 ret[  63:00  ] = pkt.qword[0] ;
   	 ret[ 127:64  ] = pkt.qword[1] ;
   	 ret[ 191:128 ] = pkt.qword[2] ;
   	 ret[ 255:192 ] = pkt.qword[3] ;
   	 ret[ 319:256 ] = pkt.qword[4] ;
   	 ret[ 383:320 ] = pkt.qword[5] ;
   	 ret[ 447:384 ] = pkt.qword[6] ;
   	 ret[ 511:448 ] = pkt.qword[7] ;
   	 return ret;
      end
   endfunction

   /*
    * FUNCTION: Pack data vector into qwords[0:7]
    */
   // function automatic void pack_vector_to_ccipkt (input [511:0] vec,
   // 						  ref cci_pkt pkt);
   //    begin
   // 	 pkt.qword[0] =  vec[  63:00 ];
   // 	 pkt.qword[1] =  vec[ 127:64  ];
   // 	 pkt.qword[2] =  vec[ 191:128 ];
   // 	 pkt.qword[3] =  vec[ 255:192 ];
   // 	 pkt.qword[4] =  vec[ 319:256 ];
   // 	 pkt.qword[5] =  vec[ 383:320 ];
   // 	 pkt.qword[6] =  vec[ 447:384 ];
   // 	 pkt.qword[7] =  vec[ 511:448 ];
   //    end
   // endfunction


   /*
    * FUNCTION: conv_gbsize_to_num_bytes
    * Converts GB size to num_bytes
    */
   function automatic longint conv_gbsize_to_num_bytes(int gb_size);
      begin
	 return (gb_size*1024*1024*1024);
      end
   endfunction


   /*
    * FUNCTION: Return absolute value
    */
   function automatic int abs_val(int num);
      begin
	 return (num < 0) ? ~num : num;
      end
   endfunction


   /*
    * CCI-P package specific request/response type check functions
    * ------------------------------------------------------------
    * - These functions are meant only for CCI-P header types
    */
   // Is a Read Request
   function logic isCCIPRdLineRequest(t_ccip_c0_req req);
      begin
	 if ((req == eREQ_RDLINE_I)||(req == eREQ_RDLINE_S))
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Write Request
   function logic isCCIPWrLineRequest(t_ccip_c1_req req);
      begin
	 if ((req == eREQ_WRLINE_I)||(req == eREQ_WRLINE_M)||(req == eREQ_WRPUSH_I))
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Write Fence Request
   function logic isCCIPWrFenceRequest(t_ccip_c1_req req);
      begin
	 if (req == eREQ_WRFENCE)
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Intr Request
   function logic isCCIPIntrRequest(t_ccip_c1_req req);
      begin
	 if (req == eREQ_INTR)
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Read Response
   function logic isCCIPRdLineResponse(t_ccip_c0_rsp rsp);
      begin
	 if (rsp == eRSP_RDLINE)
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Umsg Response (integrated only)
`ifdef ASE_ENABLE_UMSG_FEATURE
   function logic isCCIPUmsgResponse(t_ccip_c0_rsp rsp);
      begin
	 if (rsp == eRSP_UMSG)
	   return 1;
	 else
	   return 0;
      end
   endfunction
`endif

   // Is a Write Response
   function logic isCCIPWrLineResponse(t_ccip_c1_rsp rsp);
      begin
	 if (rsp == eRSP_WRLINE)
	   return 1;
	 else
	   return 0;
      end
   endfunction

   // Is a Write Fence Response
   function logic isCCIPWrFenceResponse(t_ccip_c1_rsp rsp);
      begin
	 if (rsp == eRSP_WRFENCE)
	   return 1;
	 else
	   return 0;
      end
   endfunction

   /*
    * Interrupt functions, enabled on discrete only
    */
`ifdef ASE_ENABLE_INTR_FEATURE
   // isCCIPInterruptRequest
   function automatic logic isCCIPInterruptRequest(t_ccip_c1_req req);
      begin
	 if (req == eREQ_INTR)
	   return 1;
	 else
	   return 0;
      end
   endfunction // isInterruptRequest

   // isCCIPInterruptResponse
   function automatic logic isCCIPInterruptResponse(t_ccip_c1_rsp resp);
      begin
	 if (resp == eRSP_INTR)
	   return 1;
	 else
	   return 0;
      end
   endfunction // isInterruptResponse

`endif


   /*
    * ASE Read/Write Request/Response type checks
    * --------------------------------------------
    * - These functions can only by ASE specific headers
    *
    */
   // isReadRequest
   function automatic logic isReadRequest(TxHdr_t hdr);
      begin
	 if ((hdr.reqtype == ASE_RDLINE_I)||(hdr.reqtype == ASE_RDLINE_S)) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isReadRequest

   // isReadResponse
   function automatic logic isReadResponse(RxHdr_t hdr);
      begin
	 if (hdr.resptype == ASE_RD_RSP ) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isReadResponse

   // isWriteRequest
   function automatic logic isWriteRequest(TxHdr_t hdr);
      begin
	 if ((hdr.reqtype == ASE_WRLINE_I)||(hdr.reqtype == ASE_WRLINE_M)||(hdr.reqtype == ASE_WRPUSH)) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isWriteRequest

   // isWriteResponse
   function automatic logic isWriteResponse(RxHdr_t hdr);
      begin
	 if (hdr.resptype == ASE_WR_RSP ) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isWriteResponse

   // isWrFenceRequest
   function automatic logic isWrFenceRequest(TxHdr_t hdr);
      begin
	 if (hdr.reqtype == ASE_WRFENCE) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isWrFenceRequest

   // isWrFenceResponse
   function automatic logic isWrFenceResponse(RxHdr_t hdr);
      begin
	 if (hdr.resptype == ASE_WRFENCE_RSP) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction // isWrFenceResponse

   // isIntrRequest
   function automatic logic isIntrRequest(TxHdr_t hdr);
      begin
`ifdef ASE_ENABLE_INTR_FEATURE
	 if (hdr.reqtype == ASE_INTR_REQ) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
`else
	 return 0;
`endif
      end
   endfunction

   // isIntrResponse
   function automatic logic isIntrResponse(RxHdr_t hdr);
      begin
`ifdef ASE_ENABLE_INTR_FEATURE
	 if (hdr.resptype == ASE_INTR_RSP) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
`else
	 return 0;
`endif
      end
   endfunction


   // ------------------------------------------- //
   // Virtual channel ease functions
   // ------------------------------------------- //
   // isVL0Request
   function automatic logic isVL0Request(TxHdr_t hdr);
      begin
	 if (hdr.vc == VC_VL0) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVHxRequest
   function automatic logic isVHxRequest(TxHdr_t hdr);
      begin
	 if ((hdr.vc == VC_VH0)||(hdr.vc == VC_VH1)) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVH0Request
   function automatic logic isVH0Request(TxHdr_t hdr);
      begin
	 if (hdr.vc == VC_VH0) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVH1Request
   function automatic logic isVH1Request(TxHdr_t hdr);
      begin
	 if (hdr.vc == VC_VH1) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVARequest
   function automatic logic isVARequest(TxHdr_t hdr);
      begin
	 if (hdr.vc == VC_VA) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVL0Response
   function automatic logic isVL0Response(RxHdr_t hdr);
      begin
	 if (hdr.vc_used == VC_VL0) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVHxResponse
   function automatic logic isVHxResponse(RxHdr_t hdr);
      begin
	 if ((hdr.vc_used == VC_VH0)||(hdr.vc_used == VC_VH1)) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVH0Response
   function automatic logic isVH0Response(RxHdr_t hdr);
      begin
	 if (hdr.vc_used == VC_VH0) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction

   // isVH1Response
   function automatic logic isVH1Response(RxHdr_t hdr);
      begin
	 if (hdr.vc_used == VC_VH1) begin
	    return 1;
	 end
	 else begin
	    return 0;
	 end
      end
   endfunction


   /*
    * Pretty print TXHdr & RxHdr
    */
   // Print channel (Debug)
   function string ase_channel_type (ccip_vc_t vc_sel);
      begin
	 case (vc_sel)
	   VC_VA  : return "VA ";
	   VC_VL0 : return "VL0";
	   VC_VH0 : return "VH0";
	   VC_VH1 : return "VH1";
	 endcase
      end
   endfunction // ase_channel_type
   
   // Print clnum (Debug)
   function string ase_print_clnum (ccip_len_t num);
      begin
	 case (num)
	   ASE_1CL: return "#1CL";
	   ASE_2CL: return "#2CL";
	   ASE_3CL: return "#3CL";
	   ASE_4CL: return "#4CL";
	 endcase
      end
   endfunction // ase_print_clnum
   
   // Print pack status (Debug)
   function string ase_pack_status(logic rx_fmt);
      begin
	 case (rx_fmt)
	   0 : return "nopack";
	   1 : return "pack";
	 endcase
      end
   endfunction // ase_pack_status

   // Print Reqtype (Debug)
   function string ase_print_reqtype(ccip_reqtype_t req);
      begin
	 case (req)
`ifdef ASE_ENABLE_INTR_FEATURE
	   ASE_INTR_REQ: return "Intr";
`endif
	   ASE_RDLINE_S : return "RdS";	 
	   ASE_RDLINE_I : return "RdI";	 
	   ASE_WRLINE_I : return "WrI";	 
	   ASE_WRLINE_M : return "WrM";	 
	   ASE_WRPUSH   : return "WrP";	 
	   ASE_WRFENCE  : return "WrF";	 
	   default : return "**FAIL**";
	 endcase
      end
   endfunction // ase_print_reqtype
      
   // Print Resptype (Debug)
   function string ase_print_resptype(ccip_resptype_t resp);
      begin
	 case (resp)
`ifdef ASE_ENABLE_INTR_FEATURE
	   ASE_INTR_RSP   : return "Intr";
`endif
`ifdef ASE_ENABLE_UMSG_FEATURE
 	   ASE_UMSG       : return "UMsg";
`endif
	   ASE_RD_RSP     : return "Rd";
	   ASE_WR_RSP     : return "Wr";
	   ASE_WRFENCE_RSP: return "WrF";
	   default        : return "**FAIL**";
	 endcase
      end
   endfunction
   
   // TxHdr print
   function automatic string return_txhdr(TxHdr_t hdr);
      string 			  str;
      begin
	 $sformat(str, "TxHdr = {%s,%s,%s,%x,%04x}", ase_print_reqtype(hdr.reqtype), ase_print_clnum(hdr.len), ase_channel_type(hdr.vc), hdr.addr, hdr.mdata);
	 return str;
      end
   endfunction

   // RxHdr print
   function automatic string return_rxhdr(RxHdr_t hdr);
      string str;
      begin
	 $sformat(str, "RxHdr = {%s,%s,%s,%04x,%s}", ase_print_resptype(hdr.resptype), ase_print_clnum(hdr.clnum), ase_channel_type(hdr.vc_used), hdr.mdata, ase_pack_status(hdr.format));
	 return str;
      end
   endfunction


   /*
    * Transaction count management
    */
   // VC Count struct
   typedef struct packed {
      int 	  va;
      int 	  vl0;
      int 	  vh0;
      int 	  vh1;
      } txn_vc_counts;

   // MCL count struct
   typedef struct packed {
      int 	  mcl0;
      int 	  mcl1;
      int 	  mcl3;
      } txn_mcl_counts;


   /*
    * Hazard checker interface
    */
   // Hazard event packet
   typedef struct packed {
      logic [LATBUF_TID_WIDTH-1:0] tid;
      logic 			   valid;
      TxHdr_t                      hdr;
      } ase_haz_pkt;


   // Unified interface for read/write insert/delete
   typedef struct packed {
      ase_haz_pkt read_in;
      ase_haz_pkt read_out;
      ase_haz_pkt write_in;
      ase_haz_pkt write_out;
      } ase_haz_if;


   /*
    * ASE protocol sniff codes
    */
   // parameter SNIFF_CODE_WIDTH = 5;
   parameter SNIFF_VECTOR_WIDTH = 36;
 // 2**SNIFF_CODE_WIDTH;

   // Error code indices
   typedef enum   {
		   SNIFF_NO_ERROR                = 0,
		   // ------------ C2TX -------------- //
		   MMIO_RDRSP_TIMEOUT            = 1,
		   MMIO_RDRSP_UNSOLICITED        = 2,
		   MMIO_RDRSP_RESET_IGNORED_WARN = 3,
		   MMIO_RDRSP_XZ_FOUND_WARN      = 4,
		   // ------------ C0TX ------------ //
		   SNIFF_C0TX_INVALID_REQTYPE    = 5,
		   SNIFF_C0TX_OVERFLOW           = 6,
		   SNIFF_C0TX_ADDRALIGN_2_ERROR  = 7,
		   SNIFF_C0TX_ADDRALIGN_4_ERROR  = 8,
		   SNIFF_C0TX_RESET_IGNORED_WARN = 9,
		   SNIFF_C0TX_XZ_FOUND_WARN      = 10,
		   SNIFF_C0TX_3CL_REQUEST        = 11,
		   SNIFF_C0TX_ADDR_ZERO_WARN     = 12,
		   SNIFF_C0TX_UNEXP_ADDR         = 13,
		   // ------------ C1TX -------------- //
		   SNIFF_C1TX_INVALID_REQTYPE    = 14,
		   SNIFF_C1TX_OVERFLOW           = 15,
		   SNIFF_C1TX_ADDRALIGN_2_ERROR  = 16,
		   SNIFF_C1TX_ADDRALIGN_4_ERROR  = 17,
		   SNIFF_C1TX_RESET_IGNORED_WARN = 18,
		   SNIFF_C1TX_XZ_FOUND_WARN      = 19,
		   SNIFF_C1TX_UNEXP_VCSEL        = 20,
		   SNIFF_C1TX_UNEXP_MDATA        = 21,
		   SNIFF_C1TX_UNEXP_ADDR         = 22,
		   SNIFF_C1TX_UNEXP_CLLEN        = 23,
		   SNIFF_C1TX_UNEXP_REQTYPE      = 24,
		   SNIFF_C1TX_SOP_NOT_SET        = 25,
		   SNIFF_C1TX_SOP_SET_MCL1TO3    = 26,
		   SNIFF_C1TX_3CL_REQUEST        = 27,
		   SNIFF_C1TX_WRFENCE_IN_MCL1TO3 = 28,
		   SNIFF_C1TX_ADDR_ZERO_WARN     = 29,
		   SNIFF_C1TX_WRFENCE_SOP_SET	 = 30,
		   SNIFF_C1TX_BYTE_EN_NON_WRITE	 = 31,
		   SNIFF_C1TX_BYTE_EN_MULTI_LINE = 32,
		   SNIFF_C1TX_BYTE_EN_BAD_RANGE  = 33,
		   SNIFF_C1TX_BYTE_EN_NOT_IMPL   = 34,
		   SNIFF_C1TX_BYTE_EN_WRONG_MODE = 35
		   // --------------------------------- //
		   } sniff_code_t;

   /*
    * outoforder_wrf_channel Transaction checker block
    */
 `ifdef ASE_DEBUG
   typedef struct packed {
      logic [0:3] 		   rxout_valid;
      ccip_vc_t                    virt_channel;
      } ccip_txn_t;
 `endif

   /*
    * Other Macros
    */
   // Get random number from range
 `define get_random_from_range(low, high)\
   ($random() % (high + 1 - low) + low)


endpackage
