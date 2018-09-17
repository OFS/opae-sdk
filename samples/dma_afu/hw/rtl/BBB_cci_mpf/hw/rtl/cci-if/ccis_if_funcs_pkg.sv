//
// Functions that operate on CCI-S data types.
//

package ccis_if_funcs_pkg;

    import ccis_if_pkg::*;

    function automatic t_ccis_ReqMemHdr ccis_updMemReqHdrRsvd(
        input t_ccis_ReqMemHdr h
        );

        t_ccis_ReqMemHdr h_out = h;
        h_out.rsvd2 = 0;
        h_out.rsvd1 = 0;
        h_out.rsvd0 = 0;

        return h_out;
    endfunction

    function automatic t_if_ccis_c0_Tx ccis_c0Tx_clearValids();
        t_if_ccis_c0_Tx r = 'x;
        r.rdValid = 0;
        return r;
    endfunction

    function automatic t_if_ccis_c1_Tx ccis_c1Tx_clearValids();
        t_if_ccis_c1_Tx r = 'x;
        r.wrValid = 0;
        r.intrValid = 0;
        return r;
    endfunction

    function automatic t_if_ccis_c0_Rx ccis_c0Rx_clearValids();
        t_if_ccis_c0_Rx r = 'x;
        r.wrValid = 0;
        r.rdValid = 0;
        r.cfgValid = 0;
        r.umsgValid = 0;
        r.intrValid = 0;
        return r;
    endfunction

    function automatic t_if_ccis_c1_Rx ccis_c1Rx_clearValids();
        t_if_ccis_c1_Rx r = 'x;
        r.wrValid = 0;
        r.intrValid = 0;
        return r;
    endfunction

    function automatic logic ccis_c0Rx_isValid(
        input t_if_ccis_c0_Rx r
        );

        return r.wrValid ||
               r.rdValid ||
               r.cfgValid ||
               r.umsgValid ||
               r.intrValid;
    endfunction

    function automatic logic ccis_c1Rx_isValid(
        input t_if_ccis_c1_Rx r
        );

        return r.wrValid ||
               r.intrValid;
    endfunction

endpackage // ccis_if_funcs_pkg
