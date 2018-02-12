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
// This shim manages just CCI-P signals.  It handles both registering the
// signals for timing and crossing to a different clock.
//

`include "platform_if.vh"


module platform_shim_ccip
   (
    // CCI-P Clocks and Resets
    input  logic        pClk,                 // Primary CCI-P interface clock.
    input  logic        pck_cp2af_softReset,  // CCI-P ACTIVE HIGH Soft Reset

    input  logic [1:0]  pck_cp2af_pwrState,   // CCI-P AFU Power State
    input  logic        pck_cp2af_error,      // CCI-P Protocol Error Detected

    // CCI-P structures (FIU side)
    input  t_if_ccip_Rx pck_cp2af_sRx,
    output t_if_ccip_Tx pck_af2cp_sTx,

    // CCI-P structures (AFU side, AFU clock domain)
    input  logic        afu_clk,
    output t_if_ccip_Rx afu_cp2af_sRx,
    input  t_if_ccip_Tx afu_af2cp_sTx,
    output logic        afu_cp2af_softReset,

    output logic [1:0]  afu_cp2af_pwrState,
    output logic        afu_cp2af_error
    );

    //
    // Has the AFU JSON requested a clock crossing for the CCI-P signals?
    // We compute this here because it affects register stage insertion.
    //

`ifdef PLATFORM_PARAM_CCI_P_CLOCK_IS_DEFAULT
    // AFU asked for default clock.
    localparam CCI_P_CHANGE_CLOCK = 0;
`elsif PLATFORM_PARAM_CCI_P_CLOCK_IS_PCLK
    // AFU asked for pClk explicitly.
    localparam CCI_P_CHANGE_CLOCK = 0;
`else
    // AFU asked for some other clock.
    localparam CCI_P_CHANGE_CLOCK = 1;
`endif


    // ====================================================================
    //  CCI-P auto-register for timing
    // ====================================================================

    //
    // Register CCI-P signals according to the platform's rules.  This is
    // typically done to relieve timing pressure.  Note that adding latency
    // to the almost full and transmit paths counts against the sending
    // limit to the FIU.
    //
    logic reg_cp2af_softReset;
    t_if_ccip_Tx reg_af2cp_sTx;
    t_if_ccip_Rx reg_cp2af_sRx;
    logic [1:0] reg_cp2af_pwrState;
    logic reg_cp2af_error;

    //
    // How many register stages should be inserted for timing?
    //
    function automatic int numTimingRegStages();
        int n_stages = 0;

        // Were timing registers requested in the AFU JSON?
`ifdef PLATFORM_PARAM_CCI_P_ADD_EXTRA_TIMING_REG_STAGES
        n_stages = `PLATFORM_PARAM_CCI_P_ADD_EXTRA_TIMING_REG_STAGES;
`endif

        // Override the register request if a clock crossing is being
        // inserted here.
        if (CCI_P_CHANGE_CLOCK)
        begin
            // At least one stage is required
            if (n_stages == 0) n_stages = 1;
            // Use at least the recommended number of stages
`ifdef PLATFORM_PARAM_CCI_P_SUGGESTED_EXTRA_TIMING_REG_STAGES
            if (`PLATFORM_PARAM_CCI_P_SUGGESTED_EXTRA_TIMING_REG_STAGES > n_stages)
            begin
                n_stages = `PLATFORM_PARAM_CCI_P_SUGGESTED_EXTRA_TIMING_REG_STAGES;
            end
`endif
        end

        return n_stages;
    endfunction

    localparam NUM_TIMING_REG_STAGES = numTimingRegStages();

    platform_utils_ccip_reg
      #(
        .N_REG_STAGES(NUM_TIMING_REG_STAGES)
        )
      ccip_reg
       (
        .clk(pClk),

        .fiu_reset(pck_cp2af_softReset),
        .fiu_cp2af_sRx(pck_cp2af_sRx),
        .fiu_af2cp_sTx(pck_af2cp_sTx),
        .fiu_cp2af_pwrState(pck_cp2af_pwrState),
        .fiu_cp2af_error(pck_cp2af_error),

        .afu_reset(reg_cp2af_softReset),
        .afu_cp2af_sRx(reg_cp2af_sRx),
        .afu_af2cp_sTx(reg_af2cp_sTx),
        .afu_cp2af_pwrState(reg_cp2af_pwrState),
        .afu_cp2af_error(reg_cp2af_error)
        );


    // ====================================================================
    //  Convert CCI-P signals to the clock domain specified in the
    //  AFU's JSON file.
    // ====================================================================

    generate
        if (CCI_P_CHANGE_CLOCK == 0)
        begin : nc
            // No clock crossing
            always_comb
            begin
                afu_cp2af_softReset = reg_cp2af_softReset;
                reg_af2cp_sTx = afu_af2cp_sTx;
                afu_cp2af_sRx = reg_cp2af_sRx;

                afu_cp2af_pwrState = reg_cp2af_pwrState;
                afu_cp2af_error = reg_cp2af_error;
            end
        end
        else
        begin : c
            // Cross to the target clock
            platform_utils_ccip_async_shim ccip_async_shim
               (
                .bb_softreset(reg_cp2af_softReset),
                .bb_clk(pClk),
                .bb_tx(reg_af2cp_sTx),
                .bb_rx(reg_cp2af_sRx),
                .bb_pwrState(reg_cp2af_pwrState),
                .bb_error(reg_cp2af_error),

                .afu_softreset(afu_cp2af_softReset),
                .afu_clk(afu_clk),
                .afu_tx(afu_af2cp_sTx),
                .afu_rx(afu_cp2af_sRx),
                .afu_pwrState(afu_cp2af_pwrState),
                .afu_error(afu_cp2af_error),

                .async_shim_error()
                );
        end
    endgenerate

endmodule // platform_shim_std_ccip
