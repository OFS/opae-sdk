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
 * Module Info: ASE top-level
 *              (hides ASE machinery, makes finding cci_std_afu easy)
 * Language   : System{Verilog}
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * **************************************************************************/

import ccip_if_pkg::*;

`timescale 1ns/1ns

module ase_top();


   logic pClkDiv4;
   logic pClkDiv2;
   logic pClk;
   logic pck_cp2af_softReset;

   logic uClk_usr;
   logic uClk_usrDiv2;

   t_if_ccip_Tx pck_af2cp_sTx;
   t_if_ccip_Rx pck_cp2af_sRx;

   logic [1:0] pck_cp2af_pwrState;   // CCI-P AFU Power State
   logic       pck_cp2af_error;      // CCI-P Protocol Error Detected

`ifdef FPGA_PLATFORM_DISCRETE
   wire                ddr4a_clk;

   wire                ddr4a_waitrequest_n;
   wire [511:0]        ddr4a_readdata;
   wire                ddr4a_readdatavalid;
   wire [6:0]          ddr4a_burstcount;
   wire [511:0]        ddr4a_writedata;
   wire [25:0]         ddr4a_address;
   wire                ddr4a_write;
   wire                ddr4a_read;
   wire [63:0]         ddr4a_byteenable;
   wire [1:0]          DDR4a_response;
   wire                DDR4a_writeresponsevalid;

   wire                ddr4b_waitrequest_n;
   wire [511:0]        ddr4b_readdata;
   wire                ddr4b_readdatavalid;
   wire [6:0]          ddr4b_burstcount;
   wire [511:0]        ddr4b_writedata;
   wire [25:0]         ddr4b_address;
   wire                ddr4b_write;
   wire                ddr4b_read;
   wire [63:0]         ddr4b_byteenable;
   wire [1:0]          ddr4b_response;
   wire                ddr4b_writeresponsevalid;
`endif

   // CCI-P emulator
   ccip_emulator ccip_emulator
     (
      .pClkDiv4               (pClkDiv4            ),
      .pClkDiv2               (pClkDiv2            ),
      .pClk                   (pClk                ),
      .uClk_usr               (uClk_usr            ),
      .uClk_usrDiv2           (uClk_usrDiv2        ),
      .pck_cp2af_softReset    (pck_cp2af_softReset ),
      .pck_cp2af_pwrState     (pck_cp2af_pwrState  ),
      .pck_cp2af_error        (pck_cp2af_error     ),
      .pck_af2cp_sTx	      (pck_af2cp_sTx       ),
      .pck_cp2af_sRx          (pck_cp2af_sRx       )
      );


   // CCIP AFU
   ccip_std_afu ccip_std_afu
     (
      .pClkDiv4               (pClkDiv4            ),
      .pClkDiv2               (pClkDiv2            ),
      .pClk                   (pClk                ),
      .uClk_usr               (uClk_usr            ),
      .uClk_usrDiv2           (uClk_usrDiv2        ),
      .pck_cp2af_softReset    (pck_cp2af_softReset ),
      .pck_cp2af_pwrState     (pck_cp2af_pwrState  ),
      .pck_cp2af_error        (pck_cp2af_error     ),
`ifdef FPGA_PLATFORM_DISCRETE
      .DDR4_USERCLK           (ddr4a_clk           ),
      .DDR4a_writedata        (ddr4a_writedata     ),
      .DDR4a_readdata         (ddr4a_readdata      ),
      .DDR4a_address          (ddr4a_address       ),
      .DDR4a_waitrequest      (~ddr4a_waitrequest_n),
      .DDR4a_write            (ddr4a_write         ),
      .DDR4a_read             (ddr4a_read          ),
      .DDR4a_byteenable       (ddr4a_byteenable    ),
      .DDR4a_burstcount       (ddr4a_burstcount    ),
      .DDR4a_readdatavalid    (ddr4a_readdatavalid ),

      .DDR4b_writedata        (ddr4b_writedata     ),
      .DDR4b_readdata         (ddr4b_readdata      ),
      .DDR4b_address          (ddr4b_address       ),
      .DDR4b_waitrequest      (~ddr4b_waitrequest_n),
      .DDR4b_write            (ddr4b_write         ),
      .DDR4b_read             (ddr4b_read          ),
      .DDR4b_byteenable       (ddr4b_byteenable    ),
      .DDR4b_burstcount       (ddr4b_burstcount    ),
      .DDR4b_readdatavalid    (ddr4b_readdatavalid ),
`endif
      .pck_af2cp_sTx	      (pck_af2cp_sTx       ),
      .pck_cp2af_sRx          (pck_cp2af_sRx       )
      );

   // t_ccip_c0_RspAtomicHdr DBG_C0RxAtomic;
   // assign DBG_C0RxAtomic = t_ccip_c0_RspAtomicHdr'(pck_cp2af_sRx.c0.hdr);

`ifdef FPGA_PLATFORM_DISCRETE
   // emif model
   ed_sim ed_sim (
      .ddr4a_waitrequest_n    (ddr4a_waitrequest_n),
      .ddr4a_read             (ddr4a_read         ),
      .ddr4a_write            (ddr4a_write        ),
      .ddr4a_address          (ddr4a_address      ),
      .ddr4a_readdata         (ddr4a_readdata     ),
      .ddr4a_writedata        (ddr4a_writedata    ),
      .ddr4a_burstcount       (ddr4a_burstcount   ),
      .ddr4a_byteenable       (ddr4a_byteenable   ),
      .ddr4a_readdatavalid    (ddr4a_readdatavalid),
      .ddr4a_userclk_clk      (ddr4a_clk          ),

      .ddr4b_waitrequest_n    (ddr4b_waitrequest_n),
      .ddr4b_read             (ddr4b_read         ),
      .ddr4b_write            (ddr4b_write        ),
      .ddr4b_address          (ddr4b_address      ),
      .ddr4b_readdata         (ddr4b_readdata     ),
      .ddr4b_writedata        (ddr4b_writedata    ),
      .ddr4b_burstcount       (ddr4b_burstcount   ),
      .ddr4b_byteenable       (ddr4b_byteenable   ),
      .ddr4b_readdatavalid    (ddr4b_readdatavalid)
   );
`endif

   t_ccip_c0_ReqMmioHdr DBG_C0RxMMIO;
   assign DBG_C0RxMMIO  = t_ccip_c0_ReqMmioHdr'(pck_cp2af_sRx.c0.hdr);

endmodule // ase_top
