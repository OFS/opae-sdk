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
// Module Name :	  ccip_std_afu
// Project :        ccip afu top (work in progress)
// Description :    This module instantiates CCI-P compliant AFU

// ***************************************************************************
import ccip_if_pkg::*;
module ccip_debug(
  // CCI-P Clocks and Resets
  input           logic             pClk,                 // 400MHz - CCI-P clock domain. Primary interface clock
  input           logic [1:0]       pck_cp2af_pwrState,   // CCI-P AFU Power State
  input           logic             pck_cp2af_error,      // CCI-P Protocol Error Detected

  // Interface structures
  input           t_if_ccip_Rx      pck_cp2af_sRx,        // CCI-P Rx Port
  output          t_if_ccip_Tx      pck_af2cp_sTx         // CCI-P Tx Port
);

// Register the signals
// Use these registered signals in Signal Tap
(* noprune *)   logic [1:0]  dbg_pwrState;
(* noprune *)   logic        dbg_error;
(* noprune *)   t_if_ccip_Rx dbg_sRx;
(* noprune *)   t_if_ccip_Tx dbg_sTx;

always_ff @ (posedge pClk)
begin
    dbg_pwrState           <= pck_cp2af_pwrState;
    dbg_error              <= pck_cp2af_error;

    // Rx signals
    dbg_sRx.c0.hdr         <= pck_cp2af_sRx.c0.hdr;
    dbg_sRx.c0.rspValid    <= pck_cp2af_sRx.c0.rspValid;
    dbg_sRx.c0.mmioRdValid <= pck_cp2af_sRx.c0.mmioRdValid;
    dbg_sRx.c0.mmioWrValid <= pck_cp2af_sRx.c0.mmioWrValid;
    // Data 8b only
    dbg_sRx.c0.data[7:0]   <= pck_cp2af_sRx.c0.data[7:0];

    dbg_sRx.c1             <= pck_cp2af_sRx.c1;

    // Tx signals
    dbg_sTx.c0              <= pck_af2cp_sTx;
    dbg_sTx.c1.hdr          <= pck_af2cp_sTx.c1.hdr;
    dbg_sTx.c1.valid        <= pck_af2cp_sTx.c1.valid;
    // Data 8b only
    dbg_sTx.c1.data[7:0]    <= pck_af2cp_sTx.c1.data[7:0];
    dbg_sTx.c2              <= pck_af2cp_sTx.c2;
end

endmodule
