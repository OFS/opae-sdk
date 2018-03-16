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
// Module Name :    
// Project :       
// Description :  

// ***************************************************************************

import ccip_if_pkg::*;
module green_ccip_if_reg(
  // CCI-P Clocks and Resets
  input           logic             pClk,                 // 400MHz - CCI-P clock domain. Primary interface clock

  input           logic             pck_cp2af_softReset_d,
  output          logic             pck_cp2af_softReset_q,
  
  // Interface structures
  input           t_if_ccip_Rx      pck_cp2af_sRx_d,       
  output          t_if_ccip_Rx      pck_cp2af_sRx_q,

  input           t_if_ccip_Tx      pck_af2cp_sTx_d,        
  output          t_if_ccip_Tx      pck_af2cp_sTx_q
);

always_ff @ (posedge pClk)
begin
  pck_cp2af_sRx_q       <= pck_cp2af_sRx_d;
  pck_af2cp_sTx_q       <= pck_af2cp_sTx_d;
  pck_cp2af_softReset_q <= pck_cp2af_softReset_d;
end

endmodule
