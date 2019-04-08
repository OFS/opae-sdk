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




module altera_emif_arch_nf_cal_counter (
   input logic pll_ref_clk_int,
   input logic global_reset_n_int,
   input logic afi_cal_in_progress
);
   timeunit 1ps;
   timeprecision 1ps;

   logic                         done;
   logic [31:0]                  clk_counter;

   logic                         reset_n_sync;
   logic                         cal_in_progress_sync;

   altera_std_synchronizer_nocut
   inst_sync_reset_n (
      .clk     (pll_ref_clk_int),
      .reset_n (1'b1),
      .din     (global_reset_n_int),
      .dout    (reset_n_sync)
   );

   altera_std_synchronizer_nocut
   inst_sync_cal_in_progress (
      .clk     (pll_ref_clk_int),
      .reset_n (1'b1),
      .din     (afi_cal_in_progress),
      .dout    (cal_in_progress_sync)
   );

   enum {
      INIT,
      IDLE,
      COUNT_CAL,
      STOP
   } counter_state;

   assign done = ((counter_state == STOP) ? 1'b1 : 1'b0);

   always_ff @(posedge pll_ref_clk_int) begin
      if(reset_n_sync == 1'b0) begin
         counter_state <= INIT;
      end
      else begin
         case(counter_state)
            INIT:
            begin
               clk_counter <= 32'h0;
               counter_state <= IDLE;
            end

            IDLE:
            begin
               if (cal_in_progress_sync == 1'b1)
               begin
                  counter_state <= COUNT_CAL;
               end
            end

            COUNT_CAL:
            begin
               clk_counter[31:0] <= clk_counter[31:0] + 32'h0000_0001;

               if (cal_in_progress_sync == 1'b0)
               begin
                  counter_state <= STOP;
               end
            end

            STOP:
            begin
               counter_state <= STOP;
            end

            default:
            begin
               counter_state <= INIT;
            end
         endcase
      end
   end

`ifdef ALTERA_EMIF_ENABLE_ISSP
   altsource_probe #(
      .sld_auto_instance_index ("YES"),
      .sld_instance_index      (0),
      .instance_id             ("CALC"),
      .probe_width             (33),
      .source_width            (0),
      .source_initial_value    ("0"),
      .enable_metastability    ("NO")
      ) cal_counter_issp (
      .probe  ({done, clk_counter[31:0]})
   );
`endif

endmodule
