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
// This shim is an interposer between the platform connection and the AFU's
// top-level module.  It offers automatic clock domain crossing and buffering,
// controlled by an AFU's JSON file.
//

// Preprocessor variables below are defined by the platform database configuration,
// by running the afu_platform_config script.
`include "platform_if.vh"


module platform_shim_ccip_std_afu
  #(
    `PLATFORM_ARG_LIST_BEGIN
`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
    `PLATFORM_ARG_APPEND(parameter NUM_LOCAL_MEM_BANKS = `AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM)
`endif
`ifdef AFU_TOP_REQUIRES_HSSI_RAW_PR
    `PLATFORM_ARG_APPEND(parameter NUM_HSSI_RAW_PR_IFCS = `AFU_TOP_REQUIRES_HSSI_RAW_PR)
`endif
    )
   (
    // CCI-P Clocks and Resets
    input  logic        pClk,                 // Primary CCI-P interface clock.
    input  logic        pClkDiv2,             // Aligned, pClk divided by 2.
    input  logic        pClkDiv4,             // Aligned, pClk divided by 4.
    input  logic        uClk_usr,             // User clock domain. Refer to clock programming guide.
    input  logic        uClk_usrDiv2,         // Aligned, user clock divided by 2.
    input  logic        pck_cp2af_softReset,  // CCI-P ACTIVE HIGH Soft Reset

`ifdef AFU_TOP_REQUIRES_POWER_2BIT
    input  logic [1:0]  pck_cp2af_pwrState,   // CCI-P AFU Power State
`endif
`ifdef AFU_TOP_REQUIRES_ERROR_1BIT
    input  logic        pck_cp2af_error,      // CCI-P Protocol Error Detected
`endif

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
    // Local memory interface
    avalon_mem_if.to_fiu local_mem[NUM_LOCAL_MEM_BANKS],
`endif

`ifdef AFU_TOP_REQUIRES_HSSI_RAW_PR
  `ifdef PLATFORM_PARAM_HSSI_RAW_PR_IS_VECTOR
    pr_hssi_if.to_fiu   hssi[NUM_HSSI_RAW_PR_IFCS],
  `else
    // Legacy hssi interface on a platform with only one HSSI port
    pr_hssi_if.to_fiu   hssi,
  `endif
`endif

    // CCI-P structures
    input  t_if_ccip_Rx pck_cp2af_sRx,        // CCI-P Rx Port
    output t_if_ccip_Tx pck_af2cp_sTx         // CCI-P Tx Port
    );

    // ====================================================================
    //  CCI-P register insertion and clock crossing
    // ====================================================================

    logic afu_cp2af_softReset;
    t_if_ccip_Tx afu_af2cp_sTx;
    t_if_ccip_Rx afu_cp2af_sRx;
    logic [1:0] afu_cp2af_pwrState;
    logic afu_cp2af_error;

`ifndef AFU_TOP_REQUIRES_POWER_2BIT
    logic [1:0] pck_cp2af_pwrState = 2'b0;
`endif
`ifndef AFU_TOP_REQUIRES_ERROR_1BIT
    logic pck_cp2af_error = 1'b0;
`endif

    platform_shim_ccip platform_shim_ccip
       (
        .pClk,

        .pck_cp2af_softReset,
        .pck_cp2af_pwrState,
        .pck_cp2af_error,
        .pck_cp2af_sRx,
        .pck_af2cp_sTx,

`ifdef PLATFORM_PARAM_CCI_P_CLOCK_IS_DEFAULT
         // Default clock
        .afu_clk(pClk),
`else
         // Updated CCI-P clock requested
        .afu_clk(`PLATFORM_PARAM_CCI_P_CLOCK),
`endif
        .afu_cp2af_sRx,
        .afu_af2cp_sTx,
        .afu_cp2af_softReset,
        .afu_cp2af_pwrState,
        .afu_cp2af_error
        );


    // ====================================================================
    //  Pipeline stages and clock crossing for local memory
    // ====================================================================

    // New local_mem instances in the target clock domain
`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM

    logic afu_local_mem_clk[NUM_LOCAL_MEM_BANKS];
    logic afu_local_mem_reset[NUM_LOCAL_MEM_BANKS];

    avalon_mem_if#(.ENABLE_LOG(1), .NUM_BANKS(NUM_LOCAL_MEM_BANKS))
        afu_local_mem[NUM_LOCAL_MEM_BANKS](afu_local_mem_clk, afu_local_mem_reset);


    platform_shim_avalon_mem_if
      #(
        .NUM_LOCAL_MEM_BANKS(NUM_LOCAL_MEM_BANKS)
        )
      platform_shim_avalon_mem_if
       (
`ifdef PLATFORM_PARAM_LOCAL_MEMORY_CLOCK_IS_DEFAULT
        // Not used -- local memory clocks unchanged
        .tgt_mem_afu_clk(1'b0),
`else
        // Updated target for local memory clock
        .tgt_mem_afu_clk(`PLATFORM_PARAM_LOCAL_MEMORY_CLOCK),
`endif

        .mem_fiu(local_mem),
        .mem_afu(afu_local_mem),
        .mem_afu_clk(afu_local_mem_clk),
        .mem_afu_reset(afu_local_mem_reset)
        );

`endif


    // ====================================================================
    //  Instantiate the AFU
    // ====================================================================

    // Add a timing stage to reset
    logic afu_cp2af_softReset_q = 1'b1;
    always @(posedge `PLATFORM_PARAM_CCI_P_CLOCK)
    begin
        afu_cp2af_softReset_q <= afu_cp2af_softReset;
    end

    `AFU_TOP_MODULE_NAME
      #(
        `PLATFORM_ARG_LIST_BEGIN
`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
        `PLATFORM_ARG_APPEND(.NUM_LOCAL_MEM_BANKS(NUM_LOCAL_MEM_BANKS))
`endif
`ifdef AFU_TOP_REQUIRES_HSSI_RAW_PR
  `ifdef PLATFORM_PARAM_HSSI_RAW_PR_IS_VECTOR
        `PLATFORM_ARG_APPEND(.NUM_HSSI_RAW_PR_IFCS(NUM_HSSI_RAW_PR_IFCS))
  `endif
`endif
        )
      `AFU_TOP_MODULE_NAME
       (
        // All the clocks are still passed in as usual.  It is the responsibility
        // of the AFU to pick the right clock for CCI-P traffic if CCI-P is now
        // running on something other than pClk.  Ideally, we would change the
        // name of pck_af2cp_sTx and pck_cp2af_sRx below to match the clock,
        // but that would get messy.
        .pClk(pClk),
        .pClkDiv2(pClkDiv2),
        .pClkDiv4(pClkDiv4),
        .uClk_usr(uClk_usr),
        .uClk_usrDiv2(uClk_usrDiv2),
        .pck_cp2af_softReset(afu_cp2af_softReset_q),
`ifdef AFU_TOP_REQUIRES_POWER_2BIT
        .pck_cp2af_pwrState(afu_cp2af_pwrState),
`endif
`ifdef AFU_TOP_REQUIRES_ERROR_1BIT
        .pck_cp2af_error(afu_cp2af_error),
`endif

`ifdef AFU_TOP_REQUIRES_LOCAL_MEMORY_AVALON_MM
        // Local memory's clock is included in its interface.  If a clock crossing
        // was inserted, the included clock is updated to match.
        .local_mem(afu_local_mem),
`endif

`ifdef AFU_TOP_REQUIRES_HSSI_RAW_PR
        .hssi(hssi),
`endif

        .pck_af2cp_sTx(afu_af2cp_sTx),
        .pck_cp2af_sRx(afu_cp2af_sRx)
        );

endmodule // platform_shim_cci_std_afu
