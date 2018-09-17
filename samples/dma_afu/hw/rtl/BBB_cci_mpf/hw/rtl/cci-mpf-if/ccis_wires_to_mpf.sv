//
// Map CCI-S wires to the MPF interface.
//

//
// In addition to mapping CCI-S wires, this module adds a canonicalization layer
// that shifts all channel 0 write responses to channel 1.  This is the behavior
// of later CCI versions.
//

`include "cci_mpf_if.vh"

`ifdef MPF_HOST_IFC_CCIS
import ccis_if_pkg::*;

module ccis_wires_to_mpf
  #(
    parameter REGISTER_INPUTS = 1,
    parameter REGISTER_OUTPUTS = 0,

    // CSR read compatibility mode address is the address of a CSR write
    // that triggers a CSR read.  Translate it here.
    parameter CSR_READ_COMPAT_ADDR = -1
    )
   (
    // -------------------------------------------------------------------
    //
    //   System interface.  These signals come directly from the CCI.
    //
    // -------------------------------------------------------------------

    input logic             vl_clk_LPdomain_32ui,                // CCI Inteface Clock. 32ui link/protocol clock domain.
    input logic             ffs_vl_LP32ui_lp2sy_SoftReset_n,     // CCI-S soft reset

    input  logic            vl_clk_LPdomain_16ui,                // 2x CCI interface clock. Synchronous.16ui link/protocol clock domain.
    input  logic            ffs_vl_LP32ui_lp2sy_SystemReset_n,   // System Reset

    // Native CCI Interface (cache line interface for back end)
    /* Channel 0 can receive READ, WRITE, WRITE CSR responses.*/
    input  t_ccis_RspMemHdr ffs_vl18_LP32ui_lp2sy_C0RxHdr,       // System to LP header
    input  t_ccis_clData    ffs_vl512_LP32ui_lp2sy_C0RxData,     // System to LP data 
    input  logic            ffs_vl_LP32ui_lp2sy_C0RxWrValid,     // RxWrHdr valid signal 
    input  logic            ffs_vl_LP32ui_lp2sy_C0RxRdValid,     // RxRdHdr valid signal
    input  logic            ffs_vl_LP32ui_lp2sy_C0RxCgValid,     // RxCgHdr valid signal
    input  logic            ffs_vl_LP32ui_lp2sy_C0RxUgValid,     // Rx Umsg Valid signal
    input  logic            ffs_vl_LP32ui_lp2sy_C0RxIrValid,     // Rx Interrupt valid signal
    /* Channel 1 reserved for WRITE RESPONSE ONLY */
    input  t_ccis_RspMemHdr ffs_vl18_LP32ui_lp2sy_C1RxHdr,       // System to LP header (Channel 1)
    input  logic            ffs_vl_LP32ui_lp2sy_C1RxWrValid,     // RxData valid signal (Channel 1)
    input  logic            ffs_vl_LP32ui_lp2sy_C1RxIrValid,     // Rx Interrupt valid signal (Channel 1)

    /*Channel 0 reserved for READ REQUESTS ONLY */        
    output t_ccis_ReqMemHdr ffs_vl61_LP32ui_sy2lp_C0TxHdr,       // System to LP header 
    output logic            ffs_vl_LP32ui_sy2lp_C0TxRdValid,     // TxRdHdr valid signals 
    /*Channel 1 reserved for WRITE REQUESTS ONLY */       
    output t_ccis_ReqMemHdr ffs_vl61_LP32ui_sy2lp_C1TxHdr,       // System to LP header
    output t_ccis_clData    ffs_vl512_LP32ui_sy2lp_C1TxData,     // System to LP data 
    output logic            ffs_vl_LP32ui_sy2lp_C1TxWrValid,     // TxWrHdr valid signal
    output logic            ffs_vl_LP32ui_sy2lp_C1TxIrValid,     // Tx Interrupt valid signal
    /* Tx push flow control */
    input  logic            ffs_vl_LP32ui_lp2sy_C0TxAlmFull,     // Channel 0 almost full
    input  logic            ffs_vl_LP32ui_lp2sy_C1TxAlmFull,     // Channel 1 almost full

    input  logic            ffs_vl_LP32ui_lp2sy_InitDnForSys,    // System layer is aok to run


    // -------------------------------------------------------------------
    //
    //   MPF interface.
    //
    // -------------------------------------------------------------------

    cci_mpf_if fiu
    );

    logic  clk;
    assign clk = vl_clk_LPdomain_32ui;

    // FIU connection to the external wires
    cci_mpf_if fiu_ext(.clk);
    cci_mpf_if fiu_int(.clk);

    logic reset = 1'b1;
    assign fiu_ext.reset = reset;
    assign fiu_int.reset = fiu_ext.reset;

    always @(posedge clk)
    begin
        reset <= ! (ffs_vl_LP32ui_lp2sy_SoftReset_n &&
                    ffs_vl_LP32ui_lp2sy_InitDnForSys);
    end

    assign fiu_ext.c0Tx = fiu_int.c0Tx;
    assign fiu_int.c0TxAlmFull = fiu_ext.c0TxAlmFull;

    assign fiu_ext.c1Tx = fiu_int.c1Tx;

    assign fiu_ext.c2Tx = fiu_int.c2Tx;

    //
    // Route AFU Tx lines toward FIU, converting multi-line requests into
    // single line requests since CCI-S doesn't support multi-line.
    //
    cci_mpf_if fiu_multi(.clk);
    cci_mpf_multi_line_read_to_single
      multi_rd
       (
        .clk,
        .fiu(fiu_int),
        .afu(fiu_multi)
        );

    cci_mpf_multi_line_write_to_single
      multi_wr
       (
        .clk,
        .fiu(fiu_multi),
        .afu(fiu)
        );


    // CCI-S C0 header from MPF header
    function automatic t_ccis_ReqMemHdr cci_mpf_to_ccis_c0_ReqMemHdr(t_cci_mpf_c0_ReqMemHdr mpf_h);
        t_ccis_ReqMemHdr h;

        h = t_ccis_ReqMemHdr'(0);
        h.address  = t_ccis_clAddr'(mpf_h.base.address);
        h.mdata    = t_ccis_mdata'(mpf_h.base.mdata);

        case (mpf_h.base.req_type)
            eREQ_RDLINE_I: h.req_type = esREQ_RDLINE_I;
            eREQ_RDLINE_S: h.req_type = esREQ_RDLINE_S;
            default:       h.req_type = t_ccis_req'(0);
        endcase

        return h;
    endfunction

    // CCI-S C1 header from MPF header
    function automatic t_ccis_ReqMemHdr cci_mpf_to_ccis_c1_ReqMemHdr(t_cci_mpf_c1_ReqMemHdr mpf_h);
        t_ccis_ReqMemHdr h;

        h = t_ccis_ReqMemHdr'(0);
        h.address  = t_ccis_clAddr'(mpf_h.base.address);
        h.mdata    = t_ccis_mdata'(mpf_h.base.mdata);

        case (mpf_h.base.req_type)
            eREQ_WRLINE_I: h.req_type = esREQ_WRLINE_I;
            eREQ_WRLINE_M: h.req_type = esREQ_WRLINE_M;
            eREQ_WRFENCE:  h.req_type = esREQ_WRFENCE;
            eREQ_INTR:     h.req_type = esREQ_INTR;
            default:       h.req_type = t_ccis_req'(0);
        endcase

        return h;
    endfunction

    t_ccis_ReqMemHdr c0TxHdr;
    assign c0TxHdr = cci_mpf_to_ccis_c0_ReqMemHdr(fiu_ext.c0Tx.hdr);
    t_ccis_ReqMemHdr c1TxHdr;
    assign c1TxHdr = cci_mpf_to_ccis_c1_ReqMemHdr(fiu_ext.c1Tx.hdr);

    // Validate requests and responses
    always_ff @(posedge clk)
    begin
        if (! reset)
        begin
            if (cci_mpf_c0TxIsReadReq(fiu_ext.c0Tx))
            begin
                assert(t_cci_clAddr'(c0TxHdr.address) == fiu_ext.c0Tx.hdr.base.address) else
                    $fatal("ccis_wires_to_mpf.sv: c0TxHdr address truncated");
                assert(t_cci_mdata'(c0TxHdr.mdata) == fiu_ext.c0Tx.hdr.base.mdata) else
                    $fatal("ccis_wires_to_mpf.sv: c0TxHdr mdata truncated");
                assert(! fiu_ext.c0Tx.hdr.ext.addrIsVirtual) else
                    $fatal("ccis_wires_to_mpf.sv: c0TxHdr address is virtual");
                assert(fiu_ext.c0Tx.hdr.base.cl_len == 0) else
                    $fatal("ccis_wires_to_mpf.sv: c0TxHdr cl_num != 0");
            end

            if (cci_mpf_c1TxIsWriteReq(fiu_ext.c1Tx))
            begin
                assert(t_cci_clAddr'(c1TxHdr.address) == fiu_ext.c1Tx.hdr.base.address) else
                    $fatal("ccis_wires_to_mpf.sv: c1TxHdr address truncated");
                assert(t_cci_mdata'(c1TxHdr.mdata) == fiu_ext.c1Tx.hdr.base.mdata) else
                    $fatal("ccis_wires_to_mpf.sv: c1TxHdr mdata truncated");
                assert(! fiu_ext.c1Tx.hdr.ext.addrIsVirtual) else
                    $fatal("ccis_wires_to_mpf.sv: c1TxHdr address is virtual");
                assert(fiu_ext.c1Tx.hdr.base.cl_len == 0) else
                    $fatal("ccis_wires_to_mpf.sv: c1TxHdr cl_num != 0");
            end

            assert(! ffs_vl_LP32ui_lp2sy_C0RxIrValid) else
                $fatal("ccis_wires_to_mpf.sv: ffs_vl_LP32ui_lp2sy_C0RxIrValid not supported");
        end
    end

    generate
        if (REGISTER_OUTPUTS)
        begin : reg_out
            always_ff @(posedge clk)
            begin
                ffs_vl61_LP32ui_sy2lp_C0TxHdr <= c0TxHdr;
                ffs_vl_LP32ui_sy2lp_C0TxRdValid <= cci_mpf_c0TxIsReadReq(fiu_ext.c0Tx);
                fiu_ext.c0TxAlmFull <= ffs_vl_LP32ui_lp2sy_C0TxAlmFull;

                ffs_vl61_LP32ui_sy2lp_C1TxHdr <= c1TxHdr;
                ffs_vl512_LP32ui_sy2lp_C1TxData <= fiu_ext.c1Tx.data;
                ffs_vl_LP32ui_sy2lp_C1TxWrValid <= cci_mpf_c1TxIsWriteReq(fiu_ext.c1Tx) ||
                                                   cci_mpf_c1TxIsWriteFenceReq(fiu_ext.c1Tx);
                ffs_vl_LP32ui_sy2lp_C1TxIrValid <= 1'b0;
                fiu_ext.c1TxAlmFull <= ffs_vl_LP32ui_lp2sy_C1TxAlmFull;
            end
        end
        else
        begin : wire_out
            always_comb
            begin
                ffs_vl61_LP32ui_sy2lp_C0TxHdr = c0TxHdr;
                ffs_vl_LP32ui_sy2lp_C0TxRdValid = cci_mpf_c0TxIsReadReq(fiu_ext.c0Tx);
                fiu_ext.c0TxAlmFull = ffs_vl_LP32ui_lp2sy_C0TxAlmFull;

                ffs_vl61_LP32ui_sy2lp_C1TxHdr = c1TxHdr;
                ffs_vl512_LP32ui_sy2lp_C1TxData = fiu_ext.c1Tx.data;
                ffs_vl_LP32ui_sy2lp_C1TxWrValid = cci_mpf_c1TxIsWriteReq(fiu_ext.c1Tx) ||
                                                  cci_mpf_c1TxIsWriteFenceReq(fiu_ext.c1Tx);
                ffs_vl_LP32ui_sy2lp_C1TxIrValid = 1'b0;
                fiu_ext.c1TxAlmFull = ffs_vl_LP32ui_lp2sy_C1TxAlmFull;
            end
        end
    endgenerate


    //
    // Convert RX wires
    //

    // MPF C0 header from CCI-S
    function automatic t_cci_c0_RspMemHdr ccis_to_cci_c0_RspMemHdr(t_ccis_RspMemHdr h_s);
        t_cci_c0_RspMemHdr h;

        h = t_cci_c0_RspMemHdr'(0);
        h.vc_used = eVC_VL0;
        h.mdata = t_cci_mdata'(h_s.mdata);

        case (h_s.resp_type)
            esRSP_RDLINE: h.resp_type = eRSP_RDLINE;
            esRSP_UMSG:   h.resp_type = eRSP_UMSG;
            default:      h.resp_type = t_ccip_c0_rsp'(0);
        endcase
        return h;
    endfunction

    // MPF C1 header from CCI-S
    function automatic t_cci_c1_RspMemHdr ccis_to_cci_c1_RspMemHdr(t_ccis_RspMemHdr h_s);
        t_cci_c1_RspMemHdr h;

        h = t_cci_c1_RspMemHdr'(0);
        h.vc_used = eVC_VL0;
        h.mdata = t_cci_mdata'(h_s.mdata);

        case (h_s.resp_type)
            esRSP_WRLINE: h.resp_type = eRSP_WRLINE;
            esRSP_INTR:   h.resp_type = eRSP_INTR;
            default:      h.resp_type = t_ccip_c1_rsp'(0);
        endcase
        return h;
    endfunction

    function automatic t_cci_c0_ReqMmioHdr ccis_to_cci_ReqMmioHdr(t_ccis_RspMemHdr h_s);
        t_cci_c0_ReqMmioHdr h;

        h = t_ccip_c0_ReqMmioHdr'(0);
        h.address = t_ccip_mmioAddr'(h_s.mdata);

        // CSR read compatibility mode?  Read address is in data.
        if (h.address == CSR_READ_COMPAT_ADDR)
        begin
            h.address = t_ccip_mmioAddr'(ffs_vl512_LP32ui_lp2sy_C0RxData);
        end

        h.length = 2'b1;
        return h;
    endfunction

    t_cci_c0_RspMemHdr c0RxHdr;
    assign c0RxHdr =
        ffs_vl_LP32ui_lp2sy_C0RxCgValid ?
            t_cci_c0_RspMemHdr'(ccis_to_cci_ReqMmioHdr(ffs_vl18_LP32ui_lp2sy_C0RxHdr)) :
            ccis_to_cci_c0_RspMemHdr(ffs_vl18_LP32ui_lp2sy_C0RxHdr);
    t_cci_c1_RspMemHdr c1RxHdr;
    assign c1RxHdr = ccis_to_cci_c1_RspMemHdr(ffs_vl18_LP32ui_lp2sy_C1RxHdr);

    //
    // Only 8 byte CSR writes are supported.  CCI-S only supports native 4
    // byte CSRs.  We get around this by retaining the previous write
    // and combining it when forwarding CSR writes.  The software writes
    // to a known empty CSR address to update the high part.
    //
    logic [31:0] csr_wr_high;
    always_ff @(posedge clk)
    begin
        if (ffs_vl_LP32ui_lp2sy_C0RxCgValid)
        begin
            csr_wr_high <= 32'(ffs_vl512_LP32ui_lp2sy_C0RxData);
        end
    end

    t_cci_clData c0RxData;
    always_comb
    begin
        c0RxData = ffs_vl512_LP32ui_lp2sy_C0RxData;
        if (ffs_vl_LP32ui_lp2sy_C0RxCgValid)
        begin
            c0RxData[63:32] = csr_wr_high;
        end
    end

    logic c0RxCgValid;
    assign c0RxCgValid = ffs_vl_LP32ui_lp2sy_C0RxCgValid &&
                         (ffs_vl18_LP32ui_lp2sy_C0RxHdr.mdata != CSR_READ_COMPAT_ADDR);

    // CSR read?  Triggered using "compatibility mode" in which a CSR
    // write to a magic address triggers a read.
    logic csr_rd_en;
    assign csr_rd_en = ffs_vl_LP32ui_lp2sy_C0RxCgValid &&
                       (ffs_vl18_LP32ui_lp2sy_C0RxHdr.mdata == CSR_READ_COMPAT_ADDR);

    generate
        if (REGISTER_INPUTS)
        begin : reg_in
            always_ff @(posedge clk)
            begin
                fiu_ext.c0Rx.hdr         <= c0RxHdr;
                fiu_ext.c0Rx.data        <= c0RxData;
                fiu_ext.c0Rx.rspValid    <= ffs_vl_LP32ui_lp2sy_C0RxRdValid ||
                                            ffs_vl_LP32ui_lp2sy_C0RxUgValid;
                fiu_ext.c0Rx.mmioWrValid <= c0RxCgValid;
                fiu_ext.c0Rx.mmioRdValid <= csr_rd_en;

                fiu_ext.c1Rx.hdr         <= c1RxHdr;
                fiu_ext.c1Rx.rspValid    <= ffs_vl_LP32ui_lp2sy_C1RxWrValid;
            end
        end
        else
        begin : wire_in
            always_comb
            begin
                fiu_ext.c0Rx.hdr         = c0RxHdr;
                fiu_ext.c0Rx.data        = c0RxData;
                fiu_ext.c0Rx.rspValid    = ffs_vl_LP32ui_lp2sy_C0RxRdValid ||
                                           ffs_vl_LP32ui_lp2sy_C0RxUgValid;
                fiu_ext.c0Rx.mmioWrValid = c0RxCgValid;
                fiu_ext.c0Rx.mmioRdValid = csr_rd_en;

                fiu_ext.c1Rx.hdr         = c1RxHdr;
                fiu_ext.c1Rx.rspValid    = ffs_vl_LP32ui_lp2sy_C1RxWrValid;
            end
        end
    endgenerate


    //
    // Interfaces after CCI-S return write responses only on c1.  Move
    // all c0 write responses to c1.  Also fake a WrFence response.
    //

    // Limit write requests to the available response buffer space.
    localparam MAX_WRITE_REQS = 256;
    logic [$clog2(MAX_WRITE_REQS)-1 : 0] num_active_writes;
    logic wrfence_alm_full;

    logic wr0_valid;
    assign wr0_valid = cci_mpf_c1TxIsWriteReq(fiu_int.c1Tx);
    logic wr1_valid;
    assign wr1_valid = cci_c1Rx_isWriteRsp(fiu_int.c1Rx);

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            num_active_writes <= 0;
        end
        else
        begin
            // The active write request count changes only when there
            // is a new request without a response or vice versa.

            if (wr0_valid && ! wr1_valid)
            begin
                // New request without corresponding response
                num_active_writes <= num_active_writes + 1;
            end
            else if (! wr0_valid && wr1_valid)
            begin
                // Response without corresponding new request
                num_active_writes <= num_active_writes - 1;
            end
        end
    end
    
    // Signal full to avoid filling the write response FIFO
    assign fiu_int.c1TxAlmFull =
        fiu_ext.c1TxAlmFull ||
        (num_active_writes >= MAX_WRITE_REQS - CCI_TX_ALMOST_FULL_THRESHOLD) ||
        wrfence_alm_full;



    //
    // Send c0 write responses to c1 instead.
    //
    t_ccis_mdata wr_rsp_mdata;
    logic wr_rsp_deq_en;
    logic wr_rsp_not_empty;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(CCI_PLATFORM_MDATA_WIDTH),
        .N_ENTRIES(MAX_WRITE_REQS)
        )
      c0_wr_rsp
       (
        .clk,
        .reset,
        .enq_data(ffs_vl18_LP32ui_lp2sy_C0RxHdr.mdata),
        .enq_en(ffs_vl_LP32ui_lp2sy_C0RxWrValid),
        .first(wr_rsp_mdata),
        .deq_en(wr_rsp_deq_en),
        .notEmpty(wr_rsp_not_empty),
        .notFull(),
        .almostFull()
        );


    //
    // Record WrFence and send a response.
    //
    t_cci_mdata wr_fence_mdata;
    t_cci_vc wr_fence_vc_used;
    logic wr_fence_deq_en;
    logic wr_fence_not_empty;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS(CCI_MDATA_WIDTH + $bits(t_cci_vc)),
        .N_ENTRIES(16),
        .THRESHOLD(CCI_TX_ALMOST_FULL_THRESHOLD+2)
        )
      c1_wr_fence
       (
        .clk,
        .reset,
        .enq_data({fiu_ext.c1Tx.hdr.base.mdata, fiu_ext.c1Tx.hdr.base.vc_sel}),
        .enq_en(cci_mpf_c1TxIsWriteFenceReq(fiu_ext.c1Tx)),
        .first({wr_fence_mdata, wr_fence_vc_used}),
        .deq_en(wr_fence_deq_en),
        .notEmpty(wr_fence_not_empty),
        .notFull(),
        .almostFull(wrfence_alm_full)
        );


    always_comb
    begin
        fiu_int.c0Rx = fiu_ext.c0Rx;

        wr_rsp_deq_en = 1'b0;
        wr_fence_deq_en = 1'b0;

        if (wr_rsp_not_empty && ! cci_c1Rx_isValid(fiu_ext.c1Rx))
        begin
            fiu_int.c1Rx = t_if_cci_c1_Rx'(0);
            fiu_int.c1Rx.hdr.resp_type = eRSP_WRLINE;
            fiu_int.c1Rx.hdr.vc_used = eVC_VL0;
            fiu_int.c1Rx.hdr.mdata = t_cci_mdata'(wr_rsp_mdata);
            fiu_int.c1Rx.rspValid = 1'b1;

            wr_rsp_deq_en = 1'b1;
        end
        else if (wr_fence_not_empty && ! cci_c1Rx_isValid(fiu_ext.c1Rx))
        begin
            fiu_int.c1Rx = t_if_cci_c1_Rx'(0);
            fiu_int.c1Rx.hdr.resp_type = eRSP_WRFENCE;
            fiu_int.c1Rx.hdr.vc_used = wr_fence_vc_used;
            fiu_int.c1Rx.hdr.mdata = t_cci_mdata'(wr_fence_mdata);
            fiu_int.c1Rx.rspValid = 1'b1;

            wr_fence_deq_en = 1'b1;
        end
        else
        begin
            fiu_int.c1Rx = fiu_ext.c1Rx;
        end
    end

endmodule // ccis_wires_to_mpf


//
// Convert a multi-line read request to individual requests since
// the native interface doesn't support them.
//
module cci_mpf_multi_line_read_to_single
  #(
    parameter MAX_ACTIVE_REQS = 128
    )
   (
    input  logic clk,

    cci_mpf_if.to_fiu fiu,
    cci_mpf_if.to_afu afu
    );

    assign afu.reset = fiu.reset;
    logic reset;
    assign reset = fiu.reset;

    // Only channel 0 is changed.  All others just pass through.
    assign fiu.c1Tx = afu.c1Tx;
    assign afu.c1TxAlmFull = fiu.c1TxAlmFull;
    assign fiu.c2Tx = afu.c2Tx;
    assign afu.c1Rx = fiu.c1Rx;

    //
    // Use a FIFO to make incoming c0 Tx requests latency-insensitive.
    //
    t_if_cci_mpf_c0_Tx c0Tx_fifo_first;
    logic c0Tx_fifo_deq;
    logic c0Tx_fifo_notEmpty;

    cci_mpf_prim_fifo_lutram
      #(
        .N_DATA_BITS($bits(t_if_cci_mpf_c0_Tx)),
        .N_ENTRIES(CCI_TX_ALMOST_FULL_THRESHOLD + 2),
        .THRESHOLD(CCI_TX_ALMOST_FULL_THRESHOLD)
        )
      c0_fifo(.clk,
              .reset,

              .enq_data(afu.c0Tx),
              // Map the valid bit through as enq here and notEmpty below.
              .enq_en(cci_mpf_c0TxIsValid(afu.c0Tx)),
              .notFull(),
              .almostFull(afu.c0TxAlmFull),

              .first(c0Tx_fifo_first),
              .deq_en(c0Tx_fifo_deq),
              .notEmpty(c0Tx_fifo_notEmpty)
              );

    //
    // A heap preserves the original Mdata and number of cache lines in a
    // request.
    //
    typedef logic [$clog2(MAX_ACTIVE_REQS)-1 : 0] t_req_idx;

    logic rd_heap_notFull;
    t_req_idx rd_heap_allocIdx;

    t_cci_clNum rd_heap_origClNum;
    t_req_idx rd_heap_origMdata;

    t_if_cci_c0_Rx c0Rx_q;


    //
    // Responses...
    //

    // Register c0 responses in order to read the heap
    always_ff @(posedge clk)
    begin
        c0Rx_q <= fiu.c0Rx;
    end

    always_comb
    begin
        //
        // c0 Responses.  Restore state from request stored in local heap.
        //
        afu.c0Rx = c0Rx_q;
        if (cci_c0Rx_isReadRsp(c0Rx_q))
        begin
            afu.c0Rx.hdr.mdata[$clog2(MAX_ACTIVE_REQS)-1 : 0] = rd_heap_origMdata;

            // Convert back to multi-line
            afu.c0Rx.hdr.cl_num = rd_heap_origClNum;
        end
    end


    //
    // Requests...
    //

    //
    // Convert c0 Tx multi-line to single beat requests
    //
    t_cci_clNum beat_num;

    always_comb
    begin
        fiu.c0Tx = 'x;
        fiu.c0Tx.valid = 1'b0;

        c0Tx_fifo_deq = 1'b0;

        // Request to process and space to put it?
        if (rd_heap_notFull && c0Tx_fifo_notEmpty && ! fiu.c0TxAlmFull)
        begin
            fiu.c0Tx = c0Tx_fifo_first;
            fiu.c0Tx.hdr.base.mdata[$clog2(MAX_ACTIVE_REQS)-1 : 0] = rd_heap_allocIdx;

            // Convert to a single line request
            fiu.c0Tx.hdr.base.cl_len = eCL_LEN_1;
            fiu.c0Tx.hdr.base.address[$bits(beat_num)-1:0] =
                fiu.c0Tx.hdr.base.address[$bits(beat_num)-1:0] | beat_num;

            // Done with all beats?
            c0Tx_fifo_deq = (beat_num == c0Tx_fifo_first.hdr.base.cl_len);
        end
    end

    // Track the beat number
    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            beat_num <= t_cci_clNum'(0);
        end
        else if (c0Tx_fifo_deq)
        begin
            // Done with packet
            beat_num <= t_cci_clNum'(0);
        end
        else if (fiu.c0Tx.valid)
        begin
            // In the middle of a multi-beat read
            beat_num <= beat_num + t_cci_clNum'(1);
        end
    end


    //
    // Heap for storing request state and mapping back to response headers.
    //
    cci_mpf_prim_heap
      #(
        .N_ENTRIES(MAX_ACTIVE_REQS),
        .N_DATA_BITS($bits(t_cci_clNum) + $bits(t_req_idx)),
        .MIN_FREE_SLOTS(CCI_TX_ALMOST_FULL_THRESHOLD + 1)
        )
      rd_heap
       (
        .clk,
        .reset,

        .enq(cci_mpf_c0TxIsReadReq(fiu.c0Tx)),
        .enqData({ beat_num, t_req_idx'(c0Tx_fifo_first.hdr.base.mdata) }),
        .notFull(rd_heap_notFull),
        .allocIdx(rd_heap_allocIdx),

        .readReq(t_req_idx'(fiu.c0Rx.hdr.mdata)),
        .readRsp({ rd_heap_origClNum, rd_heap_origMdata }),
        .free(cci_c0Rx_isReadRsp(fiu.c0Rx)),
        .freeIdx(t_req_idx'(fiu.c0Rx.hdr.mdata))
        );

endmodule // cci_mpf_multi_line_read_to_single


//
// Convert beats of a multi-line write request to individual requests since
// the native interface doesn't support them.
//
module cci_mpf_multi_line_write_to_single
   (
    input  logic clk,

    cci_mpf_if.to_fiu fiu,
    cci_mpf_if.to_afu afu
    );

    assign afu.reset = fiu.reset;
    logic reset;
    assign reset = fiu.reset;

    assign fiu.c0Tx = afu.c0Tx;
    assign afu.c0TxAlmFull = fiu.c0TxAlmFull;

    assign afu.c1TxAlmFull = fiu.c1TxAlmFull;

    assign fiu.c2Tx = afu.c2Tx;

    assign afu.c0Rx = fiu.c0Rx;
    assign afu.c1Rx = fiu.c1Rx;

    always_comb
    begin
        fiu.c1Tx = afu.c1Tx;

        // Convert to a single line request.  The address is already correct
        // the the receiving side is already prepared for one write response
        // per line written.
        if (cci_mpf_c1TxIsWriteReq_noCheckValid(afu.c1Tx))
        begin
            fiu.c1Tx.hdr.base.sop = 1'b1;
            fiu.c1Tx.hdr.base.cl_len = eCL_LEN_1;
        end
    end

endmodule // cci_mpf_multi_line_write_to_single

`endif
