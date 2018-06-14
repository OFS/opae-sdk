//
// Debug output for MPF CCI interface.
//

`ifndef CCI_MPF_IF_DBG_VH
`define CCI_MPF_IF_DBG_VH


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//   This file is INCLUDED by cci_mpf_if.vh.  An interface can't
//   instantiate another module, but it is cleaner to have the debugging
//   code in a separate file.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Print Channel function
    function string print_channel (logic [1:0] vc_sel);
        case (vc_sel)
            2'b00: return "VA ";
            2'b01: return "VL0";
            2'b10: return "VH0";
            2'b11: return "VH1";
        endcase
    endfunction

    // Print Req Type
    function string print_c0_reqtype (t_cci_c0_req req);
        case (req)
            eREQ_RDLINE_S: return "RdLine_S  ";
            eREQ_RDLINE_I: return "RdLine_I  ";
            default:       return "* c0 REQ ERROR * ";
        endcase
    endfunction

    function string print_c1_reqtype (t_cci_c1_req req);
        case (req)
            eREQ_WRLINE_I: return "WrLine_I  ";
            eREQ_WRLINE_M: return "WrLine_M  ";
`ifdef MPF_HOST_IFC_CCIP_WRPUSH
            eREQ_WRPUSH_I: return "WrPush_I  ";
`endif
            eREQ_WRFENCE:  return "WrFence   ";
         // eREQ_ATOMIC:   return "Atomic    ";
            eREQ_INTR:     return "IntrReq   ";
            default:       return "* c1 REQ ERROR * ";
        endcase
    endfunction

    // Print resp type
    function string print_c0_resptype (t_cci_c0_rsp rsp);
        case (rsp)
            eRSP_RDLINE:  return "RdRsp      ";
            eRSP_UMSG:    return "UmsgRsp    ";
         // eRSP_ATOMIC:  return "AtomicRsp  ";
            default:      return "* c0 RSP ERROR *  ";
        endcase
    endfunction

    function string print_c1_resptype (t_cci_c1_rsp rsp);
        case (rsp)
            eRSP_WRLINE:  return "WrRsp      ";
            eRSP_WRFENCE: return "WrFenceRsp ";
            eRSP_INTR:    return "IntrResp   ";
            default:      return "* c1 RSP ERROR *  ";
        endcase
    endfunction

    // Print CSR data
    function int csr_len(logic [1:0] length);
        case (length)
            2'b0: return 4;
            2'b1: return 8;
            2'b10: return 64;
            default: return 0;
        endcase
    endfunction

    logic [1:0] cycle = 2'b0;
    always @(posedge clk)
    begin
        cycle <= cycle + 2'b1;
    end
 
    initial
    begin : logger_proc
        if (cci_mpf_if_log_fd == -1)
        begin
            cci_mpf_if_log_fd = $fopen(LOG_NAME, "w");
        end

        // Watch traffic
        if (ENABLE_LOG != 0)
        begin
            forever @(posedge clk)
            begin
                // //////////////////////// C0 TX CHANNEL TRANSACTIONS //////////////////////////
                /******************* AFU -> MEM Read Request ******************/
                if (! reset && c0Tx.valid)
                begin
                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t\t%s\t%0d\t%s\t%x\t%s%s %x\n",
                            $time,
                            print_channel(c0Tx.hdr.base.vc_sel),
                            c0Tx.hdr.base.cl_len,
                            print_c0_reqtype(c0Tx.hdr.base.req_type),
                            c0Tx.hdr.base.mdata,
                            (c0Tx.hdr.ext.mapVAtoPhysChannel ? "M" : ""),
                            (c0Tx.hdr.ext.addrIsVirtual ? "V" : "P"),
                            c0Tx.hdr.base.address );

                end

                //////////////////////// C1 TX CHANNEL TRANSACTIONS //////////////////////////
                /******************* AFU -> MEM Write Request *****************/
                if (! reset && c1Tx.valid)
                begin
                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t\t%s\t%0d\t%s\t%s\t%x\t%s%s %x\t%x",
                            $time,
                            print_channel(c1Tx.hdr.base.vc_sel),
                            c1Tx.hdr.base.cl_len,
                            (c1Tx.hdr.base.sop ? "S" : "x"),
                            print_c1_reqtype(c1Tx.hdr.base.req_type),
                            c1Tx.hdr.base.mdata,
                            (c1Tx.hdr.ext.mapVAtoPhysChannel ? "M" : ""),
                            (c1Tx.hdr.ext.addrIsVirtual ? "V" : "P"),
                            c1Tx.hdr.base.address,
                            c1Tx.data);

                    if (c1Tx.hdr.pwrite.isPartialWrite)
                    begin
                        $fwrite(cci_mpf_if_log_fd, " PW [%x]",
                                c1Tx.hdr.pwrite.mask);
                    end

                    $fwrite(cci_mpf_if_log_fd, "\n");
                end

                //////////////////////// C2 TX CHANNEL TRANSACTIONS //////////////////////////
                /******************* AFU -> MMIO Read Response *****************/
                if (! reset && c2Tx.mmioRdValid)
                begin
                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t\tMMIORdRsp\t%x\t%x\n",
                            $time,
                            c2Tx.hdr.tid,
                            c2Tx.data);
                end

                //////////////////////// C0 RX CHANNEL TRANSACTIONS //////////////////////////
                /******************* MEM -> AFU Read Response *****************/
                if (! reset && c0Rx.rspValid)
                begin
                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t (%0d)\t%s\t%0d\t%s\t%s\t%x\t%x\n",
                            $time, cycle,
                            print_channel(c0Rx.hdr.vc_used),
                            c0Rx.hdr.cl_num,
                            ((c0Rx.hdr.cl_num == 0) ? "S" :
                               cci_mpf_c0Rx_isEOP(c0Rx) ? "E" : "x"),
                            print_c0_resptype(c0Rx.hdr.resp_type),
                            c0Rx.hdr.mdata,
                            c0Rx.data);
                end

                /****************** MEM -> AFU Write Response *****************/
                if (! reset && c1Rx.rspValid)
                begin
                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t (%0d)\t%s\t%0d\t%s\t%s\t%x\n",
                            $time, cycle,
                            print_channel(c1Rx.hdr.vc_used),
                            c1Rx.hdr.cl_num,
                            (c1Rx.hdr.format ? "F" : "x"),
                            print_c1_resptype(c1Rx.hdr.resp_type),
                            c1Rx.hdr.mdata);
                end

                /******************* SW -> AFU Config Write *******************/
                if (! reset && c0Rx.mmioWrValid)
                begin
                    t_cci_c0_ReqMmioHdr mmio_hdr;
                    mmio_hdr = t_cci_c0_ReqMmioHdr'(c0Rx.hdr);

                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t (%0d)\tMMIOWrReq\t%x\t%d bytes\t%x\t%x\n",
                            $time, cycle,
                            mmio_hdr.tid,
                            csr_len(mmio_hdr.length),
                            mmio_hdr.address,
                            c0Rx.data[63:0]);
                end

                /******************* SW -> AFU Config Read *******************/
                if (! reset && c0Rx.mmioRdValid)
                begin
                    t_cci_c0_ReqMmioHdr mmio_hdr;
                    mmio_hdr = t_cci_c0_ReqMmioHdr'(c0Rx.hdr);

                    $fwrite(cci_mpf_if_log_fd, "%m:\t%t (%0d)\tMMIORdReq\t%x\t%d bytes\t%x\n",
                            $time, cycle,
                            mmio_hdr.tid,
                            csr_len(mmio_hdr.length),
                            mmio_hdr.address);
                end
            end
        end
    end

`endif //  `ifndef CCI_MPF_IF_DBG_VH
