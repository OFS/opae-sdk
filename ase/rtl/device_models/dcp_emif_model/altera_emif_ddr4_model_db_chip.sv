// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.



///////////////////////////////////////////////////////////////////////////////
// Basic simulation model of DDR4 Data Buffer used by LRDIMM
//
///////////////////////////////////////////////////////////////////////////////
module altera_emif_ddr4_model_db_chip (
   input       BCK_t,
   input       BCK_c,
   input       BCKE,
   input       BODT,
   input       BVrefCA,
   input [3:0] BCOM,

   inout [7:0] MDQ,
   inout       MDQS0_t,
   inout       MDQS0_c,
   inout       MDQS1_t,
   inout       MDQS1_c,

   inout [7:0] DQ,
   inout       DQS0_t,
   inout       DQS0_c,
   inout       DQS1_t,
   inout       DQS1_c,

   output      ALERT_n,

   input       VDD,
   input       VSS
);

   timeunit 1ps;
   timeprecision 1ps;

   genvar i;

   generate
      for (i = 0; i < 8; i = i + 1) begin : gen_dq_delay
         altera_emif_ddrx_model_bidir_delay #(
            .DELAY                         (1.0)
         ) inst_dq_bidir_dly (
            .porta                         (MDQ[i]),
            .portb                         (DQ[i])
         );
      end
   endgenerate

   altera_emif_ddrx_model_bidir_delay #(
      .DELAY                         (1.0)
   ) dqs_p_0 (
      .porta                         (MDQS0_t),
      .portb                         (DQS0_t)
   );

   altera_emif_ddrx_model_bidir_delay #(
      .DELAY                         (1.0)
   ) dqs_n_0 (
      .porta                         (MDQS0_c),
      .portb                         (DQS0_c)
   );

   altera_emif_ddrx_model_bidir_delay #(
      .DELAY                         (1.0)
   ) dqs_p_1 (
      .porta                         (MDQS1_t),
      .portb                         (DQS1_t)
   );

   altera_emif_ddrx_model_bidir_delay #(
      .DELAY                         (1.0)
   ) dqs_n_1 (
      .porta                         (MDQS1_c),
      .portb                         (DQS1_c)
   );

endmodule

