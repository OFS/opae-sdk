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
// Module Name :	  ccip_std_afu
// Project :        ccip afu top 
// Description :    This module instantiates CCI-P compliant AFU

// ***************************************************************************
`default_nettype none
import ccip_if_pkg::*;
module ccip_std_afu(
  // CCI-P Clocks and Resets
  pClk,                      // 400MHz - CCI-P clock domain. Primary interface clock
  pClkDiv2,                  // 200MHz - CCI-P clock domain.
  pClkDiv4,                  // 100MHz - CCI-P clock domain.
  uClk_usr,                  // User clock domain.  **
  uClk_usrDiv2,              // User clock domain. 
  pck_cp2af_softReset,       // CCI-P ACTIVE HIGH Soft Reset
  pck_cp2af_pwrState,        // CCI-P AFU Power State
  pck_cp2af_error,           // CCI-P Protocol Error Detected

  // Interface structures
  pck_cp2af_sRx,             // CCI-P Rx Port
  pck_af2cp_sTx              // CCI-P Tx Port
);
  input           wire             pClk;                     // 400MHz - CCI-P clock domain. Primary interface clock
  input           wire             pClkDiv2;                 // 200MHz - CCI-P clock domain.
  input           wire             pClkDiv4;                 // 100MHz - CCI-P clock domain.
  input           wire             uClk_usr;                 // User clock domain.
  input           wire             uClk_usrDiv2;             // User clock domain.
  input           wire             pck_cp2af_softReset;      // CCI-P ACTIVE HIGH Soft Reset
  input           wire [1:0]       pck_cp2af_pwrState;       // CCI-P AFU Power State
  input           wire             pck_cp2af_error;          // CCI-P Protocol Error Detected

  // Interface structures
  input           t_if_ccip_Rx     pck_cp2af_sRx;           // CCI-P Rx Port
  output          t_if_ccip_Tx     pck_af2cp_sTx;           // CCI-P Tx Port

// =============================================================
// Register SR <--> PR signals at interface before consuming it
// =============================================================

(* noprune *) logic [1:0]  pck_cp2af_pwrState_T1;
(* noprune *) logic        pck_cp2af_error_T1;

logic        pck_cp2af_softReset_T1;
t_if_ccip_Rx pck_cp2af_sRx_T1;
t_if_ccip_Tx pck_af2cp_sTx_T0;

ccip_interface_reg inst_green_ccip_interface_reg  (
    .pClk                           (pClk),
    .pck_cp2af_softReset_T0         (pck_cp2af_softReset),
    .pck_cp2af_pwrState_T0          (pck_cp2af_pwrState), 
    .pck_cp2af_error_T0             (pck_cp2af_error),    
    .pck_cp2af_sRx_T0               (pck_cp2af_sRx),      
    .pck_af2cp_sTx_T0               (pck_af2cp_sTx_T0), 
    
    .pck_cp2af_softReset_T1         (pck_cp2af_softReset_T1),
    .pck_cp2af_pwrState_T1          (pck_cp2af_pwrState_T1), 
    .pck_cp2af_error_T1             (pck_cp2af_error_T1),    
    .pck_cp2af_sRx_T1               (pck_cp2af_sRx_T1),      
    .pck_af2cp_sTx_T1               (pck_af2cp_sTx)    
);   
  
// =================================================================
// NLB AFU- provides validation, performance characterization modes. 
// It also serves as a reference design
// =================================================================

nlb_lpbk nlb_lpbk(
    .Clk_400                        (pClk),
    .SoftReset                      (pck_cp2af_softReset_T1),
    .cp2af_sRxPort                  (pck_cp2af_sRx_T1),
    .af2cp_sTxPort                  (pck_af2cp_sTx_T0) 
);

// =================================================================
// ccip_debug is a reference debug module for tapping cci-p signals
// =================================================================
/*
ccip_debug inst_ccip_debug(
  .pClk                (pClk),        
  .pck_cp2af_pwrState  (pck_cp2af_pwrState),
  .pck_cp2af_error     (pck_cp2af_error),

  .pck_cp2af_sRx       (pck_cp2af_sRx),   
  .pck_af2cp_sTx       (pck_af2cp_sTx)    
);
*/

`ifdef FPGA_PLATFORM_DISCRETE
   ** This version of NLB cannot be built on FPGA_PLATFORM_DISCRETE mode **
`endif
   
endmodule
