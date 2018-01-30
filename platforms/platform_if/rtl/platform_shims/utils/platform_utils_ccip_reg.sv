//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
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
// Add buffer stages to CCI-P structs
//

import ccip_if_pkg::*;

module platform_utils_ccip_reg
  #(
    parameter REGISTER_RX = 1,
    parameter REGISTER_TX = 1,
    parameter REGISTER_RESET = 1,
    parameter REGISTER_ERROR = 1,

    // Number of stages to add when registering inputs or outputs
    parameter N_REG_STAGES = 1
    )
   (
    input  logic clk,

    // FIU side
    input  logic fiu_reset,
    input  t_if_ccip_Rx fiu_cp2af_sRx,       // CCI-P Rx Port
    output t_if_ccip_Tx fiu_af2cp_sTx,       // CCI-P Tx Port
    input  logic [1:0] fiu_cp2af_pwrState,   // CCI-P AFU Power State
    input  logic fiu_cp2af_error,            // CCI-P Protocol Error Detected

    // AFU side
    output logic afu_reset,
    output t_if_ccip_Rx afu_cp2af_sRx,       // CCI-P Rx Port
    input  t_if_ccip_Tx afu_af2cp_sTx,       // CCI-P Tx Port
    output logic [1:0] afu_cp2af_pwrState,
    output logic afu_cp2af_error
    );

    genvar s;
    generate
        //
        // Register reset
        //
        if (REGISTER_RESET && N_REG_STAGES)
        begin : reg_reset
            (* altera_attribute = {"-name AUTO_SHIFT_REGISTER_RECOGNITION OFF; -name PRESERVE_REGISTER ON"} *)
            logic reset[N_REG_STAGES] = '{N_REG_STAGES{1'b1}};

            always @(posedge clk)
            begin
                reset[0] <= fiu_reset;
            end

            for (s = 0; s < N_REG_STAGES - 1; s = s + 1)
            begin
                always @(posedge clk)
                begin
                    reset[s+1] <= reset[s];
                end
            end

            assign afu_reset = reset[N_REG_STAGES - 1];
        end
        else
        begin : wire_reset
            assign afu_reset = fiu_reset;
        end


        //
        // Register TX
        //
        if (REGISTER_TX && N_REG_STAGES)
        begin : reg_tx
            (* altera_attribute = {"-name AUTO_SHIFT_REGISTER_RECOGNITION OFF; -name PRESERVE_REGISTER ON"} *)
            t_if_ccip_Tx reg_af2cp_sTx[N_REG_STAGES];

            // Tx to register stages
            always_ff @(posedge clk)
            begin
                reg_af2cp_sTx[0] <= afu_af2cp_sTx;
            end

            // Intermediate stages
            for (s = 0; s < N_REG_STAGES - 1; s = s + 1)
            begin
                always_ff @(posedge clk)
                begin
                    reg_af2cp_sTx[s+1] <= reg_af2cp_sTx[s];
                end
            end

            assign fiu_af2cp_sTx = reg_af2cp_sTx[N_REG_STAGES - 1];
        end
        else
        begin : wire_tx
            assign fiu_af2cp_sTx = afu_af2cp_sTx;
        end


        //
        // Register RX
        //
        if (REGISTER_RX && N_REG_STAGES)
        begin : reg_rx
            (* altera_attribute = {"-name AUTO_SHIFT_REGISTER_RECOGNITION OFF; -name PRESERVE_REGISTER ON"} *)
            t_if_ccip_Rx reg_cp2af_sRx[N_REG_STAGES];

            always_ff @(posedge clk)
            begin
                reg_cp2af_sRx[0] <= fiu_cp2af_sRx;
            end

            // Intermediate stages
            for (s = 0; s < N_REG_STAGES - 1; s = s + 1)
            begin
                always_ff @(posedge clk)
                begin
                    reg_cp2af_sRx[s+1] <= reg_cp2af_sRx[s];
                end
            end

            assign afu_cp2af_sRx = reg_cp2af_sRx[N_REG_STAGES - 1];
        end
        else
        begin : wire_rx
            assign afu_cp2af_sRx = fiu_cp2af_sRx;
        end


        //
        // Register power state and error signals
        //
        if (REGISTER_ERROR && N_REG_STAGES)
        begin : reg_err
            (* altera_attribute = {"-name AUTO_SHIFT_REGISTER_RECOGNITION OFF; -name PRESERVE_REGISTER ON"} *)
            logic [1:0] reg_cp2af_pwrState[N_REG_STAGES];
            (* altera_attribute = {"-name AUTO_SHIFT_REGISTER_RECOGNITION OFF; -name PRESERVE_REGISTER ON"} *)
            logic reg_cp2af_error[N_REG_STAGES];

            always_ff @(posedge clk)
            begin
                reg_cp2af_pwrState[0] <= fiu_cp2af_pwrState;
                reg_cp2af_error[0] <= fiu_cp2af_error;
            end

            // Intermediate stages
            for (s = 0; s < N_REG_STAGES - 1; s = s + 1)
            begin
                always_ff @(posedge clk)
                begin
                    reg_cp2af_pwrState[s+1] <= reg_cp2af_pwrState[s];
                    reg_cp2af_error[s+1] <= reg_cp2af_error[s];
                end
            end

            assign afu_cp2af_pwrState = reg_cp2af_pwrState[N_REG_STAGES - 1];
            assign afu_cp2af_error = reg_cp2af_error[N_REG_STAGES - 1];
        end
        else
        begin : wire_err
            assign afu_cp2af_pwrState = fiu_cp2af_pwrState;
            assign afu_cp2af_error = fiu_cp2af_error;
        end
    endgenerate

endmodule // platform_utils_ccip_reg

