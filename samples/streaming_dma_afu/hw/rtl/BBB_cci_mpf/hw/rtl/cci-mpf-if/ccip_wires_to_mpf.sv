//
// Map CCI-P wires to the MPF interface.
//

`include "cci_mpf_if.vh"

`ifdef MPF_HOST_IFC_CCIP

module ccip_wires_to_mpf
  #(
    parameter REGISTER_INPUTS = 1,
    parameter REGISTER_OUTPUTS = 1
    )
   (
    // -------------------------------------------------------------------
    //
    //   System interface.  These signals come directly from the CCI.
    //
    // -------------------------------------------------------------------

    // CCI-P Clocks and Resets
    input  logic        pClk,                // 400MHz - CCI-P clock domain. Primary interface clock
    input  logic        pClkDiv2,            // 200MHz - CCI-P clock domain.
    input  logic        pClkDiv4,            // 100MHz - CCI-P clock domain.
    input  logic        uClk_usr,            // User clock domain. Refer to clock programming guide  ** Currently provides fixed 300MHz clock **
    input  logic        uClk_usrDiv2,        // User clock domain. Half the programmed frequency  ** Currently provides fixed 150MHz clock **
    input  logic        pck_cp2af_softReset, // CCI-P ACTIVE HIGH Soft Reset
    input  logic [1:0]  pck_cp2af_pwrState,  // CCI-P AFU Power State
    input  logic        pck_cp2af_error,     // CCI-P Protocol Error Detected

    // Interface structures
    input  t_if_ccip_Rx pck_cp2af_sRx,       // CCI-P Rx Port
    output t_if_ccip_Tx pck_af2cp_sTx,       // CCI-P Tx Port

    // -------------------------------------------------------------------
    //
    //   MPF interface.
    //
    // -------------------------------------------------------------------

    cci_mpf_if fiu
    );

    logic  clk;
    assign clk = pClk;

    logic reset = 1'b1;
    assign fiu.reset = reset;

    always @(posedge clk)
    begin
        reset <= pck_cp2af_softReset;
    end


    // Track multi-beat writes and optionally turn them into single-beat
    // writes.  (Useful for debugging)
    t_if_cci_mpf_c1_Tx fiu_c1_tx;
    cci_mpf_multi_line_write_to_single
     #(
       .CONVERT_TO_SINGLE(0)
       )
     wr_beats
      (
       .clk,
       .reset(reset),
       .fiu_c1_tx,
       .afu_c1_tx(fiu.c1Tx)
       );

    generate
        if (REGISTER_OUTPUTS)
        begin : reg_out
            always_ff @(posedge clk)
            begin
                pck_af2cp_sTx.c0 <= cci_mpf_cvtC0TxToBase(fiu.c0Tx);
                pck_af2cp_sTx.c1 <= cci_mpf_cvtC1TxToBase(fiu_c1_tx);
                pck_af2cp_sTx.c2 <= fiu.c2Tx;

                fiu.c0TxAlmFull <= pck_cp2af_sRx.c0TxAlmFull;
                fiu.c1TxAlmFull <= pck_cp2af_sRx.c1TxAlmFull;
            end
        end
        else
        begin : wire_out
            always_comb
            begin
                pck_af2cp_sTx.c0 = cci_mpf_cvtC0TxToBase(fiu.c0Tx);
                pck_af2cp_sTx.c1 = cci_mpf_cvtC1TxToBase(fiu_c1_tx);
                pck_af2cp_sTx.c2 = fiu.c2Tx;

                fiu.c0TxAlmFull = pck_cp2af_sRx.c0TxAlmFull;
                fiu.c1TxAlmFull = pck_cp2af_sRx.c1TxAlmFull;
            end
        end
    endgenerate

    //
    // Buffer incoming read responses for timing
    //
    generate
        if (REGISTER_INPUTS)
        begin : reg_in
            always_ff @(posedge clk)
            begin
                fiu.c0Rx <= pck_cp2af_sRx.c0;
                fiu.c1Rx <= pck_cp2af_sRx.c1;
            end
        end
        else
        begin : wire_in
            always_comb
            begin
                fiu.c0Rx = pck_cp2af_sRx.c0;
                fiu.c1Rx = pck_cp2af_sRx.c1;
            end
        end
    endgenerate

endmodule // ccip_wires_to_mpf

//
// Optionally convert beats of a multi-line write request to individual
// requests.  This can be useful for debugging.
//
// The module always tracks the order in which multi-beat writes are
// received and verifies that beats are received together.
//
module cci_mpf_multi_line_write_to_single
  #(
    CONVERT_TO_SINGLE = 0
    )
   (
    input  logic clk,
    input  logic reset,

    output t_if_cci_mpf_c1_Tx fiu_c1_tx,
    input  t_if_cci_mpf_c1_Tx afu_c1_tx
    );

    t_cci_clNum beat_num;

    generate
        if (CONVERT_TO_SINGLE == 0)
        begin : std
            // Normal mode.  Keep multi-beat writes.
            always_comb
            begin
                fiu_c1_tx = afu_c1_tx;
            end
        end
        else
        begin : no_multi
            always_comb
            begin
                fiu_c1_tx = afu_c1_tx;

                // Convert to a single line request
                if (cci_mpf_c1TxIsWriteReq_noCheckValid(afu_c1_tx))
                begin
                    fiu_c1_tx.hdr.base.sop = 1'b1;
                    fiu_c1_tx.hdr.base.cl_len = eCL_LEN_1;
                    fiu_c1_tx.hdr.base.address[1:0] = fiu_c1_tx.hdr.base.address[1:0] |
                                                      beat_num;
                end
            end
        end
    endgenerate

    // Track the beat number
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            beat_num <= t_cci_clNum'(0);
        end
        else if (cci_mpf_c1TxIsWriteReq(afu_c1_tx))
        begin
            if (beat_num == afu_c1_tx.hdr.base.cl_len)
            begin
                // Last beat in the packet
                beat_num <= t_cci_clNum'(0);
            end
            else
            begin
                beat_num <= beat_num + t_cci_clNum'(1);
            end

            assert(afu_c1_tx.hdr.base.sop == (beat_num == 0)) else
                $fatal("ccip_wires_to_mpf: SOP out of phase");
        end
    end

endmodule

`endif
