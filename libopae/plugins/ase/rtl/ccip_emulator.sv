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
 * Module Info: CCI Emulation top-level - SystemVerilog Module
 * Language   : System{Verilog}
 * Author     : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * MAJOR UPGRADES:
 * Wed Aug 10 22:17:28 PDT 2011   | Completed FIFO'ing all channels in all directions
 * Tue Jun 17 16:46:06 PDT 2014   | Started cleaning up code to add latency model
 *                                | Connect up new transactions CCI 1.8
 * Tue Dec 23 11:01:28 PST 2014   | Optimizing ASE for performance
 *                                | Added return path FIFOs for marshalling
 * Tue Oct 21 13:33:34 PDT 2015   | CCIP migration
 *
 */

import ase_pkg::*;
import ccip_if_pkg::*;

`include "platform.vh"

// CCI to Memory translator module
module ccip_emulator
   (
    // CCI-P Clocks and Resets
    output logic       pClk,                   // 400MHz - CCI-P clock domain. Primary interface clock
    output logic       pClkDiv2,               // 200MHz - CCI-P clock domain
    output logic       pClkDiv4,               // 100MHz - CCI-P clock domain
   
    // User clocks
    output logic       uClk_usr,               // User clock domain. Refer to clock programming guide
    output logic       uClk_usrDiv2,           // User clock domain. Half the programmed frequency
   
    // Power & error states
    output logic       pck_cp2af_softReset,    // CCI-P ACTIVE HIGH Soft Reset
    output logic [1:0] pck_cp2af_pwrState,     // CCI-P AFU Power State
    output logic       pck_cp2af_error,        // CCI-P Protocol Error Detected
   
    // Interface structures
    output 	      t_if_ccip_Rx pck_cp2af_sRx, // CCI-P Rx Port
    input 	      t_if_ccip_Tx pck_af2cp_sTx  // CCI-P Tx Port
    );

    // Power and error state
    assign pck_cp2af_pwrState = 2'b0;
    assign pck_cp2af_error    = 1'b0;

    // Clock/reset
    logic          Clk16UI ;
    logic          Clk32UI ;
    logic          Clk64UI ;
    logic          SoftReset;

    // Tx0 & bookkeeper
    ASETxHdr_t                         ASE_C0TxHdr;
    TxHdr_t                            C0TxHdr;
    logic                              C0TxValid;

    // Tx1 & bookkeeper
    ASETxHdr_t                         ASE_C1TxHdr;
    TxHdr_t                            C1TxHdr;
    logic [CCIP_DATA_WIDTH-1:0]        C1TxData;
    logic                              C1TxValid;

    // Tx2
    MMIOHdr_t                          C2TxHdr;
    logic                              C2TxMmioRdValid;
    logic [CCIP_MMIO_RDDATA_WIDTH-1:0] C2TxData;

    // Rx0 & bookkeeper
    logic                              C0RxMmioWrValid;
    logic                              C0RxMmioRdValid;
    logic [CCIP_DATA_WIDTH-1:0] 	      C0RxData;
    RxHdr_t                            C0RxHdr;
    logic                              C0RxRspValid;

    // Rx1 & bookkeeper
    RxHdr_t                            C1RxHdr;
    logic                              C1RxRspValid;

    // Almost full signals
    logic                              C0TxAlmFull;
    logic                              C1TxAlmFull;


    // Internal 800 Mhz clock (for creating synchronized clocks)
    logic                              Clk8UI;
    logic                              clk;

    // Real Full ch2as
    logic                              cf2as_ch0_realfull;
    logic                              cf2as_ch1_realfull;

    // Reset lockdown flag
    // Stop taking any more requests if ase_reset was requested
    logic                              reset_lockdown = 0;

    // Disable settings
    logic                              ase_logger_disable;
    logic                              ase_checker_disable;
 
    // Local valid/debug breakout signals
    logic                              C0RxRdValid;
    logic                              C0RxUMsgValid;
    logic                              C1RxWrValid;
    logic                              C1RxIntrValid;

    /*
     * Request/Response Type conversion functions
     */
    // ccip_tx0_to_ase_tx0: Convert from CCIP -> ASE Tx0
    function ASETxHdr_t ccip_tx0_to_ase_tx0(t_ccip_c0_ReqMemHdr inhdr);
        ASETxHdr_t        txasehdr;
    begin
        txasehdr.txhdr = TxHdr_t'(inhdr);
        txasehdr.channel_id = 0;
        case (inhdr.req_type)
            eREQ_RDLINE_I : txasehdr.txhdr.reqtype = ASE_RDLINE_I;
            eREQ_RDLINE_S : txasehdr.txhdr.reqtype = ASE_RDLINE_S;
        endcase
        return txasehdr;
    end
    endfunction

    // ccip_tx1_to_ase_tx1: Convert from CCIP -> ASE Tx1
    function ASETxHdr_t ccip_tx1_to_ase_tx1(t_ccip_c1_ReqMemHdr inhdr);
        ASETxHdr_t        txasehdr;
        logic [41:0]      c1tx_mcl_baseaddr;
        ccip_reqtype_t c1tx_mcl_basetype;
        ccip_len_t     c1tx_mcl_baselen;
        logic [15:0]   c1tx_mcl_basemdata;
        ccip_vc_t      c1tx_mcl_basevc;
    begin
        txasehdr.txhdr = TxHdr_t'(inhdr);
        txasehdr.channel_id = 1;
        // Request type remap to ASE internal types
        case (inhdr.req_type)
            eREQ_WRLINE_I : txasehdr.txhdr.reqtype = ASE_WRLINE_I;
            eREQ_WRLINE_M : txasehdr.txhdr.reqtype = ASE_WRLINE_M;
            eREQ_WRFENCE  : txasehdr.txhdr.reqtype = ASE_WRFENCE;
            eREQ_WRPUSH_I : txasehdr.txhdr.reqtype = ASE_WRPUSH;
           `ifdef ASE_ENABLE_INTR_FEATURE
            eREQ_INTR     : txasehdr.txhdr.reqtype = ASE_INTR_REQ;
           `endif
        endcase

        // Accomodating MCL addr[41:2]=X when SOP=0
        if ( (txasehdr.txhdr.reqtype == ASE_WRLINE_I)
             || (txasehdr.txhdr.reqtype == ASE_WRLINE_M)
             || (txasehdr.txhdr.reqtype == ASE_WRPUSH) )
        begin
            if (inhdr.sop)
            begin
                c1tx_mcl_baseaddr  = txasehdr.txhdr.addr;
                c1tx_mcl_basetype  = txasehdr.txhdr.reqtype;
                c1tx_mcl_baselen   = txasehdr.txhdr.len;
                c1tx_mcl_basemdata = txasehdr.txhdr.mdata;
                c1tx_mcl_basevc    = txasehdr.txhdr.vc;
            end
            else
            begin
                txasehdr.txhdr.reqtype = c1tx_mcl_basetype;
                case (c1tx_mcl_baselen)
                    ASE_2CL: txasehdr.txhdr.addr = {c1tx_mcl_baseaddr[41:1], inhdr.address[0]};
                    ASE_4CL: txasehdr.txhdr.addr = {c1tx_mcl_baseaddr[41:2], inhdr.address[1:0]};
                endcase // case (c1tx_mcl_baselen)
                txasehdr.txhdr.mdata = c1tx_mcl_basemdata;
                txasehdr.txhdr.vc    = c1tx_mcl_basevc;
            end
        end
        return txasehdr;
    end
    endfunction

    // ase_rx0_to_ccip_rx0: Convert from ASE -> CCIP RX0
    function t_ccip_c0_RspMemHdr ase_rx0_to_ccip_rx0(ASERxHdr_t inhdr);
        t_ccip_c0_RspMemHdr rxasehdr;
    begin
        rxasehdr = RxHdr_t'(inhdr.rxhdr);
        case (inhdr.rxhdr.resptype)
            ASE_RD_RSP     : rxasehdr.resp_type = eRSP_RDLINE;
           `ifdef ASE_ENABLE_UMSG_FEATURE
            ASE_UMSG       : rxasehdr.resp_type = eRSP_UMSG;
           `endif
        endcase
        case (inhdr.rxhdr.vc_used)
            VC_VA           : rxasehdr.vc_used = eVC_VA;
            VC_VL0          : rxasehdr.vc_used = eVC_VL0;
            VC_VH0          : rxasehdr.vc_used = eVC_VH0;
            VC_VH1          : rxasehdr.vc_used = eVC_VH1;
        endcase
        return rxasehdr;
    end
    endfunction

    // ase_rx1_to_ccip_rx1: Convert from ASE -> CCIP RX1
    function t_ccip_c1_RspMemHdr ase_rx1_to_ccip_rx1(ASERxHdr_t inhdr);
        t_ccip_c1_RspMemHdr rxasehdr;
    begin
        rxasehdr = RxHdr_t'(inhdr.rxhdr);
        case (inhdr.rxhdr.resptype)
            ASE_WR_RSP      : rxasehdr.resp_type = eRSP_WRLINE;
            ASE_WRFENCE_RSP : rxasehdr.resp_type = eRSP_WRFENCE;
           `ifdef ASE_ENABLE_INTR_FEATURE
            ASE_INTR_RSP    : rxasehdr.resp_type = eRSP_INTR;
           `endif
        endcase
        case (inhdr.rxhdr.vc_used)
            VC_VA           : rxasehdr.vc_used = eVC_VA;
            VC_VL0          : rxasehdr.vc_used = eVC_VL0;
            VC_VH0          : rxasehdr.vc_used = eVC_VH0;
            VC_VH1          : rxasehdr.vc_used = eVC_VH1;
        endcase
        return rxasehdr;
    end
    endfunction


    // ASE's internal reset signal
    logic                 ase_reset = 1;
    logic                 init_reset = 1;

    /*
     * Remapping ASE CCIP to cvl_pkg struct
     */
    // Clocks 16ui, 32ui, 64ui
    assign pClk = Clk16UI;
    assign pClkDiv2 = Clk32UI;
    assign pClkDiv4 = Clk64UI;

    // Reset out
    assign pck_cp2af_softReset = SoftReset;


    // Rx/Tx mapping from ccip_if_pkg to ASE's internal format
    always @(*) begin : ccip2ase_remap
        // Rx OUT (CH0)
        // If MMIO RDWR request, cast directly to interface format
        if (C0RxMmioRdValid|C0RxMmioWrValid) 
        begin
            pck_cp2af_sRx.c0.hdr      <= t_ccip_c0_RspMemHdr'(C0RxHdr);
        end        
        else // Else, cast via function, changing resptype(s)
        begin
            pck_cp2af_sRx.c0.hdr      <= ase_rx0_to_ccip_rx0(t_ccip_c0_RspMemHdr'(C0RxHdr));
        end
        pck_cp2af_sRx.c0.data         <= t_ccip_clData'(C0RxData);
        pck_cp2af_sRx.c0.rspValid     <= C0RxRspValid;
        pck_cp2af_sRx.c0.mmioRdValid  <= C0RxMmioRdValid;
        pck_cp2af_sRx.c0.mmioWrValid  <= C0RxMmioWrValid;

        // Rx OUT (CH1)
        pck_cp2af_sRx.c1.hdr         <= ase_rx1_to_ccip_rx1(t_ccip_c1_RspMemHdr'(C1RxHdr));
        pck_cp2af_sRx.c1.rspValid    <= C1RxRspValid;

        // Tx OUT (CH0)
        ASE_C0TxHdr                  <= ccip_tx0_to_ase_tx0( pck_af2cp_sTx.c0.hdr );
        C0TxHdr                      <= ASE_C0TxHdr.txhdr;
        C0TxValid                    <= pck_af2cp_sTx.c0.valid;

        // Tx OUT (CH1)
        ASE_C1TxHdr                  <= ccip_tx1_to_ase_tx1(pck_af2cp_sTx.c1.hdr);
        C1TxHdr                      <= ASE_C1TxHdr.txhdr;
        C1TxData                     <= pck_af2cp_sTx.c1.data;
        C1TxValid                    <= pck_af2cp_sTx.c1.valid;

        // Tx OUT (CH2)
        C2TxHdr                      <= MMIOHdr_t'(pck_af2cp_sTx.c2.hdr);
        C2TxData                     <= pck_af2cp_sTx.c2.data;
        C2TxMmioRdValid              <= pck_af2cp_sTx.c2.mmioRdValid;

        // Almost full signals
        pck_cp2af_sRx.c0TxAlmFull    <= C0TxAlmFull | reset_lockdown;
        pck_cp2af_sRx.c1TxAlmFull    <= C1TxAlmFull | reset_lockdown;
    end


    /*
     * DPI import/export functions
     */
    // Scope function
    import "DPI-C" context function void scope_function();

    // ASE Initialize function
    import "DPI-C" context task ase_init();

    // Indication that ASE is ready
    import "DPI-C" function void ase_ready();

    // Global listener function
    import "DPI-C" context task ase_listener();

    // ASE config data exchange (read from ase.cfg)
    export "DPI-C" task ase_config_dex;

    // Unordered message dispatch
   `ifdef ASE_ENABLE_UMSG_FEATURE
    export "DPI-C" task umsg_dispatch;
   `endif

    // MMIO dispatch
    export "DPI-C" task mmio_dispatch;

    // Start simulation structures teardown
    import "DPI-C" context task start_simkill_countdown();

    // Signal to kill simulation
    export "DPI-C" task simkill;

    // Transaction count update ping/pong
    export "DPI-C" task count_error_flag_ping;
    import "DPI-C" function void count_error_flag_pong(int flag);

    // ASE instance check
    import "DPI-C" function int ase_instance_running();

    // Global dealloc allowed flag
    import "DPI-C" function void update_glbl_dealloc(int flag);

    // CONFIG, SCRIPT DEX operations
    import "DPI-C" function void sv2c_config_dex(string str);
    import "DPI-C" function void sv2c_script_dex(string str);

    // Data exchange for READ, WRITE system
    import "DPI-C" function void rd_memline_req_dex(inout cci_pkt pkg);
    import "DPI-C" function void rd_memline_rsp_dex(inout cci_pkt pkg);
    logic rd_memline_active;
    cci_pkt Rx0_pkt;
    RxHdr_t Rx0_hdr;

    import "DPI-C" function void wr_memline_req_dex(inout cci_pkt pkg);
    import "DPI-C" function void wr_memline_rsp_dex(inout cci_pkt pkg);
    cci_pkt Rx1_pkt;

    // MMIO response
    import "DPI-C" function void mmio_response(inout mmio_t mmio_pkt);
    mmio_t mmio_rdrsp_pkt;
    mmio_t mmio_wrrsp_pkt;

    // Software controlled process - run clocks
    export "DPI-C" task run_clocks;

    // Software controlled process - Run AFU Reset
    export "DPI-C" task afu_softreset_trig;

    // Simulator global reset (issued at simulator start, or session end)
    export "DPI-C" task ase_reset_trig;

    // Software controlled reset response
    import "DPI-C" function void sw_reset_response();

    // cci_logger buffer message
    export "DPI-C" task buffer_msg_inject;

    // Scope generator
    initial          scope_function();

    // Ready PID
    int              ase_ready_pid;

    // Finish logger command
    logic            finish_trigger = 0;

    /*
     * Credit control system
     */
    int              glbl_dealloc_credit;
    int              glbl_dealloc_credit_q;

    // Individual credit counts
    int              rd_credit;
    int              wr_credit;
    int              mmiowr_credit;
    int              mmiord_credit;
    int              umsg_credit;

    logic [ASE_RSPFIFO_COUNT_WIDTH:0]    rdrsp_fifo_cnt;
    logic [ASE_RSPFIFO_COUNT_WIDTH:0]    wrrsp_fifo_cnt;

    /*
     * CH0 and CH1 latbuf
     */
    // cf2as_latbuf_ch0 signals
    TxHdr_t                       cf2as_latbuf_tx0hdr;
    RxHdr_t                       cf2as_latbuf_rx0hdr;
    logic                         cf2as_latbuf_ch0_empty;
    logic                         cf2as_latbuf_ch0_read;
    int                           cf2as_latbuf_ch0_count;
    logic                         cf2as_latbuf_ch0_valid;

    // cf2as_latbuf_ch1 signals
    logic [CCIP_DATA_WIDTH-1:0]    cf2as_latbuf_tx1data;
    TxHdr_t                        cf2as_latbuf_tx1hdr;
    RxHdr_t                        cf2as_latbuf_rx1hdr;
    logic                          cf2as_latbuf_ch1_empty;
    logic                          cf2as_latbuf_ch1_read;
    int                            cf2as_latbuf_ch1_count;
    logic                          cf2as_latbuf_ch1_valid;

    // Hazard checker signals
    ase_haz_if haz_if;

    /*
     * ASE Simulator reset
     * - Use sparingly, only for initialization and reset between session_init(s)
     */
    task ase_reset_trig();
    begin
        reset_lockdown = 1;
        wait (glbl_dealloc_credit == 0);
        @(posedge clk);
        ase_reset = 1; 
        run_clocks(20);
        ase_reset = 0;
        run_clocks(20);
        reset_lockdown = 0;
    end
    endtask

    /*
     * Issue Simulation Finish trigger
     */
    task issue_finish_trig();
    begin
       finish_trigger = 1;
       @(posedge clk);
       finish_trigger = 0;
       @(posedge clk);
    end
    endtask

    /*
     * Multi-instance multi-user +CONFIG,+SCRIPT instrumentation
     * RUN =>
     * cd <work>
     * ./<simulator> +CONFIG=<path_to_cfg> +SCRIPT=<path_to_run_SEE_README>
     *
     */
    string config_filepath;
    string script_filepath;
`ifdef ASE_DEBUG
    initial begin
        if ($value$plusargs("CONFIG=%S", config_filepath))
        begin
           `BEGIN_YELLOW_FONTCOLOR;
           $display("  [DEBUG]  Config = %s", config_filepath);
           `END_YELLOW_FONTCOLOR;
        end
    end

    initial begin
        if ($value$plusargs("SCRIPT=%S", script_filepath))
        begin
        `BEGIN_YELLOW_FONTCOLOR;
         $display("  [DEBUG]  Script = %s", script_filepath);
        `END_YELLOW_FONTCOLOR;
        end
    end
`else
    initial $value$plusargs("CONFIG=%S", config_filepath);
    initial $value$plusargs("SCRIPT=%S", script_filepath);
`endif


    /*
     * Overflow/underflow signal checks
     */
    logic             tx0_underflow;
    logic             tx1_underflow;
    logic             tx0_overflow;
    logic             tx1_overflow;

    /*
     * Fabric Clock, pClk{*}
     */
    logic [2:0]        ase_clk_rollover = 3'b111;

    // ASE clock
    assign clk = Clk16UI;
    assign Clk16UI = ase_clk_rollover[0];
    assign Clk32UI = ase_clk_rollover[1];
    assign Clk64UI = ase_clk_rollover[2];

    // 800 Mhz internal reference clock
    initial begin : clk8ui_proc
        begin
            Clk8UI = 0;
            forever begin
                #(`CLK_8UI_TIME/2);
                Clk8UI = 0;
                #(`CLK_8UI_TIME/2);
                Clk8UI = 1;
            end
        end
    end

    // 200 Mhz clock
    always @(posedge Clk8UI) begin : clk_rollover_ctr
        ase_clk_rollover <= ase_clk_rollover - 1;
    end

    // Reset management
    logic             sw_reset_trig = 1;
    logic             app_reset_trig;
    logic             app_reset_trig_q;
    int               rst_timeout_counter;
    int               rst_counter;

    // Register app_reset_trig
    always @(posedge clk)
    begin
        app_reset_trig_q <= app_reset_trig;
    end

    // Reset states
    typedef enum {
        ResetIdle,
        ResetHoldHigh,
        ResetHoldLow,
        ResetWait
    } ResetStateEnum;
    ResetStateEnum        rst_state;

    // AFU Soft Reset Trigger
    task afu_softreset_trig(int init, int value );
    begin
        if (init) begin
            app_reset_trig = 0;
        end
        else begin
            app_reset_trig = value[0];
        end
    end
    endtask

    // Reset FSM
    always @(posedge clk)
    begin
        if (ase_reset)
        begin
            rst_state           <= ResetIdle;
            rst_counter         <= 0;
            rst_timeout_counter <= 0;
        end
        else 
        begin
            case (rst_state)
            ResetIdle:
                begin
                    rst_timeout_counter <= 0;
                    // 0 -> 1
                    if (~app_reset_trig_q && app_reset_trig)
                    begin
                        rst_state <= ResetHoldHigh;
                    end
                    // 1 -> 0
                    else if (app_reset_trig_q && ~app_reset_trig)
                    begin
                        rst_state <= ResetHoldLow;
                    end
                    else
                    begin
                        rst_state <= ResetIdle;
                    end
                end

            // Set to 1
            ResetHoldHigh:
                begin
                    if (glbl_dealloc_credit != 0)
                    begin
                        if (rst_timeout_counter < `RESET_TIMEOUT_DURATION)
                        begin
                            rst_timeout_counter <= rst_timeout_counter + 1;
                            rst_state           <= ResetHoldHigh;
                        end
                        else
                        begin
                            `BEGIN_RED_FONTCOLOR;
                            $display("  [SIM]  Reset request timed out... Behavior maybe undefined !");
                            `END_RED_FONTCOLOR;
                            sw_reset_trig <= 1;
                            rst_state <= ResetWait;
                        end
                    end
                    else
                    begin
                       sw_reset_trig <= 1;
                       rst_state <= ResetWait;
                    end
                end

            // Set to 0
            ResetHoldLow:
                begin
                    sw_reset_trig <= 0;
                    rst_state <= ResetWait;
                end

            // Wait few cycles
            ResetWait:
                begin
                    if (rst_counter < `SOFT_RESET_DURATION)
                    begin
                        rst_counter <= rst_counter + 1;
                        rst_state <= ResetWait;
                    end
                    else begin
                       rst_counter <= 0;
                       rst_state <= ResetIdle;
                       sw_reset_response();
                    end
                 end

            default:
                begin
                    rst_state <= ResetIdle;
                end

            endcase
        end
    end


    /*
     * User clock, uclk{*}
     */
    logic        usrClk;
    logic        usrClkDiv2 = 0;
    int          usrClk_delay = 3200;

    // Function: Update usrclk_delay
    function void update_usrclk_delay(int delay);
    begin
        usrClk_delay = delay;
    end
    endfunction

    // User clock process
    initial begin : usrclk_proc
        begin
            usrClk = 0;
            forever begin
                #(usrClk_delay/2);
                usrClk = ~usrClk;
            end
        end
    end


    // Div2 output
    always @(posedge usrClk) begin : usrclkdiv2_proc
        usrClkDiv2 = ~usrClkDiv2;
    end

    // UCLK interface
    assign uClk_usr     = usrClk;
    assign uClk_usrDiv2 = usrClkDiv2;


    /*
     * AFU reset - software & system resets
     */
    //
    //       0        |     0               0     |
    //       1        |     0               1     |
    //       1        |     1               0     |
    //       1        |     1               1     | Initial reset

    assign SoftReset = ase_reset | sw_reset_trig;


    /********************************************************************
     *
     * run_clocks : Run 'n' clocks
     * Software controlled event trigger for watching signals
     *
     * *****************************************************************/
    task run_clocks (int num_clks);
        int clk_iter;
    begin
        for (clk_iter = 0; clk_iter <= num_clks; clk_iter = clk_iter + 1)
        begin
            @(posedge clk);
        end
      end
    endtask

    /* ***************************************************************************
     * Buffer message injection into ccip_logger
     * ---------------------------------------------------------------------------
     * Task sets buffer message to be posted into ccip_transactions.tsv log
     *
     * ***************************************************************************/
    string buffer_msg;
    logic  buffer_msg_en;
    logic  buffer_msg_tstamp_en;

    // Inject task
    task buffer_msg_inject (int timestamp_en, string logstr);
    begin
        buffer_msg = logstr;
        buffer_msg_en = 1;
        buffer_msg_tstamp_en = timestamp_en[0];
        @(posedge clk);
        buffer_msg_en = 0;
        @(posedge clk);
    end
    endtask


    /* ******************************************************************
     *
     * MMIO block
     * CSR Write/Read is managed through this interface.
     *
     * *****************************************************************/
    // TID:Address tuple storage
    int                     unsigned tid_array[*];

    /*
     * CSR Read/Write infrastructure
     * csr_write_dispatch: A Single task to dispatch CSR Writes
     * Storage format = <wrvalid, rdvalid, hdr_width, data_width>
     *
     */
    parameter int MMIOREQ_FIFO_WIDTH = 2 + CCIP_CFG_HDR_WIDTH + CCIP_DATA_WIDTH;

    logic [MMIOREQ_FIFO_WIDTH-1:0]    mmioreq_din;
    logic                             mmioreq_write;
    logic                             mmioreq_read;
    logic                             mmioreq_valid;
    logic                             mmioreq_full;
    logic                             mmioreq_empty;
    logic [4:0]                       mmioreq_count;

    logic [CCIP_CFG_HDR_WIDTH-1:0]    cwlp_header;
    logic [CCIP_DATA_WIDTH-1:0]       cwlp_data;
    logic                             cwlp_wrvalid;
    logic                             cwlp_rdvalid;

    logic                             mmio_wrvalid;
    logic                             mmio_rdvalid;
    logic [CCIP_DATA_WIDTH-1:0]       mmio_data512;
    logic [CCIP_CFG_HDR_WIDTH-1:0]    mmio_hdrvec;

    logic                             mmio_dispatch_lock;

    // MMIO dispatch unit
    task mmio_dispatch (int initialize, mmio_t mmio_pkt);
        CfgHdr_t hdr;
    begin
        if (initialize) begin
            cwlp_wrvalid       = 0;
            cwlp_rdvalid       = 0;
            cwlp_header        = 0;
            cwlp_data          = 0;
            mmio_dispatch_lock = 0;
        end
        else begin
            while(mmio_dispatch_lock != 0);
            mmio_dispatch_lock = 1;

            @(posedge clk);
            hdr.index    = mmio_pkt.addr[CCIP_CFGHDR_ADDR_WIDTH-1:2];
            hdr.rsvd9    = 1'b0;
            hdr.tid      = mmio_pkt.tid[CCIP_CFGHDR_TID_WIDTH-1:0];

            // Set MMIO Width
            if (mmio_pkt.width == MMIO_WIDTH_32) begin
               hdr.len      = 2'b00;
            end
            else if (mmio_pkt.width == MMIO_WIDTH_64) begin
                hdr.len      = 2'b01;
            end
            else if (mmio_pkt.width == MMIO_WIDTH_512) begin
                hdr.len      = 2'b10;
            end
        
            // Set MMIO Read/Write behavior
            if (mmio_pkt.write_en == MMIO_WRITE_REQ)
            begin
                if (mmio_pkt.width == MMIO_WIDTH_32) begin
                    cwlp_data = {480'b0, mmio_pkt.qword[0][31:0]};
                end
                else if (mmio_pkt.width == MMIO_WIDTH_64) begin
                    cwlp_data = {448'b0, mmio_pkt.qword[0][63:0]};
                end
                else if (mmio_pkt.width == MMIO_WIDTH_512) begin
                    cwlp_data = {mmio_pkt.qword[7], mmio_pkt.qword[6],
                                 mmio_pkt.qword[5], mmio_pkt.qword[4],
                                 mmio_pkt.qword[3], mmio_pkt.qword[2],
                                 mmio_pkt.qword[1], mmio_pkt.qword[0]};
                end
                cwlp_header = logic_cast_CfgHdr_t'(hdr);
                cwlp_wrvalid = 1;
                cwlp_rdvalid = 0;
                mmio_pkt.resp_en = 1;
            end
            else if (mmio_pkt.write_en == MMIO_READ_REQ)
            begin
                cwlp_data    = 0;
                cwlp_header  = logic_cast_CfgHdr_t'(hdr);
                cwlp_wrvalid = 0;
                cwlp_rdvalid = 1;
                mmio_pkt.resp_en = 1;
            end

            @(posedge clk);
            cwlp_wrvalid = 0;
            cwlp_rdvalid = 0;

            @(posedge clk);
            mmio_dispatch_lock = 0;
            run_clocks (`MMIO_LATENCY);
        end
    end
    endtask

    // CSR readreq/write FIFO data
    assign mmioreq_din = {cwlp_wrvalid, cwlp_rdvalid, cwlp_header, cwlp_data};
    assign mmioreq_write = cwlp_wrvalid | cwlp_rdvalid;

    // Request staging
    ase_svfifo
     #(
       .DATA_WIDTH     ( MMIOREQ_FIFO_WIDTH ),
       .DEPTH_BASE2    ( 4 ),
       .ALMFULL_THRESH ( 12 )
       )
    mmioreq_fifo
     (
      .clk        ( clk ),
      .rst        ( ase_reset ),
      .wr_en      ( mmioreq_write ),
      .data_in    ( mmioreq_din ),
      .rd_en      ( mmioreq_read ),
      .data_out   ( {mmio_wrvalid, mmio_rdvalid, mmio_hdrvec, mmio_data512} ),
      .data_out_v ( mmioreq_valid ),
      .alm_full   ( mmioreq_full ),
      .full       (  ),
      .empty      ( mmioreq_empty ),
      .count      ( mmioreq_count ),
      .overflow   (  ),
      .underflow  (  )
      );

    // Debug config header
    CfgHdr_t       DBG_cfgheader;
    logic          DBG_cfgvld;
    assign DBG_cfgheader = CfgHdr_t'(C0RxHdr);
    assign DBG_cfgvld = C0RxMmioWrValid | C0RxMmioRdValid;


    /*
     * MMIO Read response
    */
    parameter int MMIORESP_FIFO_WIDTH = CCIP_MMIO_TID_WIDTH + CCIP_MMIO_RDDATA_WIDTH;

    logic [CCIP_MMIO_RDDATA_WIDTH-1:0] mmioresp_dout;
    logic [CCIP_MMIO_TID_WIDTH-1:0]    mmioresp_tid;

    logic                              mmioresp_read;
    logic                              mmioresp_valid;
    logic                              mmioresp_full;
    logic                              mmioresp_empty;
    logic [3:0]                        mmioresp_count;

    // Response staging FIFO
    ase_svfifo
     #(
       .DATA_WIDTH     ( MMIORESP_FIFO_WIDTH ),
       .DEPTH_BASE2    ( 3 ),
       .ALMFULL_THRESH ( 5 )
       )
    mmioresp_fifo
     (
      .clk        ( clk ),
      .rst        ( ase_reset ),
      .wr_en      ( C2TxMmioRdValid ),
      .data_in    ( {logic_cast_MMIOHdr_t'(C2TxHdr), C2TxData} ),
      .rd_en      ( mmioresp_read & ~mmioresp_empty ),
      .data_out   ( {mmioresp_tid, mmioresp_dout} ),
      .data_out_v ( mmioresp_valid ),
      .alm_full   ( mmioresp_full ),
      .full       (  ),
      .empty      ( mmioresp_empty ),
      .count      ( mmioresp_count ),
      .overflow   (  ),
      .underflow  (  )
      );

    // MMIO Response mask (act by reference)
    function automatic void mmio_rdrsp_mask( );
    begin
        // TID
        mmio_rdrsp_pkt.tid      = mmioresp_tid;
        // Write Enable
        mmio_rdrsp_pkt.write_en = MMIO_READ_REQ;
        // Data (use only lower int)
        mmio_rdrsp_pkt.qword[0]       = mmioresp_dout;
        if (mmio_rdrsp_pkt.width == 32) begin
            mmio_rdrsp_pkt.qword[0][63:32] = 32'b0;
        end
        mmio_rdrsp_pkt.qword[1] = 0;
        mmio_rdrsp_pkt.qword[2] = 0;
        mmio_rdrsp_pkt.qword[3] = 0;
        mmio_rdrsp_pkt.qword[4] = 0;
        mmio_rdrsp_pkt.qword[5] = 0;
        mmio_rdrsp_pkt.qword[6] = 0;
        mmio_rdrsp_pkt.qword[7] = 0;
        // Response flag
        mmio_rdrsp_pkt.resp_en  = 1;
        // Return
        mmio_response ( mmio_rdrsp_pkt );
    end
    endfunction

    // MMIO Response trigger
    always @(posedge clk) begin : mmio_read_proc
        mmioresp_read <= ~mmioresp_empty;
    end

    // FIFO writes to memory
    always @(posedge clk) begin : dpi_mmio_response
        if (mmioresp_valid) begin
            mmio_rdrsp_mask ();
        end
    end

    /*
     * MMIO Write response
     */
    // Function to tie Write Response
    function automatic void mmio_wrrsp_mask();
    begin
        // TID
        mmio_wrrsp_pkt.tid      = DBG_cfgheader.tid;

        // Write Enable
        mmio_wrrsp_pkt.write_en = MMIO_WRITE_REQ;

        // Data packing
        mmio_wrrsp_pkt.qword[0] = C0RxData[  63:00  ];
        mmio_wrrsp_pkt.qword[1] = C0RxData[ 127:64  ];
        mmio_wrrsp_pkt.qword[2] = C0RxData[ 191:128 ];
        mmio_wrrsp_pkt.qword[3] = C0RxData[ 255:192 ];
        mmio_wrrsp_pkt.qword[4] = C0RxData[ 319:256 ];
        mmio_wrrsp_pkt.qword[5] = C0RxData[ 383:320 ];
        mmio_wrrsp_pkt.qword[6] = C0RxData[ 447:384 ];
        mmio_wrrsp_pkt.qword[7] = C0RxData[ 511:448 ];

        // Address
        mmio_wrrsp_pkt.addr = DBG_cfgheader.index;

        // Response flag
        mmio_wrrsp_pkt.resp_en  = 1;

        // Return
        mmio_response ( mmio_wrrsp_pkt );
    end
    endfunction

    // Response to MMIO write (credit control only)
    always @(posedge clk) begin
        if (C0RxMmioWrValid) begin
            mmio_wrrsp_mask( );
        end
    end


    /* ******************************************************************
     *
     * Unordered Messages Engine
     * umsg_dispatch: Single push process triggering UMSG machinery
     * This feature is only available in integrated configuration
     *
     * *****************************************************************/
    logic           umsgfifo_write;
    logic           umsgfifo_read;
    logic           umsgfifo_valid;
    logic           umsgfifo_full;
    logic           umsgfifo_empty;

    // Enabled only in integrated configuration
`ifdef ASE_ENABLE_UMSG_FEATURE
    parameter int UMSG_FIFO_WIDTH = CCIP_RX_HDR_WIDTH + CCIP_DATA_WIDTH;

    UMsgHdr_t                         umsgfifo_hdr_in;
    logic [CCIP_DATA_WIDTH-1:0]       umsgfifo_data_in;

    logic [CCIP_DATA_WIDTH-1:0]       umsgfifo_data_out;
    logic [ASE_UMSG_HDR_WIDTH-1:0]    umsgfifo_hdrvec_out;


    // Data store
    logic [CCIP_DATA_WIDTH-1:0]       umsg_latest_data_array [0:NUM_UMSG_PER_AFU-1];

    // Umsg engine
    umsg_t umsg_array[NUM_UMSG_PER_AFU];

    // UMSG dispatch function
    task umsg_dispatch (int init, umsgcmd_t umsg_pkt);
        int ii;
    begin
        if (init) begin
            for (ii = 0; ii < NUM_UMSG_PER_AFU; ii = ii + 1) begin
                umsg_latest_data_array[ii]   <= {CCIP_DATA_WIDTH{1'b0}};
                umsg_array[ii].line_accessed <= 0;
                umsg_array[ii].hint_enable   <= 0;
            end
        end
        else begin
            umsg_array[ umsg_pkt.id ].line_accessed = 1;
            umsg_array[ umsg_pkt.id ].hint_enable   = umsg_pkt.hint;
            umsg_latest_data_array[umsg_pkt.id][  63:00  ] = umsg_pkt.qword[0] ;
            umsg_latest_data_array[umsg_pkt.id][ 127:64  ] = umsg_pkt.qword[1] ;
            umsg_latest_data_array[umsg_pkt.id][ 191:128 ] = umsg_pkt.qword[2] ;
            umsg_latest_data_array[umsg_pkt.id][ 255:192 ] = umsg_pkt.qword[3] ;
            umsg_latest_data_array[umsg_pkt.id][ 319:256 ] = umsg_pkt.qword[4] ;
            umsg_latest_data_array[umsg_pkt.id][ 383:320 ] = umsg_pkt.qword[5] ;
            umsg_latest_data_array[umsg_pkt.id][ 447:384 ] = umsg_pkt.qword[6] ;
            umsg_latest_data_array[umsg_pkt.id][ 511:448 ] = umsg_pkt.qword[7] ;
            run_clocks(1);
            umsg_array[ umsg_pkt.id ].line_accessed = 0;
        end
    end
    endtask

    // Umsg slot/hint selector
    int                           umsg_data_slot;
    int                           umsg_hint_slot;
    int                           umsg_data_slot_old = 255;
    int                           umsg_hint_slot_old = 255;

    logic [0:NUM_UMSG_PER_AFU-1]  umsg_hint_enable_array;
    logic [0:NUM_UMSG_PER_AFU-1]  umsg_data_enable_array;

    logic [4:0]                   umsgfifo_cnt_tmp;
    logic [4:0]                   umsgfifo_cnt;

    // UMSG Hint-to-Data time emulator (toaster style)
    // New Umsg hints to same location are ignored
    // If Data is same, hints dont get generated

    generate
        for (genvar ii = 0; ii < NUM_UMSG_PER_AFU; ii = ii + 1 ) begin : umsg_engine

            // Status board
            always @(*) begin
                umsg_hint_enable_array[ii] <= umsg_array[ii].hint_ready;
                umsg_data_enable_array[ii] <= umsg_array[ii].data_ready;
            end

            // State machine
            always @(posedge clk) begin
                if (ase_reset) begin
                    umsg_array[ii].hint_timer <= 0;
                    umsg_array[ii].data_timer <= 0;
                    umsg_array[ii].hint_ready <= 0;
                    umsg_array[ii].data_ready <= 0;
                    umsg_array[ii].state      <= UMsgIdle;
                end
                else begin
                    case (umsg_array[ii].state)

                    UMsgIdle:  // Wait here until activated
                    begin
                        umsg_array[ii].hint_ready <= 0;
                        umsg_array[ii].data_ready <= 0;
                        if (umsg_array[ii].line_accessed && umsg_array[ii].hint_enable) begin
                            umsg_array[ii].hint_timer <= $urandom_range(`UMSG_START2HINT_LATRANGE);
                            umsg_array[ii].data_timer <= 0;
                            umsg_array[ii].state      <= UMsgHintWait;
                        end
                        else if (umsg_array[ii].line_accessed && ~umsg_array[ii].hint_enable) begin
                            umsg_array[ii].hint_timer <= 0;
                            umsg_array[ii].data_timer <= $urandom_range(`UMSG_START2DATA_LATRANGE);
                            umsg_array[ii].state      <= UMsgDataWait;
                        end
                        else begin
                            umsg_array[ii].hint_timer <= 0;
                            umsg_array[ii].data_timer <= 0;
                            umsg_array[ii].state      <= UMsgIdle;
                        end
                    end

                    // Wait to send out hint, go to UMsgSendHint after t_hint ticks
                    UMsgHintWait:
                    begin
                        umsg_array[ii].hint_ready <= 0;
                        umsg_array[ii].data_ready <= 0;
                        umsg_array[ii].data_timer <= 0;
                        if (umsg_array[ii].hint_timer <= 0) begin
                            umsg_array[ii].state      <= UMsgSendHint;
                        end
                        else begin
                            umsg_array[ii].hint_timer <= umsg_array[ii].hint_timer - 1;
                            umsg_array[ii].state      <= UMsgHintWait;
                        end
                    end

                    // Wait until hint popped by event queue
                    UMsgSendHint:
                    begin
                        umsg_array[ii].hint_timer <= 0;
                        umsg_array[ii].data_ready <= 0;
                        if (umsg_array[ii].hint_pop) begin
                            umsg_array[ii].hint_ready <= 0;
                            umsg_array[ii].data_timer <= $urandom_range(`UMSG_HINT2DATA_LATRANGE);
                            umsg_array[ii].state      <= UMsgDataWait;
                        end
                        else begin
                            umsg_array[ii].hint_ready <= 1;
                            umsg_array[ii].data_timer <= 0;
                            umsg_array[ii].state      <= UMsgSendHint;
                        end
                    end

                    // Wait to send out data, go to UMsgSendData after t_data ticks
                    UMsgDataWait:
                    begin
                        umsg_array[ii].hint_timer <= 0;
                        umsg_array[ii].hint_ready    <= 0;
                        umsg_array[ii].data_ready    <= 0;
                        if (umsg_array[ii].data_timer <= 0) begin
                            umsg_array[ii].state      <= UMsgSendData;
                        end
                        else begin
                            umsg_array[ii].data_timer <= umsg_array[ii].data_timer - 1;
                            umsg_array[ii].state      <= UMsgDataWait;
                        end
                    end

                    // Wait until popped by event queue
                    UMsgSendData:
                    begin
                        umsg_array[ii].hint_timer <= 0;
                        umsg_array[ii].data_timer <= 0;
                        if (umsg_array[ii].data_pop) begin
                            umsg_array[ii].hint_ready <= 0;
                            umsg_array[ii].data_ready <= 0;
                            umsg_array[ii].state      <= UMsgIdle;
                        end
                        else begin
                            umsg_array[ii].hint_ready <= 0;
                            umsg_array[ii].data_ready <= 1;
                            umsg_array[ii].state      <= UMsgSendData;
                        end
                    end

                    // lala-land
                    default:
                    begin
                        umsg_array[ii].hint_timer <= 0;
                        umsg_array[ii].data_timer <= 0;
                        umsg_array[ii].hint_ready <= 0;
                        umsg_array[ii].data_ready <= 0;
                        umsg_array[ii].state      <= UMsgIdle;
                    end
                    endcase
                end
            end

        end
    endgenerate


    // Find UMSG Hintable slot
    function int find_umsg_hint ();
        int ret_hint_slot;
        int slot;
        int start_iter;
        int end_iter;
    begin
        start_iter = 0;
        end_iter   = start_iter + NUM_UMSG_PER_AFU;
        ret_hint_slot = 255;
        for (slot = start_iter ; slot < end_iter ; slot = slot + 1) begin
            if (umsg_array[slot].hint_ready && ~umsg_array[slot].data_ready) begin
                ret_hint_slot = slot;
                umsg_hint_slot_old = ret_hint_slot;
                break;
            end
        end
        return ret_hint_slot;
    end
    endfunction

    // Find UMSG Data slot to send
    function int find_umsg_data();
        int ret_data_slot;
        int slot;
        int start_iter;
        int end_iter;
    begin
        start_iter = 0;
        end_iter   = start_iter + NUM_UMSG_PER_AFU;
        ret_data_slot = 255;
        for (slot = start_iter ; slot < end_iter ; slot = slot + 1) begin
            if (umsg_array[slot].data_ready) begin
                ret_data_slot = slot;
                umsg_data_slot_old = ret_data_slot;
                break;
            end
        end
        return ret_data_slot;
    end
    endfunction

    // Calculate slots for UMSGs
    always @(posedge clk) begin : umsg_slot_finder_proc
        umsg_data_slot <= find_umsg_data();
        umsg_hint_slot <= find_umsg_hint();
    end

    // Pop HINT/DATA
    typedef enum {UPopIdle, UPopHint, UPopData, UPopWait, UPopSleep} UmsgPopStateMachine;
    UmsgPopStateMachine upop_state;

    always @(posedge clk) begin : umsgfifo_pop_write
        if (ase_reset) begin
            umsgfifo_hdr_in    <= {ASE_UMSG_HDR_WIDTH{1'b0}};
            umsgfifo_data_in   <= {UMSG_FIFO_WIDTH{1'b0}};
            umsgfifo_write     <= 0;
            for(int jj = 0; jj < NUM_UMSG_PER_AFU; jj = jj + 1) begin
                umsg_array[jj].hint_pop <= 0;
                umsg_array[jj].data_pop <= 0;
            end
            upop_state <= UPopIdle;
        end
        else begin
            case (upop_state)
            // UMsg Pop idle
            UPopIdle:
            begin
                umsgfifo_write     <= 0;
                for(int jj = 0; jj < NUM_UMSG_PER_AFU; jj = jj + 1) begin
                    umsg_array[jj].hint_pop <= 0;
                    umsg_array[jj].data_pop <= 0;
                end
                if (~umsgfifo_full && (umsg_hint_slot != 255)) begin
                    upop_state <= UPopHint;
                end
                else if (~umsgfifo_full && (umsg_data_slot != 255)) begin
                    upop_state <= UPopData;
                end
                else begin
                    upop_state <= UPopIdle;
                end
            end

            // Pop UMsg hint
            UPopHint:
            begin
                umsgfifo_hdr_in.rsvd25              <= 0;
                umsgfifo_hdr_in.resp_type           <= ASE_UMSG;
                umsgfifo_hdr_in.umsg_type           <= 1;
                umsgfifo_hdr_in.umsg_id             <= umsg_hint_slot;
                umsgfifo_data_in                    <= {CCIP_DATA_WIDTH{1'b0}};
                umsgfifo_write                      <= 1;
                umsg_array[umsg_hint_slot].hint_pop <= 1;
                upop_state                          <= UPopWait;
            end

            // Pop Umsg data
            UPopData:
            begin
                umsgfifo_hdr_in.rsvd25              <= 0;
                umsgfifo_hdr_in.resp_type           <= ASE_UMSG;
                umsgfifo_hdr_in.umsg_type           <= 0;
                umsgfifo_hdr_in.umsg_id             <= umsg_data_slot;
                umsgfifo_data_in                    <= umsg_latest_data_array[umsg_data_slot];
                umsgfifo_write                      <= 1;
                umsg_array[umsg_data_slot].data_pop <= 1;
                upop_state                          <= UPopWait;
            end

            // UMsg wait machine
            UPopWait:
            begin
                umsgfifo_write     <= 0;
                for(int jj = 0; jj < NUM_UMSG_PER_AFU; jj = jj + 1) begin
                    umsg_array[jj].hint_pop <= 0;
                    umsg_array[jj].data_pop <= 0;
                end
                upop_state         <= UPopSleep;
            end

            // Stabilize, before moving on
            UPopSleep:
            begin
                umsgfifo_write     <= 0;
                upop_state         <= UPopIdle;
            end

            default:
            begin
                umsgfifo_write             <= 0;
                for(int jj = 0; jj < NUM_UMSG_PER_AFU; jj = jj + 1) begin
                    umsg_array[jj].hint_pop <= 0;
                    umsg_array[jj].data_pop <= 0;
                end
                upop_state <= UPopIdle;
            end
            endcase
        end
    end

    // UMSG events queue
    ase_svfifo
     #(
       .DATA_WIDTH     (UMSG_FIFO_WIDTH),
       .DEPTH_BASE2    (4),
       .ALMFULL_THRESH (12)
       )
    umsg_fifo
     (
      .clk        ( clk ),
      .rst        ( ase_reset ),
      .wr_en      ( umsgfifo_write ),
      .data_in    ( { logic_cast_UMsgHdr_t'(umsgfifo_hdr_in), umsgfifo_data_in} ),
      .rd_en      ( umsgfifo_read & ~umsgfifo_empty ),
      .data_out   ( { umsgfifo_hdrvec_out, umsgfifo_data_out} ),
      .data_out_v ( umsgfifo_valid ),
      .alm_full   ( umsgfifo_full ),
      .full       (  ),
      .empty      ( umsgfifo_empty ),
      .count      ( umsgfifo_cnt_tmp ),
      .overflow   (  ),
      .underflow  (  )
      );

    // UMSG Debug header
    UMsgHdr_t C0RxUMsgHdr;
    assign C0RxUMsgHdr = UMsgHdr_t'(C0RxHdr);

    // Register UMSG fifo count
    always @(posedge clk) begin
        if (ase_reset) begin
            umsgfifo_cnt <= 0;
        end
        else begin
            umsgfifo_cnt <= umsgfifo_cnt_tmp;
        end
    end
`endif //  `ifdef ASE_ENABLE_UMSG_FEATURE


    /* ******************************************************************
     *
     * Config data exchange - Supplied by ase.cfg
     * Configuration of ASE managed by a text file, modifiable runtime
     *
     * *****************************************************************/
    task ase_config_dex(ase_cfg_t cfg_in);
    begin
        // Cfg transfer
        cfg.ase_mode                 = cfg_in.ase_mode                 ;
        cfg.ase_timeout              = cfg_in.ase_timeout              ;
        cfg.ase_num_tests            = cfg_in.ase_num_tests            ;
        cfg.enable_reuse_seed        = cfg_in.enable_reuse_seed        ;
        cfg.ase_seed                 = cfg_in.ase_seed                 ;
        cfg.enable_cl_view           = cfg_in.enable_cl_view           ;
        cfg.usr_tps                  = cfg_in.usr_tps                  ;
        cfg.phys_memory_available_gb = cfg_in.phys_memory_available_gb ;

        // Set UsrClk
        update_usrclk_delay( cfg.usr_tps );
    end
    endtask


    /* ******************************************************************
     * Count transactions
     * Live count of transactions to be printed at end of simulation
     *
     * ******************************************************************/
    // MMIO Activity counts
    int ase_rx0_mmiowrreq_cnt ;
    int ase_rx0_mmiordreq_cnt ;
    int ase_tx2_mmiordrsp_cnt ;
    // Read counts
    int ase_tx0_rdvalid_cnt   ;
    int ase_rx0_rdvalid_cnt   ;
    // Write counts
    int ase_tx1_wrvalid_cnt   ;
    int ase_rx1_wrvalid_cnt   ;
    // Write Fence counts
    int ase_tx1_wrfence_cnt   ;
    int ase_rx1_wrfence_cnt   ;
    // Umsg counts (only available in integrated configuration)
`ifdef ASE_ENABLE_UMSG_FEATURE
    int ase_rx0_umsghint_cnt  ;
    int ase_rx0_umsgdata_cnt  ;
`endif
    // Interrupt counts (only available in discrete configuration)
`ifdef ASE_ENABLE_INTR_FEATURE
    int ase_tx1_intrreq_cnt   ;
    int ase_rx1_intrrsp_cnt   ;
`endif


    // Remap UmsgHdr for count purposes
`ifdef ASE_ENABLE_UMSG_FEATURE
    UMsgHdr_t ase_umsghdr_map;
    assign ase_umsghdr_map = UMsgHdr_t'(C0RxHdr);
`endif

    // Count increment macro
`define incr_cnt(condition, counter_val)\
    if (condition == 1) counter_val <= counter_val + 1


    /*
     * Transaction counts
     */
    // Channel count structures
    txn_vc_counts   rdreq_vc_cnt  , rdrsp_vc_cnt;
    txn_vc_counts   wrreq_vc_cnt  , wrrsp_vc_cnt;
    txn_vc_counts   wrfreq_vc_cnt , wrfrsp_vc_cnt;

    txn_mcl_counts   rdreq_mcl_cnt  ;
    txn_mcl_counts   wrreq_mcl_cnt  , wrrsp_mcl_cnt;

    // Transaction count process
    always @(posedge clk) begin : transact_cnt_proc
        // ===================================================== //
        // Intiialization
        // ===================================================== //
        if (init_reset) begin
            // ------------------------------------ //
            // MMIO
            // ------------------------------------ //
            ase_rx0_mmiowrreq_cnt <= 0 ;
            ase_rx0_mmiordreq_cnt <= 0 ;
            ase_tx2_mmiordrsp_cnt <= 0 ;
            // ------------------------------------ //
            // Umsg counts
            // ------------------------------------ //
`ifdef ASE_ENABLE_UMSG_FEATURE
            ase_rx0_umsghint_cnt <= 0 ;
            ase_rx0_umsgdata_cnt <= 0 ;
`endif
            // ------------------------------------ //
            // Read transactions
            // ------------------------------------ //
            rdreq_vc_cnt  <= '{0, 0, 0, 0};
            rdrsp_vc_cnt  <= '{0, 0, 0, 0};
            rdreq_mcl_cnt <= '{0, 0, 0};
            // Total read counts
            ase_tx0_rdvalid_cnt <= 0 ;
            ase_rx0_rdvalid_cnt <= 0 ;
            // ------------------------------------ //
            // Write transactions
            // ------------------------------------ //
            wrreq_vc_cnt  <= '{0, 0, 0, 0};
            wrrsp_vc_cnt  <= '{0, 0, 0, 0};
            wrreq_mcl_cnt <= '{0, 0, 0};
            wrrsp_mcl_cnt <= '{0, 0, 0};
            // Total write counts
            ase_tx1_wrvalid_cnt <= 0 ;
            ase_rx1_wrvalid_cnt <= 0 ;
            // ------------------------------------ //
            // WriteFence transactions
            // ------------------------------------ //
            wrfreq_vc_cnt  <= '{0, 0, 0, 0};
            wrfrsp_vc_cnt  <= '{0, 0, 0, 0};
            // WriteFence Counts
            ase_tx1_wrfence_cnt <= 0 ;
            ase_rx1_wrfence_cnt <= 0 ;
            // ------------------------------------ //
            // Interrupt transactions
            // ------------------------------------ //
           `ifdef ASE_ENABLE_INTR_FEATURE
            ase_tx1_intrreq_cnt <= 0;
            ase_rx1_intrrsp_cnt <= 0;
           `endif
        end // if (init_reset)

        // ===================================================== //
        // Active counts
        // ===================================================== //
        else begin
            // ------------------------------------ //
            // MMIO counts
            // ------------------------------------ //
           `incr_cnt (C0RxMmioWrValid, ase_rx0_mmiowrreq_cnt);
           `incr_cnt (C0RxMmioRdValid, ase_rx0_mmiordreq_cnt);
           `incr_cnt (C2TxMmioRdValid, ase_tx2_mmiordrsp_cnt);
            // ------------------------------------ //
            // UMsg counts
            // ------------------------------------ //
           `ifdef ASE_ENABLE_UMSG_FEATURE
               `incr_cnt ( (C0RxUMsgValid && ase_umsghdr_map.umsg_type) , ase_rx0_umsghint_cnt);
               `incr_cnt ( (C0RxUMsgValid && ~ase_umsghdr_map.umsg_type), ase_rx0_umsgdata_cnt);
           `endif
            // ------------------------------------ //
            // Interrupt counts
            // ------------------------------------ //
           `ifdef ASE_ENABLE_INTR_FEATURE
               `incr_cnt ( (C1TxValid    && isIntrRequest(C1TxHdr)) , ase_tx1_intrreq_cnt);
               `incr_cnt ( (C1RxRspValid && isIntrResponse(C1RxHdr)), ase_rx1_intrrsp_cnt);
           `endif
            // ------------------------------------ //
            // Read counts
            // ------------------------------------ //
            // Total counts
            if (C0TxValid && isReadRequest(C0TxHdr))
                ase_tx0_rdvalid_cnt <= ase_tx0_rdvalid_cnt + (C0TxHdr.len + 1);
           `incr_cnt ( (C0RxRspValid && isReadResponse(C0RxHdr)), ase_rx0_rdvalid_cnt);
            // C0Tx granular counts
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.vc == VC_VA )), rdreq_vc_cnt.va);
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.vc == VC_VL0)), rdreq_vc_cnt.vl0);
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.vc == VC_VH0)), rdreq_vc_cnt.vh0);
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.vc == VC_VH1)), rdreq_vc_cnt.vh1);
            // C0Tx MCL granular counts
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.len == ASE_1CL)), rdreq_mcl_cnt.mcl0);
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.len == ASE_2CL)), rdreq_mcl_cnt.mcl1);
           `incr_cnt ( (C0TxValid && isReadRequest(C0TxHdr) && (C0TxHdr.len == ASE_4CL)), rdreq_mcl_cnt.mcl3);
            // C0Rx VC granular counts
           `incr_cnt ( (C0RxRspValid && isReadResponse(C0RxHdr) && (C0RxHdr.vc_used == VC_VA)), rdrsp_vc_cnt.va);
           `incr_cnt ( (C0RxRspValid && isReadResponse(C0RxHdr) && (C0RxHdr.vc_used == VC_VL0)), rdrsp_vc_cnt.vl0);
           `incr_cnt ( (C0RxRspValid && isReadResponse(C0RxHdr) && (C0RxHdr.vc_used == VC_VH0)), rdrsp_vc_cnt.vh0);
           `incr_cnt ( (C0RxRspValid && isReadResponse(C0RxHdr) && (C0RxHdr.vc_used == VC_VH1)), rdrsp_vc_cnt.vh1);
            // ------------------------------------ //
            // Write counts
            // ------------------------------------ //
           `incr_cnt ( (C1TxValid && isWriteRequest(C1TxHdr))   , ase_tx1_wrvalid_cnt);
            if (C1RxRspValid && isWriteResponse(C1RxHdr)) begin
                if (isVL0Response(C1RxHdr)) begin
                    ase_rx1_wrvalid_cnt <= ase_rx1_wrvalid_cnt + 1;
                end
                else if (isVHxResponse(C1RxHdr) && C1RxHdr.format) begin
                    ase_rx1_wrvalid_cnt <= ase_rx1_wrvalid_cnt + (C1RxHdr.clnum + 1);
                end
            end
            // C1Tx VC granular counts
           `incr_cnt ( (C1TxValid && isWriteRequest(C1TxHdr) && (C1TxHdr.vc == VC_VA) ), wrreq_vc_cnt.va);
           `incr_cnt ( (C1TxValid && isWriteRequest(C1TxHdr) && (C1TxHdr.vc == VC_VL0)), wrreq_vc_cnt.vl0);
           `incr_cnt ( (C1TxValid && isWriteRequest(C1TxHdr) && (C1TxHdr.vc == VC_VH0)), wrreq_vc_cnt.vh0);
           `incr_cnt ( (C1TxValid && isWriteRequest(C1TxHdr) && (C1TxHdr.vc == VC_VH1)), wrreq_vc_cnt.vh1);
            // C1Tx MCL granular counts
           `incr_cnt ( (C1TxValid && C1TxHdr.sop && isWriteRequest(C1TxHdr) && (C1TxHdr.len == ASE_1CL)), wrreq_mcl_cnt.mcl0);
           `incr_cnt ( (C1TxValid && C1TxHdr.sop && isWriteRequest(C1TxHdr) && (C1TxHdr.len == ASE_2CL)), wrreq_mcl_cnt.mcl1);
           `incr_cnt ( (C1TxValid && C1TxHdr.sop && isWriteRequest(C1TxHdr) && (C1TxHdr.len == ASE_4CL)), wrreq_mcl_cnt.mcl3);
            // C1Rx VC granular counts
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VA)), wrrsp_vc_cnt.va);
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VL0)), wrrsp_vc_cnt.vl0);
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VH0)), wrrsp_vc_cnt.vh0);
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VH1)), wrrsp_vc_cnt.vh1);
            // C1Tx MCL granular counts
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (~C1RxHdr.format||(C1RxHdr.format && (C1RxHdr.clnum == ASE_1CL)))), wrrsp_mcl_cnt.mcl0);
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.clnum == ASE_2CL) && C1RxHdr.format), wrrsp_mcl_cnt.mcl1);
           `incr_cnt ( (C1RxRspValid && isWriteResponse(C1RxHdr) && (C1RxHdr.clnum == ASE_4CL) && C1RxHdr.format), wrrsp_mcl_cnt.mcl3);
            // ------------------------------------ //
            // WriteFence counts
            // ------------------------------------ //
           `incr_cnt ( (C1TxValid && isWrFenceRequest(C1TxHdr))    , ase_tx1_wrfence_cnt);
           `incr_cnt ( (C1RxRspValid && isWrFenceResponse(C1RxHdr)), ase_rx1_wrfence_cnt);
            // C1Tx WrF VC granular counts
           `incr_cnt ( (C1TxValid && isWrFenceRequest(C1TxHdr) && (C1TxHdr.vc == VC_VA) ), wrfreq_vc_cnt.va );
           `incr_cnt ( (C1TxValid && isWrFenceRequest(C1TxHdr) && (C1TxHdr.vc == VC_VL0)), wrfreq_vc_cnt.vl0);
           `incr_cnt ( (C1TxValid && isWrFenceRequest(C1TxHdr) && (C1TxHdr.vc == VC_VH0)), wrfreq_vc_cnt.vh0);
           `incr_cnt ( (C1TxValid && isWrFenceRequest(C1TxHdr) && (C1TxHdr.vc == VC_VH1)), wrfreq_vc_cnt.vh1);
            // C1Rx WrF VC granular counts
           `incr_cnt ( (C1RxRspValid && isWrFenceResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VA) ), wrfrsp_vc_cnt.va );
           `incr_cnt ( (C1RxRspValid && isWrFenceResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VL0)), wrfrsp_vc_cnt.vl0 );
           `incr_cnt ( (C1RxRspValid && isWrFenceResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VH0)), wrfrsp_vc_cnt.vh0 );
           `incr_cnt ( (C1RxRspValid && isWrFenceResponse(C1RxHdr) && (C1RxHdr.vc_used == VC_VH1)), wrfrsp_vc_cnt.vh1 );
        end
    end

    /*
     * Count error flag
     */
    int count_error_flag;
    always @(posedge clk) begin
        if (init_reset) begin
            count_error_flag <= 0;
        end
        else begin
            if (ase_tx0_rdvalid_cnt != ase_rx0_rdvalid_cnt)
                count_error_flag <= 1;
            else if (ase_tx1_wrvalid_cnt != ase_rx1_wrvalid_cnt)
                count_error_flag <= 1;
            else if (ase_tx2_mmiordrsp_cnt != ase_rx0_mmiordreq_cnt)
                count_error_flag <= 1;
            else if (ase_tx1_wrfence_cnt != ase_rx1_wrvalid_cnt)
                count_error_flag <= 1;
            else
                count_error_flag <= 0;
        end
    end // always @ (posedge clk)


    // Ping to get error flag
    task count_error_flag_ping();
    begin
        count_error_flag_pong(glbl_dealloc_credit);
    end
    endtask


    /* *******************************************************************
     *
     * Unified message watcher daemon
     * - Looks for MMIO Requests, buffer requests
     *
     * *******************************************************************/
    always @(posedge clk) begin : daemon_proc
        ase_listener();
    end


    /* *******************************************************************
     *
     * TX to RX channel FULFILLMENT
     *
     * -------------------------------------------------------------------
     * stg0       | stg1       | stg2
     * -------------------------------------
     * latbuf_out | cast & DEX | Response
     *            | tx_pkt     | tx_pkt_q
     *
     * *******************************************************************/
    // Read response staging signals
    logic [CCIP_RX_HDR_WIDTH-1:0]    rdrsp_hdr_out_vec;
    logic [CCIP_DATA_WIDTH-1:0]      rdrsp_data_in, rdrsp_data_out;
    RxHdr_t                          rdrsp_hdr_in, rdrsp_hdr_out;
    logic                            rdrsp_write;
    logic                            rdrsp_read;
    logic                            rdrsp_full;
    logic                            rdrsp_empty;
    logic                            rdrsp_valid;

    // Atomics response staging signals
    // logic [CCIP_RX_HDR_WIDTH-1:0] atomics_hdr_out_vec;
    // logic [CCIP_DATA_WIDTH-1:0]   atomics_data_in, atomics_data_out;
    // Atomics_t                     atomics_hdr_in, atomics_hdr_out;
    // logic                         atomics_write;
    // logic                         atomics_read;
    // logic                         atomics_full;
    // logic                         atomics_empty;
    // logic                         atomics_valid;

    // Pre-packed signals
    logic [CCIP_RX_HDR_WIDTH-1:0]    wrrsp_hdr_out_vec;
    RxHdr_t                          wrrsp_hdr_in, wrrsp_hdr_out;
    logic                            wrrsp_write;
    logic                            wrrsp_read;
    logic                            wrrsp_full;
    logic                            wrrsp_empty;
    logic                            wrrsp_valid;

    // Write response 1 staging signals
    // logic [CCIP_RX_HDR_WIDTH-1:0] pp_wrrsp_hdr_out_vec;
    RxHdr_t                          pp_wrrsp_hdr;
    logic                            pp_wrrsp_write;


    /*
     * FUNCTION: Cast TxHdr_t to cci_pkt
     */
    function automatic void cast_txhdr_to_ccipkt (ref   cci_pkt  pkt,
        input TxHdr_t               txhdr,
        input [CCIP_DATA_WIDTH-1:0] txdata);
        t_ccip_c1_ReqIntrHdr    ccip_intr_txhdr;
    begin
        case (txhdr.reqtype)
        ASE_RDLINE_S:
          begin
            pkt.mode = CCIPKT_READ_MODE;
            pkt.resp_channel = 0;
          end
        ASE_RDLINE_I:
          begin
            pkt.mode = CCIPKT_READ_MODE;
            pkt.resp_channel = 0;
          end
        ASE_WRLINE_M:
          begin
            pkt.mode = CCIPKT_WRITE_MODE;
            pkt.resp_channel = 1;
          end
        ASE_WRLINE_I:
          begin
            pkt.mode = CCIPKT_WRITE_MODE;
            pkt.resp_channel = 1;
          end
        ASE_WRPUSH:
          begin
            pkt.mode = CCIPKT_WRITE_MODE;
            pkt.resp_channel = 1;
          end
        ASE_WRFENCE:
          begin
            pkt.mode = CCIPKT_WRFENCE_MODE;
            pkt.resp_channel = 1;
          end
`ifdef ASE_ENABLE_INTR_FEATURE
        ASE_INTR_REQ:
          begin
            pkt.mode = CCIPKT_INTR_MODE;
            pkt.resp_channel = 1;
          end
`endif
        // ASE_ATOMIC_REQ:
        //   begin
        //     pkt.mode = CCIPKT_ATOMIC_MODE;
        //     pkt.resp_channel = 0;
        //   end
        endcase

        // Metadata
        pkt.mdata    = int'(txhdr.mdata);
        // cache line address
        pkt.cl_addr  = longint'(txhdr.addr);
        // Qword assignment
        pkt.qword[0] =  txdata[  63:00 ];
        pkt.qword[1] =  txdata[ 127:64  ];
        pkt.qword[2] =  txdata[ 191:128 ];
        pkt.qword[3] =  txdata[ 255:192 ];
        pkt.qword[4] =  txdata[ 319:256 ];
        pkt.qword[5] =  txdata[ 383:320 ];
        pkt.qword[6] =  txdata[ 447:384 ];
        pkt.qword[7] =  txdata[ 511:448 ];

        // Interrupt ID set
        ccip_intr_txhdr = t_ccip_c1_ReqIntrHdr'(txhdr);
        pkt.intr_id  = ccip_intr_txhdr.id;
        // Response enable
    end
    endfunction


    /*
     * CAFU->ASE CH0 (TX0)
     * Formed as {TxHdr_t}
     * Latency scoreboard (for latency modeling and shuffling)
     */
   `FORWARDING_CHANNEL
     #(
       .DEBUG_LOGNAME       ("latbuf_ch0.log"),
       .NUM_WAIT_STATIONS   (LATBUF_NUM_TRANSACTIONS),
       .NUM_STATIONS_FULL_THRESH (LATBUF_FULL_THRESHOLD),
       .COUNT_WIDTH         (LATBUF_COUNT_WIDTH),
       .VISIBLE_DEPTH_BASE2 (8),
       .VISIBLE_FULL_THRESH (32),
       .LATBUF_MAX_TXN      (1),
       .WRITE_CHANNEL       (0)
       )
    cf2as_latbuf_ch0
     (
      .clk    ( clk ),
      .rst    ( ase_reset ),
      .finish_trigger   ( finish_trigger ),
      .hdr_in    ( C0TxHdr ),
      .data_in    ( {CCIP_DATA_WIDTH{1'b0}} ),
      .write_en    ( C0TxValid ),
      .txhdr_out    ( cf2as_latbuf_tx0hdr ),
      .rxhdr_out        ( cf2as_latbuf_rx0hdr ),
      .data_out    (  ),
      .valid_out    ( cf2as_latbuf_ch0_valid ),
      .read_en    ( cf2as_latbuf_ch0_read ),
      .empty    ( cf2as_latbuf_ch0_empty ),
      .almfull          ( C0TxAlmFull ),
      .full             ( cf2as_ch0_realfull ),
      .overflow_error   ( ),
      .hazpkt_in        ( haz_if.read_in ),
      .hazpkt_out       ( haz_if.read_out )
      );

    // Read TX0
    always @(posedge clk) begin
        if (ase_reset) begin
            cf2as_latbuf_ch0_read <= 0;
        end
        else if (~cf2as_latbuf_ch0_empty && ~rdrsp_full) begin
            cf2as_latbuf_ch0_read <= 1;
        end
        else begin
            cf2as_latbuf_ch0_read <= 0;
        end
    end

    // TASK: cf2as_ch0_rdreq -- send read request to the application (host memory)
    task cf2as_ch0_rdreq();
        cci_pkt Tx0_pkt;
    begin
        // Cast ccipkt from txhdr
        cast_txhdr_to_ccipkt(Tx0_pkt, cf2as_latbuf_tx0hdr, {CCIP_DATA_WIDTH{1'b0}});
        // $display(" ** DEBUG **: %d => cf2as_latbuf_rx0hdr.mdata = %x", $time, cf2as_latbuf_rx0hdr);

        // Read line request
        rd_memline_req_dex(Tx0_pkt);

        // Preserve intermediate request packet.  It will be completed by
        // the response in cf2as_ch0_rdrsp_to_rdrsp_fifo.
        Rx0_pkt <= Tx0_pkt;
        Rx0_hdr <= cf2as_latbuf_rx0hdr;
    end
    endtask

    // TASK: cf2as_ch0_rdrsp_to_rdrsp_fifo -- receive read response from application
    task cf2as_ch0_rdrsp_to_rdrsp_fifo();
    begin
        // Read line fulfillment
        rd_memline_rsp_dex(Rx0_pkt);

        // Write to rdrsp_fifo
        rdrsp_data_in <= unpack_ccipkt_to_vector(Rx0_pkt);
        rdrsp_hdr_in <= Rx0_hdr;
    end
    endtask

    // Read request glue process
    always @(posedge clk) begin
        if (ase_reset) begin
            rd_memline_active <= 0;
        end
        else if (cf2as_latbuf_ch0_valid) begin
            cf2as_ch0_rdreq();
            rd_memline_active <= 1;
        end
        else begin
            rd_memline_active <= 0;
        end
    end

    // Read response glue process
    always @(posedge clk) begin
        if (ase_reset) begin
            rdrsp_write <= 0;
        end
        else if (rd_memline_active) begin
            cf2as_ch0_rdrsp_to_rdrsp_fifo();
            rdrsp_write <= 1;
        end
        else begin
            rdrsp_write <= 0;
        end
    end


    /*
     * CAFU->ASE CH1 (TX1)
     * Formed as {TxHdr_t, <data_512>}
     * Latency scoreboard (latency modeling and shuffling)
     */
   `FORWARDING_CHANNEL
     #(
       .DEBUG_LOGNAME       ("latbuf_ch1.log"),
       .NUM_WAIT_STATIONS   (LATBUF_NUM_TRANSACTIONS),
       .NUM_STATIONS_FULL_THRESH (LATBUF_FULL_THRESHOLD),
       .COUNT_WIDTH         (LATBUF_COUNT_WIDTH),
       .VISIBLE_DEPTH_BASE2 (8),
       .VISIBLE_FULL_THRESH (32),
       .LATBUF_MAX_TXN      (4),
       .WRITE_CHANNEL       (1)
       )
    cf2as_latbuf_ch1
     (
      .clk    ( clk ),
      .rst    ( ase_reset ),
      .finish_trigger   ( finish_trigger ),
      .hdr_in    ( C1TxHdr ),
      .data_in    ( C1TxData ),
      .write_en    ( C1TxValid ),
      .txhdr_out    ( cf2as_latbuf_tx1hdr ),
      .rxhdr_out        ( cf2as_latbuf_rx1hdr ),
      .data_out    ( cf2as_latbuf_tx1data ),
      .valid_out    ( cf2as_latbuf_ch1_valid),
      .read_en    ( cf2as_latbuf_ch1_read  ),
      .empty    ( cf2as_latbuf_ch1_empty ),
      .almfull          ( C1TxAlmFull ),
      .full             ( cf2as_ch1_realfull ),
      .overflow_error   ( ),
      .hazpkt_in        ( haz_if.write_in ),
      .hazpkt_out       ( haz_if.write_out )
      );


    // Read TX1
    always @(posedge clk) begin
        if (ase_reset) begin
            cf2as_latbuf_ch1_read <= 0;
        end
        else if (~cf2as_latbuf_ch1_empty && ~wrrsp_full) begin
            cf2as_latbuf_ch1_read <= 1;
        end
        else begin
            cf2as_latbuf_ch1_read <= 0;
        end
    end

    // TASK: cf2as_latbuf_to_wrrsp_fifo
    task cf2as_latbuf_to_wrrsp_fifo();
        cci_pkt Tx1_pkt;
    begin
        // Cast ccipkt from txhdr
        cast_txhdr_to_ccipkt(Tx1_pkt, cf2as_latbuf_tx1hdr, cf2as_latbuf_tx1data);
        // Write memory
        wr_memline_req_dex(Tx1_pkt);
        // Write to wrrsp_fifo
        Rx1_pkt <= Tx1_pkt;
        pp_wrrsp_hdr           = cf2as_latbuf_rx1hdr;
        // $display(" ** DEBUG **: %d => cf2as_latbuf_rx1hdr.mdata = %x", $time, cf2as_latbuf_rx1hdr);
    end
    endtask

    // TASK: wr_memline_rsp_chk
    task wr_memline_rsp_chk();
    begin
        // Memory write response confirms that the address is valid and raises an
        // error in the C code if it is not.
        wr_memline_rsp_dex(Rx1_pkt);
    end
    endtask

    /*
     * WrResp Coalescer
     * --------------------------------------------
     * - If cf2as_latbuf_ch1_valid is HIGH
     *   - If cf2as_latbuf_rx1hdr.fmt is HIGH
     *   - If cf2as_latbuf_rx1hdr.fmt is LOW
     *     - Fullfill request and passthru
     */

    // Packing states
    typedef enum {
        PassThru_Pack1CL,
        Pack2CL,
        Pack3CL,
        Pack4CL,
        PackError
    } c1rx_pack_state;

    c1rx_pack_state pack_state;

    RxHdr_t      pack_hdr;
    logic     pack_hdr_valid;

    // Packing input register
`ifdef VCS
    always @(posedge clk) begin
        pack_hdr       <= pp_wrrsp_hdr;
        pack_hdr_valid <= pp_wrrsp_write;
    end
`else
 `ifdef QUESTA
    assign pack_hdr       = pp_wrrsp_hdr;
    assign pack_hdr_valid = pp_wrrsp_write;
 `else
    // Compile time error goes here ??
    unsupported rtl compiler found
 `endif
`endif

    // Packing state machine
    always @(posedge clk) begin
        if (ase_reset) begin
            pack_state  <= PassThru_Pack1CL;
            wrrsp_write <= 0;
            wrrsp_hdr_in <= RxHdr_t'(0);
        end
        else begin
            case (pack_state)
            // ---------------------------------------- //
            // Pack 1CL if format enable, else passthru
            // ---------------------------------------- //

            PassThru_Pack1CL:
              begin
                if (pack_hdr_valid) begin
                    if (pack_hdr.format) begin
                        if (pack_hdr.clnum == ASE_1CL) begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 1;
                            pack_state   <= PassThru_Pack1CL;
                        end
                        else begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 0;
                            pack_state <= Pack2CL;
                        end
                    end
                    else begin
                        wrrsp_hdr_in <= pack_hdr;
                        wrrsp_write  <= 1;
                        pack_state   <= PassThru_Pack1CL;
                    end
                end
                else begin
                    wrrsp_hdr_in <= pack_hdr;
                    wrrsp_write  <= 0;
                    pack_state   <= PassThru_Pack1CL;
                end
              end

            // ---------------------------------------- //
            // Pack 2CL/4CL if format enable, else ERROR
            // ---------------------------------------- //
            Pack2CL:
              begin
                if (pack_hdr_valid) begin
                    if (pack_hdr.format) begin
                        if (pack_hdr.clnum == ASE_2CL) begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 1;
                            pack_state   <= PassThru_Pack1CL;
                        end
                        else if (pack_hdr.clnum == ASE_4CL) begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 0;
                            pack_state   <= Pack3CL;
                        end
                        else begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 0;
                            pack_state   <= PackError;
                        end
                    end
                    else begin
                        wrrsp_hdr_in <= pack_hdr;
                        wrrsp_write  <= 0;
                        pack_state   <= PackError;
                    end
                end
                else begin
                    wrrsp_hdr_in <= pack_hdr;
                    wrrsp_write  <= 0;
                    pack_state   <= Pack2CL;
                end
              end

            // ---------------------------------------- //
            // Pack 3CL if format enable, else ERROR
            // ---------------------------------------- //
            Pack3CL:
              begin
                if (pack_hdr_valid) begin
                    if (pack_hdr.format) begin
                        if (pack_hdr.clnum == ASE_4CL) begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 0;
                            pack_state   <= Pack4CL;
                        end
                        else begin
                            wrrsp_hdr_in <= pack_hdr;
                            wrrsp_write  <= 0;
                            pack_state   <= PackError;
                        end
                    end
                    else begin
                        wrrsp_hdr_in <= pack_hdr;
                        wrrsp_write  <= 0;
                        pack_state   <= PackError;
                    end
                end
                else begin
                    wrrsp_hdr_in <= pack_hdr;
                    wrrsp_write  <= 0;
                    pack_state   <= Pack3CL;
                end
              end

            // ---------------------------------------- //
            // Pack 4CL if format enable, else ERROR
            // ---------------------------------------- //
            Pack4CL:
              begin
                if (pack_hdr_valid) begin
                    if (pack_hdr.format && pack_hdr.clnum == ASE_4CL) begin
                        wrrsp_hdr_in <= pack_hdr;
                        wrrsp_write  <= 1;
                        pack_state   <= PassThru_Pack1CL;
                    end
                    else begin
                        wrrsp_hdr_in <= pack_hdr;
                        wrrsp_write  <= 0;
                        pack_state   <= PackError;
                    end
                end
                else begin
                    wrrsp_hdr_in <= pack_hdr;
                    wrrsp_write  <= 0;
                    pack_state   <= Pack4CL;
                end
              end

            // --------------------------------------------------- //
            // Packing ERROR, bail out | ASE should not reach here
            // --------------------------------------------------- //
            PackError:
              begin
           `BEGIN_RED_FONTCOLOR;
                $display("** ERROR ** : %d => Unexpected formatting order found --- packing cannot proceed, EXITING", $time);
           `END_RED_FONTCOLOR;
                start_simkill_countdown();
              end

            // --------------------------------------------------- //
            // Default
            // --------------------------------------------------- //
            default:
              begin
                wrrsp_hdr_in <= pack_hdr;
                wrrsp_write  <= 0;
                pack_state   <= PassThru_Pack1CL;
              end
            endcase
        end
    end


    // latbuf_ch1 -> pack logic
    always @(posedge clk) begin
        if (ase_reset) begin
            pp_wrrsp_write <= 0;
        end
        else if (cf2as_latbuf_ch1_valid) begin
            cf2as_latbuf_to_wrrsp_fifo();
            pp_wrrsp_write <= 1;
        end
        else begin
            pp_wrrsp_write <= 0;
        end
    end

    // Consume write line response
    always @(posedge clk) begin
        if (! ase_reset && pp_wrrsp_write) begin
            wr_memline_rsp_chk();
        end
    end


    /* *******************************************************************
     * RESPONSE PATHS
     * -------------------------------------------------------------------
     * as2cf_rdresp_fifo    | Read Response staging
     * as2cf_wrresp_fifo    | Write Response staging
     * as2cf_umsg_fifo      | Unordered message staging
     *
     * *******************************************************************/
    /*
     * RX0 Read Response staging
     */
    ase_svfifo
      #(
       .DATA_WIDTH     ( CCIP_RX_HDR_WIDTH + CCIP_DATA_WIDTH ),
       .DEPTH_BASE2    ( ASE_RSPFIFO_COUNT_WIDTH ),
       .ALMFULL_THRESH ( ASE_RSPFIFO_ALMFULL_THRESH )
       )
    rdrsp_fifo
     (
      .clk             ( clk ),
      .rst             ( ase_reset ),
      .wr_en           ( rdrsp_write ),
      .data_in         ( { logic_cast_RxHdr_t'(rdrsp_hdr_in), rdrsp_data_in } ),
      .rd_en           ( ~rdrsp_empty && rdrsp_read ),
      .data_out        ( { rdrsp_hdr_out_vec, rdrsp_data_out } ),
      .data_out_v      ( rdrsp_valid ),
      .alm_full        ( rdrsp_full ),
      .full            (),
      .empty           ( rdrsp_empty ),
      .count           ( rdrsp_fifo_cnt ),
      .overflow        (),
      .underflow       ()
      );

    assign rdrsp_hdr_out = RxHdr_t'(rdrsp_hdr_out_vec);


    /*
     * RX1 Write Response staging
     */
    ase_svfifo
     #(
       .DATA_WIDTH     ( CCIP_RX_HDR_WIDTH ),
       .DEPTH_BASE2    ( ASE_RSPFIFO_COUNT_WIDTH ),
       .ALMFULL_THRESH ( ASE_RSPFIFO_ALMFULL_THRESH )
       )
    wrrsp_fifo
     (
      .clk             ( clk ),
      .rst             ( ase_reset ),
      .wr_en           ( wrrsp_write ),
      .data_in         ( logic_cast_RxHdr_t'(wrrsp_hdr_in) ),
      .rd_en           ( ~wrrsp_empty && wrrsp_read ),
      .data_out        ( wrrsp_hdr_out_vec ),
      .data_out_v      ( wrrsp_valid ),
      .alm_full        ( wrrsp_full ),
      .full            (),
      .empty           ( wrrsp_empty ),
      .count           ( wrrsp_fifo_cnt ),
      .overflow        (),
      .underflow       ()
      );

    assign wrrsp_hdr_out = RxHdr_t'(wrrsp_hdr_out_vec);


    /* *******************************************************************
     * RX0 Channel management
     * -------------------------------------------------------------------
     * - MMIO Request management
     *   When request is seen in mmioreq_fifo, it is forwarded to
     *   CCIP-RX0
     * - Read Response
     *   When response is seen in as2cf_rdresp_fifo, it is forwarded to
     *   CCIP-RX0
     * - Write response
     *   When response is seen in as2cf_wrresp_fifo & tx2rx_chsel == 0, it
     *   is forwarded to CCIP-RX0
     *
     * *******************************************************************/
    // Read from staging FIFOs
    always @(posedge clk) begin
        if (ase_reset) begin
            mmioreq_read  <= 0;
            rdrsp_read    <= 0;
            umsgfifo_read <= 0;
        end
        else if (~mmioreq_empty) begin
            mmioreq_read  <= 1;
            rdrsp_read    <= 0;
            umsgfifo_read <= 0;
        end
        else if (~umsgfifo_empty) begin
            mmioreq_read  <= 0;
            rdrsp_read    <= 0;
            umsgfifo_read <= 1;
        end
        else if (~rdrsp_empty) begin
            mmioreq_read  <= 0;
            rdrsp_read    <= 1;
            umsgfifo_read <= 0;
        end
        else begin
            mmioreq_read  <= 0;
            rdrsp_read    <= 0;
            umsgfifo_read <= 0;
        end
    end

    // Output channel
    always @(posedge clk) begin
        if (SoftReset) begin
            C0RxMmioWrValid <= 0;
            C0RxMmioRdValid <= 0;
            C0RxRdValid     <= 0;
            C0RxUMsgValid   <= 0;
            C0RxHdr         <= RxHdr_t'({CCIP_RX_HDR_WIDTH{1'b0}});
            C0RxData        <= {CCIP_DATA_WIDTH{1'b0}};
        end
        else if (mmioreq_valid) begin
            C0RxMmioWrValid <= mmio_wrvalid;
            C0RxMmioRdValid <= mmio_rdvalid;
            C0RxRdValid     <= 0;
            C0RxUMsgValid   <= 0;
            C0RxHdr         <= RxHdr_t'(mmio_hdrvec);
            C0RxData        <= mmio_data512;
        end
`ifdef ASE_ENABLE_UMSG_FEATURE
        else if (umsgfifo_valid) begin
            C0RxMmioWrValid <= 0;
            C0RxMmioRdValid <= 0;
            C0RxRdValid     <= 0;
            C0RxUMsgValid   <= umsgfifo_valid;
            C0RxHdr         <= RxHdr_t'(umsgfifo_hdrvec_out);
            C0RxData        <= umsgfifo_data_out;
        end
`endif
        else if (rdrsp_valid) begin
            C0RxMmioWrValid <= 0;
            C0RxMmioRdValid <= 0;
            C0RxRdValid     <= rdrsp_valid;
            C0RxUMsgValid   <= 0;
            C0RxHdr         <= rdrsp_hdr_out;
            C0RxData        <= rdrsp_data_out;
        end
        else begin
            C0RxMmioWrValid <= 0;
            C0RxMmioRdValid <= 0;
            C0RxRdValid     <= 0;
            C0RxUMsgValid   <= 0;
            C0RxHdr         <= RxHdr_t'({CCIP_RX_HDR_WIDTH{1'b0}});
            C0RxData        <= {CCIP_DATA_WIDTH{1'b0}};
        end
    end

    // C0Rx Valid aggregate
    assign C0RxRspValid = C0RxRdValid | C0RxUMsgValid;


    /* *******************************************************************
     * RX1 Channel management
     * --------------------------------------------------------------
     * - Write response
     *   When response is seen in as2cf_wrresp_fifo & tx2rx_chsel == 1, it
     *   is forwarded to CCIP-RX1
     *
     * *******************************************************************/
    // Read from staging FIFOs
    always @(posedge clk) begin
        if (ase_reset) begin
            wrrsp_read  <= 0 ;
        end
        else if (~wrrsp_empty) begin
            wrrsp_read  <= 1 ;
        end
        else begin
            wrrsp_read  <= 0 ;
        end
    end

    // Output register
    always @(posedge clk) begin
        if (SoftReset) begin
            C1RxHdr <= {CCIP_RX_HDR_WIDTH{1'b0}};
            C1RxWrValid <= 0;
            C1RxIntrValid <= 0;
        end
        else if (wrrsp_valid) begin
            C1RxHdr       <= RxHdr_t'(wrrsp_hdr_out);
            C1RxWrValid   <= wrrsp_valid;
            C1RxIntrValid <= 0;
        end
        else begin
            C1RxHdr       <= {CCIP_RX_HDR_WIDTH{1'b0}};
            C1RxWrValid   <= 0;
            C1RxIntrValid <= 0;
        end
    end

    // Rx1 aggregate valid
    assign C1RxRspValid = C1RxWrValid | C1RxIntrValid;


    /* *******************************************************************
     * Inactivity management block
     *
     * DESCRIPTION: Running ASE simulations for too long can cause
     *              large dump-files to be formed. To prevent this, the
     *              inactivity counter will close down the simulation
     *              when CCI transactions are not seen for a long
     *              duration of time.
     *
     * This feature can be disabled, if desired.
     *
     * *******************************************************************/
    logic         first_transaction_seen;
    int           inactivity_counter;
    logic         any_valid;
    logic         inactivity_found;


    // Inactivity management - Sense first transaction
    always @(posedge clk) begin : any_valid_proc
        if (ase_reset) begin
            any_valid <= 0;
        end
        else begin
        any_valid <= pck_cp2af_sRx.c0.rspValid    |
            pck_cp2af_sRx.c0.mmioRdValid |
            pck_cp2af_sRx.c0.mmioWrValid |
            pck_cp2af_sRx.c1.rspValid    |
            pck_af2cp_sTx.c0.valid       |
            pck_af2cp_sTx.c1.valid       |
            pck_af2cp_sTx.c2.mmioRdValid;
        end
    end

    // First transaction seen
    always @(posedge clk) begin : first_txn_watcher
        if (ase_reset) begin
            first_transaction_seen <= 0;
        end
        else if ( ~first_transaction_seen && any_valid ) begin
            first_transaction_seen <= 1;
        end
    end

    // Inactivity watchdog counter
    always @(posedge clk) begin : inact_ctr
        if (cfg.ase_mode != ASE_MODE_TIMEOUT_SIMKILL) begin
            inactivity_counter <= 0;
        end
        else begin
        // Watchdog countdown
            if (first_transaction_seen && any_valid) begin
                inactivity_counter <= 0;
            end
            else if (first_transaction_seen && ~any_valid) begin
                inactivity_counter <= inactivity_counter + 1;
            end
        end
    end

    // Inactivity management - killswitch
    always @(posedge clk) begin : call_simkill_countdown
        if ( (inactivity_counter > cfg.ase_timeout) && (cfg.ase_mode == ASE_MODE_TIMEOUT_SIMKILL) ) begin
            $display("  [SIM]  Inactivity timeout reached !!\n");
            start_simkill_countdown();
        end
    end


    /*
     * Initialization procedure
     *
     * DESCRIPTION: This procedural block is called when ./simv is
     *              kicked off, helps put the simulation in a known
     *              state.
     *
     * STEPS:
     * - Print startup info
     * - Send initial system reset, cleaning up state machines
     * - Initialize ASE (ase_init executes in SW)
     *   - Set up message queues for IPC (done in SW)
     *   - Set up memory management structure (called in SW)
     * - If ENABLED, start the CA-private memory region (emulated with
     *   software
     * - Then set up the QLP InitDone signal to go indicate readiness
     * - SIMULATION is ready to begin
     *
     */
    initial begin : ase_entry_point
        init_reset <= 1;
        $display("  [SIM]  Simulator started...");

        // Check if simulator is already running in this directory:
        // If YES, kill simulator, post message
        // If NO, continue
        ase_ready_pid = ase_instance_running();
        if (ase_ready_pid != 0) begin
           `BEGIN_RED_FONTCOLOR;
           $display("  [SIM]  An ASE instance is probably still running in current directory !");
           $display("  [SIM]  Check for PID %d", ase_ready_pid);
           $display("  [SIM]  Simulation will exit... you may use a SIGKILL to kill the simulation process.");
           $display("  [SIM]  Also check if '.ase_ready.pid' file is removed before proceeding.");
           `END_RED_FONTCOLOR;
           $finish;
        end

        // AFU reset
        init_reset <= 0;
        afu_softreset_trig(1, 0 );

        // Initialize mmio_dispatch function (both integrated & discrete)
        mmio_dispatch (1, '{0, 0, 0, 0, '{0,0,0,0,0,0,0,0}, 0});

        // Initialize umsg_dispatch function (integrated only)
`ifdef ASE_ENABLE_UMSG_FEATURE
        umsg_dispatch (1, '{0, 0, '{0,0,0,0,0,0,0,0}});
`endif

        // Globally write CONFIG, SCRIPT paths
        if (config_filepath.len() != 0) begin
            sv2c_config_dex(config_filepath);
        end
        if (script_filepath.len() != 0) begin
            sv2c_script_dex(script_filepath);
        end

        // Initialize SW side of ASE
        ase_init();

        // Read seed and print
        $display("  [SIM]  ASE running with seed => %d", cfg.ase_seed);
        // $srandom(cfg.ase_seed);
        // $urandom(cfg.ase_seed);

        // Initial signal values
        $display("  [SIM]  Sending initial reset...");
        ase_reset_trig();

        sw_reset_trig <= 0;
        run_clocks(20);

        // Indicate to APP that ASE is ready
        ase_ready();
    end


    /*
     * ASE Flow control error monitoring
     */
    // Flow simkill
    task flowerror_simkill(int sim_time, int channel) ;
    begin
       `BEGIN_RED_FONTCOLOR;
        $display("  [SIM]  ASE has detected a possible OVERFLOW or UNDERFLOW error.");
        $display("  [SIM]  Check simulation around time, t = %d in Channel %d", sim_time, channel);
        $display("  [SIM]  Simulation will end now");
       `END_RED_FONTCOLOR;
        start_simkill_countdown();
    end
    endtask


    /*
     * CCI-P Checker
     * Aggregate point for all ASE checkers
     * - XZ checker
     * - Data hazard warning
     */
`ifndef ASE_DISABLE_CHECKER

    assign ase_checker_disable = 0;

    // ccip_checker instance
    ccip_checker ccip_checker
      (
        // ----------------------------------------- //
        // Logger control
        .finish_logger      (finish_trigger     ),
        .init_sniffer       (ase_reset          ),
        .ase_reset          (ase_reset          ),
        // ----------------------------------------- //
        // CCIP ports
        .clk                ( clk                ),
        .SoftReset          ( SoftReset          ),
        .ccip_rx            ( pck_cp2af_sRx      ),
        .ccip_tx            ( pck_af2cp_sTx      ),
        // ----------------------------------------- //
        // Hazard checker interface
        .haz_if             ( haz_if ),
        .error_code         ( ),
        // ----------------------------------------- //
        // Overflow check signals
        .cf2as_ch0_realfull ( cf2as_ch0_realfull ),
        .cf2as_ch1_realfull ( cf2as_ch1_realfull )
      );
`else
    assign ase_checker_disable = 1;
`endif


    /*
     * CCI Logger module
     */
`ifndef ASE_DISABLE_LOGGER

    assign ase_logger_disable = 0;

    // ccip_logger instance
    ccip_logger
     #(
        .LOGNAME         ("ccip_transactions.tsv")
       )
    ccip_logger
        (
        // Logger control
        .finish_logger    ( finish_trigger       ),
        .stdout_en        ( cfg.enable_cl_view[0]),
        // Buffer message injection
        .log_string_en    ( buffer_msg_en        ),
        .log_timestamp_en ( buffer_msg_tstamp_en ),
        .log_string       ( buffer_msg           ),
        // CCIP ports
        .clk              ( clk                  ),
        .SoftReset        ( SoftReset            ),
        .ccip_rx          ( pck_cp2af_sRx        ),
        .ccip_tx          ( pck_af2cp_sTx        )
        );
`else
    assign ase_logger_disable = 1;
`endif //  `ifndef ASE_DISABLE_LOGGER

    /*
     * Transaction drop checker
     */
`ifdef ASE_DEBUG
    longint rdtxn_array[*];
    longint wrtxn_array[*];
    longint wrf_array[*];

    logic [1:0] c1tx_mcl;

    // Check and delete by key
    function automatic void ccip_txn_check_delete(longint key, ref longint assoc_array [*] );
    begin
        if (assoc_array.exists( key )) begin
            assoc_array.delete( key);
        end
        else begin
            `BEGIN_RED_FONTCOLOR;
            $display(" ** ERROR ** ccip_emulator checker couldnt find key=%x", key);
            `END_RED_FONTCOLOR;
        end
    end
    endfunction

    // Iterate-print
    // function automatic void print_assoc_array(ref longint assoc_array[*]);
    //    longint temp;
    //    begin
    //      if (assoc_array.first(temp))
    //        do
    //          $display("( %05x : %12x) ", temp, assoc_array[temp] );
    //      while (assoc_array.next(temp));
    //    end
    // endfunction

    // c1tx_mcl
    always @(*) begin
        if (C1TxHdr.sop) begin
            c1tx_mcl = C1TxHdr.len;
        end
    end

`endif


    /* ******************************************************************
     *
     * This call is made on ERRORs requiring a shutdown
     * simkill is called from software, and is the final step before
     * graceful closedown
     *
     * *****************************************************************/
    // Flag
    logic       simkill_started = 0;

`ifdef ASE_PROFILE
    int            hist_ch0_fd, hist_ch1_fd;
`endif

    // Simkill progress
    task simkill();
        string print_str;
    begin
        simkill_started = 1;
        $display("  [SIM]  Simulation kill command received...");
        // Print transactions
        `BEGIN_YELLOW_FONTCOLOR;
        $display("  Transaction count \t| %8s %8s %8s %8s | %8s %8s %8s", "VA", "VL0", "VH0", "VH1", "MCL-1", "MCL-2", "MCL-4");
        $display("  ========================================================================================");
        $display("  MMIOWrReq %d | ", ase_rx0_mmiowrreq_cnt );
        $display("  MMIORdReq %d | ", ase_rx0_mmiordreq_cnt );
        $display("  MMIORdRsp %d | ", ase_tx2_mmiordrsp_cnt );
`ifdef ASE_ENABLE_UMSG_FEATURE
        $display("  UMsgHint  %d | ", ase_rx0_umsghint_cnt  );
        $display("  UMsgData  %d | ", ase_rx0_umsgdata_cnt  );
`endif
`ifdef ASE_ENABLE_INTR_FEATURE
        $display("  IntrReq   %d | ", ase_tx1_intrreq_cnt  );
        $display("  IntrResp  %d | ", ase_rx1_intrrsp_cnt  );
`endif
        $display("  RdReq     %d | %8d %8d %8d %8d | %8d %8d %8d",
            ase_tx0_rdvalid_cnt, rdreq_vc_cnt.va, rdreq_vc_cnt.vl0, rdreq_vc_cnt.vh0, rdreq_vc_cnt.vh1, 
            rdreq_mcl_cnt.mcl0, rdreq_mcl_cnt.mcl1, rdreq_mcl_cnt.mcl3);
        $display("  RdResp    %d | %8d %8d %8d %8d | ",
            ase_rx0_rdvalid_cnt, rdrsp_vc_cnt.va, rdrsp_vc_cnt.vl0, rdrsp_vc_cnt.vh0, rdrsp_vc_cnt.vh1);
        $display("  WrReq     %d | %8d %8d %8d %8d | %8d %8d %8d",
            ase_tx1_wrvalid_cnt, wrreq_vc_cnt.va, wrreq_vc_cnt.vl0, wrreq_vc_cnt.vh0, wrreq_vc_cnt.vh1,
            wrreq_mcl_cnt.mcl0, wrreq_mcl_cnt.mcl1, wrreq_mcl_cnt.mcl3);
        $display("  WrResp    %d | %8d %8d %8d %8d | %8d %8d %8d",
            ase_rx1_wrvalid_cnt, wrrsp_vc_cnt.va, wrrsp_vc_cnt.vl0, wrrsp_vc_cnt.vh0, wrrsp_vc_cnt.vh1,
            wrrsp_mcl_cnt.mcl0, wrrsp_mcl_cnt.mcl1, wrrsp_mcl_cnt.mcl3);
        $display("  WrFence   %d | %8d %8d %8d %8d | ",
            ase_tx1_wrfence_cnt, wrfreq_vc_cnt.va, wrfreq_vc_cnt.vl0, wrfreq_vc_cnt.vh0, wrfreq_vc_cnt.vh1);
        $display("  WrFenRsp  %d | %8d %8d %8d %8d | ",
            ase_rx1_wrfence_cnt, wrfrsp_vc_cnt.va, wrfrsp_vc_cnt.vl0, wrfrsp_vc_cnt.vh0, wrfrsp_vc_cnt.vh1);
        `END_YELLOW_FONTCOLOR;

        // Valid Count
`ifdef ASE_DEBUG
        // Print errors
        `BEGIN_RED_FONTCOLOR;
        if (ase_tx0_rdvalid_cnt != ase_rx0_rdvalid_cnt)
            $display("\tREADs   : Response counts dont match request count !!");
        if (ase_tx1_wrvalid_cnt != ase_rx1_wrvalid_cnt)
            $display("\tWRITEs  : Response counts dont match request count !!");
        if (ase_tx2_mmiordrsp_cnt != ase_rx0_mmiordreq_cnt)
            $display("\tMMIORd  : Response counts dont match request count !!");
        if (ase_tx1_wrfence_cnt != ase_rx1_wrfence_cnt)
            $display("\tWrFence : Response counts dont match request count !!");
        `END_RED_FONTCOLOR;
        // Dropped transactions
       `BEGIN_YELLOW_FONTCOLOR;
        $display("-----------------------------------------------------------------");
        $display("cf2as_latbuf_ch0 contents =>");
        $display(ase_top.ccip_emulator.cf2as_latbuf_ch0.check_hdr_array);
        $display("cf2as_latbuf_ch1 contents =>");
        $display(ase_top.ccip_emulator.cf2as_latbuf_ch1.check_hdr_array);
        $display("-----------------------------------------------------------------");
       `END_YELLOW_FONTCOLOR;
`endif

        // Histogram dump generator
`ifdef ASE_PROFILE
        `BEGIN_YELLOW_FONTCOLOR;
        $display("Generating Latency distribution histograms... cf2as_latbuf_ch0");
        hist_ch0_fd = $fopen("latbuf_ch0.hist.dat", "w");
        for(int ii = 0; ii < `ASE_MAX_LATENCY; ii = ii + 1)
        begin
           $fwrite(hist_ch0_fd, "%d\t%d\n", ii, ase_top.ccip_emulator.cf2as_latbuf_ch0.histogram_stats[ii]);
        end
        $fclose(hist_ch0_fd);
        $display("Generating Latency distribution histograms... cf2as_latbuf_ch1");
        hist_ch1_fd = $fopen("latbuf_ch1.hist.dat", "w");
        for(int ii = 0; ii < `ASE_MAX_LATENCY; ii = ii + 1)
        begin
            $fwrite(hist_ch1_fd, "%d\t%d\n", ii, ase_top.ccip_emulator.cf2as_latbuf_ch1.histogram_stats[ii]);
        end
        $fclose(hist_ch0_fd);
       `END_YELLOW_FONTCOLOR;
`endif
        // Finish command issue
        // issue_finish_trig();

        // Command to close logfd
        $finish;
    end
    endtask

    /* ***************************************************************************
     * Memory deallocation lock
     * --------------------------------------------------------------------------
     * Problem: Due to reordering nature of ASE (guaranteed unordered
     * transactions, DSMs can get unordered, resulting in applications
     * deallocating memory
     *
     * Potential solution:
     * - Count credits running in ASE, { (Tx0, RX0), (Tx1, Rx1), Umsg
     * outstanding, CsrWrite, (Rx0MMIORd, C2tx) }
     * - If total credit count is non-zero, a lock variable will be set, back
     * pressuring any deallocate_buffer requests
     * - Dellocate requests will be queued but not executed
     * **************************************************************************/
    always @(posedge clk) begin
        if (ase_reset) begin
            rd_credit     <= 0;
            wr_credit     <= 0;
            mmiowr_credit <= 0;
            mmiord_credit <= 0;
            // atomic_credit <= 0;
        end
        else begin
            // --------------------------------------------- //
            // Read credit counter
            rd_credit <= ase_tx0_rdvalid_cnt - ase_rx0_rdvalid_cnt;
            // ---------------------------------------------------- //
            // Write credit counter
            wr_credit <= ase_tx1_wrvalid_cnt - ase_rx1_wrvalid_cnt;
            // ---------------------------------------------------- //
            // MMIO Writevalid counter
            case ( {cwlp_wrvalid, C0RxMmioWrValid} )
                2'b10   : mmiowr_credit <= mmiowr_credit + 1;
                2'b01   : mmiowr_credit <= mmiowr_credit - 1;
            default : mmiowr_credit <= mmiowr_credit;
            endcase // case ( {cwlp_wrvalid, C0RxMmioWrValid} )
            // ---------------------------------------------------- //
            // MMIO readvalid counter
            case ( {cwlp_rdvalid, mmioresp_valid} )
                2'b10   : mmiord_credit <= mmiord_credit + 1;
                2'b01   : mmiord_credit <= mmiord_credit - 1;
            default : mmiord_credit <= mmiord_credit;
            endcase // case ( {cwlp_rdvalid, mmioresp_valid} )
            // ---------------------------------------------------- //
            // Umsg valid counter
`ifdef ASE_ENABLE_UMSG_FEATURE
            umsg_credit <= $countones(umsg_hint_enable_array) + $countones(umsg_data_enable_array) + umsgfifo_cnt;
`endif
            // ---------------------------------------------------- //
            // Atomics CmpXchg counter
            // case ( { (C1TxValid && (C1TxHdr.reqtype==ASE_ATOMIC_REQ)), (C0RxRdValid && (C0RxHdr.resptype==ASE_ATOMIC_RSP)) } )
            //   2'b10   : atomic_credit <= atomic_credit + 1;
            //   2'b01   : atomic_credit <= atomic_credit - 1;
            //   default : atomic_credit <= atomic_credit;
            // endcase
            // ---------------------------------------------------- //
        end
    end

    // Global dealloc flag enable
    always @(posedge clk) begin
        glbl_dealloc_credit <= wr_credit + rd_credit + mmiord_credit + mmiowr_credit + umsg_credit + mmioreq_count + rdrsp_fifo_cnt + wrrsp_fifo_cnt ;
    end

    // Register for changes
    always @(posedge clk) begin
        glbl_dealloc_credit_q <= glbl_dealloc_credit;
    end

    // Update process
    always @(posedge clk) begin
        if ((glbl_dealloc_credit_q == 0) && (glbl_dealloc_credit != 0)) begin
            update_glbl_dealloc(0);
        end
        else if ((glbl_dealloc_credit_q != 0) && (glbl_dealloc_credit == 0)) begin
            update_glbl_dealloc(1);
        end
        else if (glbl_dealloc_credit == 0) begin
            update_glbl_dealloc(1);
        end
        else if ((glbl_dealloc_credit_q == 0) && (glbl_dealloc_credit == 0)) begin
            update_glbl_dealloc(0);
        end
    end

endmodule // cci_emulator
