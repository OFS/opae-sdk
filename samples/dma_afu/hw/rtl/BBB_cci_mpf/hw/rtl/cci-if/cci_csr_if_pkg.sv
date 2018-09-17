//
// CSR functions to make reading CSRs look similar on CCI-S and CCI-P
//

package cci_csr_if_pkg;
    import cci_mpf_if_pkg::*;

    //
    // Is the incoming request a CSR write?
    //
    function automatic logic cci_csr_isWrite(
        input t_if_cci_c0_Rx r
        );

        return r.mmioWrValid;
    endfunction


    //
    // Is the incoming request a CSR read?
    //
    function automatic logic cci_csr_isRead(
        input t_if_cci_c0_Rx r
        );

        return r.mmioRdValid;
    endfunction


    //
    // Get the CSR tid from a read request.
    //
    function automatic t_cci_tid cci_csr_getTid(
        input t_if_cci_c0_Rx r
        );

        t_cci_c0_ReqMmioHdr h = t_cci_c0_ReqMmioHdr'(r.hdr);
        return h.tid;
    endfunction


    //
    // Get the CSR address of a read/write request.
    //
    function automatic t_cci_mmioAddr cci_csr_getAddress(
        input t_if_cci_c0_Rx r
        );

        t_cci_c0_ReqMmioHdr h = t_cci_c0_ReqMmioHdr'(r.hdr);
        return h.address;
    endfunction

endpackage // cci_csr_if_pkg
