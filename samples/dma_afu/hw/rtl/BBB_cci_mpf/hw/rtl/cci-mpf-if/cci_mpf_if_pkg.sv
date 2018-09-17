//
// Abstract CCI wrapper around hardware-specific CCI specifications.
//
// In addition to providing functions for accessing an updating CCI data
// structions, the abstraction extends the CCI header to add support
// for virtual memory addresses as well as control of memory protocol
// factory (MPF) features such as enabling or disabling memory ordering.
//
// Naming:
//
//   - This module imports data structures from the base interface
//     (e.g. CCI-P) and renames the underlying data structures as
//     t_cci_... from the version-specific names, e.g. t_ccip_....
//
//   - MPF-specific data structures are extensions of t_cci structures
//     and are named t_cci_mpf_....
//

// ========================================================================
//
//  Before importing this module, define exactly one preprocessor macro
//  to specify the physical interface.  E.g. MPF_PLATFORM_BDX.
//
// ========================================================================

`include "cci_mpf_platform.vh"

package cci_mpf_if_pkg;
    import ccis_if_pkg::*;
    import ccis_if_funcs_pkg::*;

    import ccip_if_pkg::*;
    import ccip_if_funcs_pkg::*;

    //
    // MPF's CCI is an extension of the platform's base CCI:
    //
    //  - It offers virtual addressing. The base CCI address field is
    //    defined to be large enough for either virtual or physical
    //    addresses. MPF adds a flag to indicate whether a given request's
    //    address is virtual or physical.
    //
    //  - MPF provides functions for platform-independent manipulation
    //    of CCI structures. It defines platform agnostic names,
    //    e.g. t_cci_clAddr instead of t_ccip_clAddr. Objects with
    //    MPF-specific data are given a name that includes "mpf", such
    //    as t_cci_mpf_ReqMemHdr.
    //

    //
    // Derive abstract CCI from CCI-P, which is a superset of all previous
    // platforms.
    //
    parameter CCI_CLADDR_WIDTH = CCIP_CLADDR_WIDTH;
    parameter CCI_CLDATA_WIDTH = CCIP_CLDATA_WIDTH;

    parameter CCI_MMIOADDR_WIDTH = CCIP_MMIOADDR_WIDTH;
    parameter CCI_MMIODATA_WIDTH = CCIP_MMIODATA_WIDTH;

    parameter CCI_MDATA_WIDTH = CCIP_MDATA_WIDTH;

`ifdef MPF_HOST_IFC_CCIS
    parameter CCI_PLATFORM_MDATA_WIDTH = CCIS_MDATA_WIDTH;
`elsif MPF_HOST_IFC_CCIP
    parameter CCI_PLATFORM_MDATA_WIDTH = CCIP_MDATA_WIDTH;
`else
    ** ERROR: Select a valid platform
`endif

    // Always use the CCI-P almost full threshold since we emulate
    // CCI-P style multi-beat writes on CCI-S.
    parameter CCI_TX_ALMOST_FULL_THRESHOLD = CCIP_TX_ALMOST_FULL_THRESHOLD;

    typedef t_ccip_clAddr t_cci_clAddr;
    typedef t_ccip_clData t_cci_clData;
    typedef t_ccip_mdata t_cci_mdata;

    // True platform mdata width, which is less on CCI-S
    typedef logic [CCI_MDATA_WIDTH-1 : 0] t_cci_mdata_platform;

    typedef t_ccip_vc t_cci_vc;
    typedef t_ccip_clLen t_cci_clLen;
    typedef t_ccip_clNum t_cci_clNum;
    typedef t_ccip_qwIdx t_cci_qwIdx;
    // Maximum number of beats in a multi-line request
    parameter CCI_MAX_MULTI_LINE_BEATS = 1 << $bits(t_cci_clNum);

    typedef t_ccip_mmioAddr t_cci_mmioAddr;
    typedef t_ccip_mmioData t_cci_mmioData;
    typedef t_ccip_tid t_cci_tid;

    typedef t_ccip_c0_req t_cci_c0_req;
    typedef t_ccip_c1_req t_cci_c1_req;
    typedef t_ccip_c0_rsp t_cci_c0_rsp;
    typedef t_ccip_c1_rsp t_cci_c1_rsp;

    typedef t_ccip_c0_ReqMemHdr t_cci_c0_ReqMemHdr;
    parameter CCI_C0TX_HDR_WIDTH = CCIP_C0TX_HDR_WIDTH;
    typedef t_ccip_c1_ReqMemHdr t_cci_c1_ReqMemHdr;
    parameter CCI_C1TX_HDR_WIDTH = CCIP_C1TX_HDR_WIDTH;

    typedef t_ccip_c0_RspMemHdr t_cci_c0_RspMemHdr;
    parameter CCI_C0RX_HDR_WIDTH = CCIP_C0RX_HDR_WIDTH;
    typedef t_ccip_c1_RspMemHdr t_cci_c1_RspMemHdr;
    parameter CCI_C1RX_HDR_WIDTH = CCIP_C1RX_HDR_WIDTH;

    // Memory fence request/response variants
    typedef t_ccip_c1_ReqFenceHdr t_cci_c1_ReqFenceHdr;
    typedef t_ccip_c1_RspFenceHdr t_cci_c1_RspFenceHdr;

    // MMIO request/response variants
    typedef t_ccip_c0_ReqMmioHdr t_cci_c0_ReqMmioHdr;
    typedef t_ccip_c2_RspMmioHdr t_cci_c2_RspMmioHdr;

    typedef t_if_ccip_c0_Tx t_if_cci_c0_Tx;
    typedef t_if_ccip_c1_Tx t_if_cci_c1_Tx;
    typedef t_if_ccip_c2_Tx t_if_cci_c2_Tx;
    typedef t_if_ccip_Tx t_if_cci_Tx;

    typedef t_if_ccip_c0_Rx t_if_cci_c0_Rx;
    typedef t_if_ccip_c1_Rx t_if_cci_c1_Rx;
    typedef t_if_ccip_Rx t_if_cci_Rx;

    function automatic t_cci_c0_ReqMemHdr cci_c0_updMemReqHdrRsvd(
        input t_cci_c0_ReqMemHdr h
        );
        return ccip_c0_updMemReqHdrRsvd(h);
    endfunction

    function automatic t_cci_c1_ReqMemHdr cci_c1_updMemReqHdrRsvd(
        input t_cci_c1_ReqMemHdr h
        );
        return ccip_c1_updMemReqHdrRsvd(h);
    endfunction

    function automatic t_if_cci_c0_Tx cci_c0Tx_clearValids();
        return ccip_c0Tx_clearValids();
    endfunction

    function automatic t_if_cci_c1_Tx cci_c1Tx_clearValids();
        return ccip_c1Tx_clearValids();
    endfunction

    function automatic t_if_cci_c0_Rx cci_c0Rx_clearValids();
        return ccip_c0Rx_clearValids();
    endfunction

    function automatic t_if_cci_c1_Rx cci_c1Rx_clearValids();
        return ccip_c1Rx_clearValids();
    endfunction

    function automatic logic cci_c0Rx_isValid(
        input t_if_cci_c0_Rx r
        );
        return ccip_c0Rx_isValid(r);
    endfunction

    function automatic logic cci_c1Rx_isValid(
        input t_if_cci_c1_Rx r
        );
        return ccip_c1Rx_isValid(r);
    endfunction

    function automatic logic cci_c0Rx_isReadRsp(
        input t_if_cci_c0_Rx r
        );
        return ccip_c0Rx_isReadRsp(r);
    endfunction

    function automatic logic cci_c1Rx_isWriteRsp(
        input t_if_cci_c1_Rx r
        );
        return ccip_c1Rx_isWriteRsp(r);
    endfunction

    function automatic logic cci_c1Rx_isWriteFenceRsp(
        input t_if_cci_c1_Rx r
        );
        return ccip_c1Rx_isWriteFenceRsp(r);
    endfunction


    // ====================================================================
    //
    //   Platform parameters
    //
    // ====================================================================

    parameter MPF_PLATFORM_NUM_PHYSICAL_CHANNELS = `MPF_PLATFORM_NUM_PHYSICAL_CHANNELS;
    parameter t_cci_vc MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL = t_cci_vc'(`MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL);


    // ====================================================================
    //
    //   MPF-specific header.
    //
    // ====================================================================

    // CCI_CL_ADDR_WIDTH must be at least as large as both the virtual
    // and physical address widths.
    parameter CCI_MPF_CLADDR_WIDTH = CCI_CLADDR_WIDTH;

    //
    // The CCI MPF request header adds fields that are used only for
    // requests flowing from the AFU and through the memory properties
    // factory.  As requests leave MPF and enter the physical CCI the
    // extra fields are dropped.
    //
    // Fields include extra bits to specify virtual addresses and some
    // memory ordering controls.
    //

    //
    // Extension to the request header exposed in the MPF interface to
    // the AFU and used inside MPF.  The extension is dropped before
    // requests reach the FIU.
    //
    typedef struct packed {
        // Enforce load/store and store/store ordering within lines?
        // Setting this to zero bypasses ordering logic for this request.
        logic checkLoadStoreOrder;

        // Map eVC_VA to a real physical channel?
        logic mapVAtoPhysChannel;

        // Is the address in the header virtual (1) or physical (0)?
        logic addrIsVirtual;
    } t_cci_mpf_ReqMemHdrExt;


    //
    // Header for partial (masked) write emulation.
    //
    parameter CCI_CLDATA_NUM_BYTES = CCI_CLDATA_WIDTH / 8;
    typedef logic [CCI_CLDATA_NUM_BYTES-1 : 0] t_cci_mpf_clDataByteMask;

    typedef struct packed {
        t_cci_mpf_clDataByteMask mask;
        logic isPartialWrite;
    } t_cci_mpf_c1_PartialWriteHdr;


    //
    // A full header
    //
    typedef struct packed {
        t_cci_mpf_ReqMemHdrExt ext;

        // The base component must be last in order to preserve the header
        // property that mdata is in the low bits.  Some code treats the
        // header as opaque and manipulates the mdata bits without using
        // the struct fields.
        t_cci_c0_ReqMemHdr     base;
    } t_cci_mpf_c0_ReqMemHdr;
    parameter CCI_MPF_C0TX_MEMHDR_WIDTH = $bits(t_cci_mpf_c0_ReqMemHdr);

    typedef struct packed {
        t_cci_mpf_c1_PartialWriteHdr pwrite;

        t_cci_mpf_ReqMemHdrExt ext;
        t_cci_c1_ReqMemHdr     base;
    } t_cci_mpf_c1_ReqMemHdr;
    parameter CCI_MPF_C1TX_MEMHDR_WIDTH = $bits(t_cci_mpf_c1_ReqMemHdr);


    // ====================================================================
    //
    //   TX channels with MPF extension
    //
    // ====================================================================

    //
    // Rewrite the TX channel structs to include the MPF extended header.
    //

    // Channel 0 : Memory Reads
    typedef struct packed {
        t_cci_mpf_c0_ReqMemHdr hdr;            // Request Header
        logic                  valid;          // Request Valid 
    } t_if_cci_mpf_c0_Tx;

    // Channel 1 : Memory Writes
    typedef struct packed {
        t_cci_mpf_c1_ReqMemHdr hdr;            // Request Header
        t_cci_clData           data;           // Request Data
        logic                  valid;          // Request Valid 
    } t_if_cci_mpf_c1_Tx;


    // ====================================================================
    //
    //   Helper functions to hide the underlying data structures.
    //
    // ====================================================================

    function automatic t_cci_clAddr cci_mpf_c0_getReqAddr(
        input t_cci_mpf_c0_ReqMemHdr h
        );

        return h.base.address;
    endfunction

    function automatic t_cci_clAddr cci_mpf_c1_getReqAddr(
        input t_cci_mpf_c1_ReqMemHdr h
        );

        return h.base.address;
    endfunction


    function automatic logic cci_mpf_c0_getReqCheckOrder(
        input t_cci_mpf_c0_ReqMemHdr h
        );

        return h.ext.checkLoadStoreOrder;
    endfunction

    function automatic logic cci_mpf_c1_getReqCheckOrder(
        input t_cci_mpf_c1_ReqMemHdr h
        );

        return h.ext.checkLoadStoreOrder;
    endfunction


    function automatic logic cci_mpf_c0_getReqMapVA(
        input t_cci_mpf_c0_ReqMemHdr h
        );

        return h.ext.mapVAtoPhysChannel;
    endfunction

    function automatic logic cci_mpf_c1_getReqMapVA(
        input t_cci_mpf_c1_ReqMemHdr h
        );

        return h.ext.mapVAtoPhysChannel;
    endfunction


    function automatic logic cci_mpf_c0_getReqAddrIsVirtual(
        input t_cci_mpf_c0_ReqMemHdr h
        );

        return h.ext.addrIsVirtual;
    endfunction

    function automatic logic cci_mpf_c1_getReqAddrIsVirtual(
        input t_cci_mpf_c1_ReqMemHdr h
        );

        return h.ext.addrIsVirtual;
    endfunction


    // Update an existing request header with a new virtual address.
    function automatic t_cci_mpf_c0_ReqMemHdr cci_mpf_c0_updReqVAddr(
        input t_cci_mpf_c0_ReqMemHdr h,
        input t_cci_clAddr           address
        );

        h.ext.addrIsVirtual = 1'b1;
        h.base.address = address;

        return h;
    endfunction

    function automatic t_cci_mpf_c0_ReqMemHdr cci_mpf_c1_updReqVAddr(
        input t_cci_mpf_c1_ReqMemHdr h,
        input t_cci_clAddr           address
        );

        h.ext.addrIsVirtual = 1'b1;
        h.base.address = address;

        return h;
    endfunction


    // Generate a new request header.  With so many parameters and defaults
    // we use a struct to pass non-basic parameters.
    typedef struct {
        logic       checkLoadStoreOrder;
        logic       mapVAtoPhysChannel;
        logic       addrIsVirtual;
        t_cci_vc    vc_sel;
        t_cci_clLen cl_len;

        // Applies only to writes
        logic       sop;
    } t_cci_mpf_ReqMemHdrParams;

    // Default value for request header construction. It takes only one
    // option (whether or not the reqest is a VA) to keep the interface
    // simple and because answering VA vs. PA separates the two main
    // categories of requests.
    function automatic t_cci_mpf_ReqMemHdrParams cci_mpf_defaultReqHdrParams(
        input int addrIsVirtual = 1
        );

        t_cci_mpf_ReqMemHdrParams p;
        p.checkLoadStoreOrder = 1'b0;      // Default hardware behavior
        p.mapVAtoPhysChannel = 1'b0;
        p.addrIsVirtual = 1'(addrIsVirtual);
        p.vc_sel = MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL;
        p.cl_len = eCL_LEN_1;
        p.sop = 1'b1;
        return p;
    endfunction

    function automatic t_cci_mpf_c0_ReqMemHdr cci_mpf_c0_genReqHdr(
        input t_cci_c0_req              requestType,
        input t_cci_clAddr              address,
        input t_cci_mdata               mdata,
        input t_cci_mpf_ReqMemHdrParams params
        );

        t_cci_mpf_c0_ReqMemHdr h;

        h.base = t_cci_c0_ReqMemHdr'(0);
        h = cci_mpf_c0_updReqVAddr(h, address);

        h.ext.checkLoadStoreOrder = params.checkLoadStoreOrder;
        h.ext.mapVAtoPhysChannel = params.mapVAtoPhysChannel;
        h.ext.addrIsVirtual = params.addrIsVirtual;

        h.base.req_type = requestType;
        h.base.mdata = mdata;
        h.base.vc_sel = params.vc_sel;
        h.base.cl_len = params.cl_len;

        return h;
    endfunction

    function automatic t_cci_mpf_c1_ReqMemHdr cci_mpf_c1_genReqHdr(
        input t_cci_c1_req              requestType,
        input t_cci_clAddr              address,
        input t_cci_mdata               mdata,
        input t_cci_mpf_ReqMemHdrParams params
        );

        t_cci_mpf_c1_ReqMemHdr h;

        h.base = t_cci_c1_ReqMemHdr'(0);
        h = cci_mpf_c1_updReqVAddr(h, address);

        h.pwrite.isPartialWrite = 1'b0;
        h.pwrite.mask = 'x;

        h.ext.checkLoadStoreOrder = params.checkLoadStoreOrder;
        h.ext.mapVAtoPhysChannel = params.mapVAtoPhysChannel;
        h.ext.addrIsVirtual = params.addrIsVirtual;

        h.base.req_type = requestType;
        h.base.mdata = mdata;
        h.base.vc_sel = params.vc_sel;
        h.base.cl_len = params.cl_len;
        h.base.sop = params.sop;

        return h;
    endfunction

    // Same as MPF version of genReqHdr but return only the base header
    function automatic t_cci_c0_ReqMemHdr cci_c0_genReqHdr(
        input t_cci_c0_req              requestType,
        input t_cci_clAddr              address,
        input t_cci_mdata               mdata,
        input t_cci_mpf_ReqMemHdrParams params
        );

        t_cci_mpf_c0_ReqMemHdr h = cci_mpf_c0_genReqHdr(requestType,
                                                        address,
                                                        mdata,
                                                        params);
        return h.base;
    endfunction

    function automatic t_cci_c1_ReqMemHdr cci_c1_genReqHdr(
        input t_cci_c1_req              requestType,
        input t_cci_clAddr              address,
        input t_cci_mdata               mdata,
        input t_cci_mpf_ReqMemHdrParams params
        );

        t_cci_mpf_c1_ReqMemHdr h = cci_mpf_c1_genReqHdr(requestType,
                                                        address,
                                                        mdata,
                                                        params);
        return h.base;
    endfunction


    // Generate a new request header from a base CCI header
    function automatic t_cci_mpf_c0_ReqMemHdr cci_mpf_c0_cvtReqHdrFromBase(
        input t_cci_c0_ReqMemHdr baseHdr
        );

        t_cci_mpf_c0_ReqMemHdr h;

        h.base = baseHdr;

        // Clear the MPF-specific flags in the MPF extended header so
        // that MPF treats the request as a standard CCI request.
        h.ext = 'x;
        h.ext.checkLoadStoreOrder = 0;
        h.ext.mapVAtoPhysChannel = 0;
        h.ext.addrIsVirtual = 0;

        return h;
    endfunction

    function automatic t_cci_mpf_c1_ReqMemHdr cci_mpf_c1_cvtReqHdrFromBase(
        input t_cci_c1_ReqMemHdr baseHdr
        );

        t_cci_mpf_c1_ReqMemHdr h;

        h.base = baseHdr;

        // Clear the MPF-specific flags in the MPF extended header so
        // that MPF treats the request as a standard CCI request.
        h.ext = 'x;
        h.ext.checkLoadStoreOrder = 0;
        h.ext.mapVAtoPhysChannel = 0;
        h.ext.addrIsVirtual = 0;

        h.pwrite.isPartialWrite = 1'b0;
        h.pwrite.mask = 'x;

        return h;
    endfunction


    // Generate a new response header
    function automatic t_cci_c0_RspMemHdr cci_c0_genRspHdr(
        input t_cci_c0_rsp responseType,
        input t_cci_mdata  mdata
        );

        t_cci_c0_RspMemHdr h;
        h = t_cci_c0_RspMemHdr'(0);

        h.resp_type = responseType;
        h.mdata = mdata;

        return h;
    endfunction

    function automatic t_cci_c1_RspMemHdr cci_c1_genRspHdr(
        input t_cci_c1_rsp responseType,
        input t_cci_mdata  mdata
        );

        t_cci_c1_RspMemHdr h;
        h = t_cci_c1_RspMemHdr'(0);

        h.resp_type = responseType;
        h.mdata = mdata;

        return h;
    endfunction


    // Generate an MPF C0 TX from a base struct
    function automatic t_if_cci_mpf_c0_Tx cci_mpf_cvtC0TxFromBase(
        input t_if_cci_c0_Tx b
        );

        t_if_cci_mpf_c0_Tx m;

        m.hdr = cci_mpf_c0_cvtReqHdrFromBase(b.hdr);
        m.valid = b.valid;

        return m;
    endfunction


    // Generate an MPF C1 TX from a base struct
    function automatic t_if_cci_mpf_c1_Tx cci_mpf_cvtC1TxFromBase(
        input t_if_cci_c1_Tx b
        );

        t_if_cci_mpf_c1_Tx m;

        m.hdr = cci_mpf_c1_cvtReqHdrFromBase(b.hdr);
        m.data = b.data;
        m.valid = b.valid;

        return m;
    endfunction


    // Generate a base C0 TX from an MPF struct.
    //  *** This only works if the address stored in the MPF header is
    //  *** physical.
    function automatic t_if_cci_c0_Tx cci_mpf_cvtC0TxToBase(
        input t_if_cci_mpf_c0_Tx m
        );

        t_if_cci_c0_Tx b;

        b.hdr = m.hdr.base;
        b.valid = m.valid;

        return b;
    endfunction


    // Generate a base C1 TX from an MPF struct.
    //  *** This only works if the address stored in the MPF header is
    //  *** physical.
    function automatic t_if_cci_c1_Tx cci_mpf_cvtC1TxToBase(
        input t_if_cci_mpf_c1_Tx m
        );

        t_if_cci_c1_Tx b;

        b.hdr = m.hdr.base;
        b.data = m.data;
        b.valid = m.valid;

        return b;
    endfunction


    // Initialize an MPF C0 TX with all valid bits clear
    function automatic t_if_cci_mpf_c0_Tx cci_mpf_c0Tx_clearValids();
        t_if_cci_mpf_c0_Tx r = 'x;
        r.valid = 1'b0;
        return r;
    endfunction

    // Initialize an MPF C1 TX with all valid bits clear
    function automatic t_if_cci_mpf_c1_Tx cci_mpf_c1Tx_clearValids();
        t_if_cci_mpf_c1_Tx r = 'x;
        r.valid = 1'b0;
        return r;
    endfunction


    // Mask the valid bits in an MPF C0 TX
    function automatic t_if_cci_mpf_c0_Tx cci_mpf_c0TxMaskValids(
        input t_if_cci_mpf_c0_Tx r,
        input logic mask
        );

        r.valid = r.valid && mask;
        return r;
    endfunction

    function automatic t_if_cci_mpf_c0_Tx cci_mpf_c0TxSetValids(
        input t_if_cci_mpf_c0_Tx r,
        input logic valid
        );

        r.valid = valid;
        return r;
    endfunction

    // Mask the valid bits in an MPF C1 TX
    function automatic t_if_cci_mpf_c1_Tx cci_mpf_c1TxMaskValids(
        input t_if_cci_mpf_c1_Tx r,
        input logic mask
        );

        r.valid = r.valid && mask;
        return r;
    endfunction

    function automatic t_if_cci_mpf_c1_Tx cci_mpf_c1TxSetValids(
        input t_if_cci_mpf_c1_Tx r,
        input logic valid
        );

        r.valid = valid;
        return r;
    endfunction


    // Does an MPF C0 TX have a valid request?
    function automatic logic cci_mpf_c0TxIsValid(
        input t_if_cci_mpf_c0_Tx r
        );

        return r.valid;
    endfunction

    // Does an MPF C0 TX have read request?
    function automatic logic cci_mpf_c0TxIsReadReq_noCheckValid(
        input t_if_cci_mpf_c0_Tx r
        );

        return ((r.hdr.base.req_type == eREQ_RDLINE_I) ||
                (r.hdr.base.req_type == eREQ_RDLINE_S));
    endfunction

    // Does an MPF C0 TX have a valid read request?
    function automatic logic cci_mpf_c0TxIsReadReq(
        input t_if_cci_mpf_c0_Tx r
        );

        return r.valid && cci_mpf_c0TxIsReadReq_noCheckValid(r);
    endfunction


    // Does an MPF C1 TX have a valid request?
    function automatic logic cci_mpf_c1TxIsValid(
        input t_if_cci_mpf_c1_Tx r
        );

        return r.valid;
    endfunction

    // Does an MPF C0 TX have a valid write request?
    function automatic logic cci_mpf_c1TxIsWriteReq_noCheckValid(
        input t_if_cci_mpf_c1_Tx r
        );

        return ((r.hdr.base.req_type == eREQ_WRLINE_I) ||
                (r.hdr.base.req_type == eREQ_WRLINE_M)
`ifdef MPF_HOST_IFC_CCIP_WRPUSH
                || (r.hdr.base.req_type == eREQ_WRPUSH_I)
`endif
                );
    endfunction

    // Does an MPF C0 TX have a valid write request?
    function automatic logic cci_mpf_c1TxIsWriteReq(
        input t_if_cci_mpf_c1_Tx r
        );

        return r.valid && cci_mpf_c1TxIsWriteReq_noCheckValid(r);
    endfunction


    function automatic logic cci_mpf_c1TxIsWriteFenceReq_noCheckValid(
        input t_if_cci_mpf_c1_Tx r
        );

        return (r.hdr.base.req_type == eREQ_WRFENCE);
    endfunction

    function automatic logic cci_mpf_c1TxIsWriteFenceReq(
        input t_if_cci_mpf_c1_Tx r
        );

        return r.valid && cci_mpf_c1TxIsWriteFenceReq_noCheckValid(r);
    endfunction


    // Generate an MPF C0 TX read request given a header
    function automatic t_if_cci_mpf_c0_Tx cci_mpf_genC0TxReadReq(
        input t_cci_mpf_c0_ReqMemHdr h,
        input logic valid
        );

        t_if_cci_mpf_c0_Tx r = cci_mpf_c0Tx_clearValids();
        r.hdr = h;
        r.valid = valid;

        return r;
    endfunction

    // Generate an MPF C1 TX write request given a header and data
    function automatic t_if_cci_mpf_c1_Tx cci_mpf_genC1TxWriteReq(
        input t_cci_mpf_c1_ReqMemHdr h,
        input t_cci_clData data,
        input logic valid
        );

        t_if_cci_mpf_c1_Tx r = cci_mpf_c1Tx_clearValids();
        r.hdr = h;
        r.data = data;
        r.valid = valid;

        return r;
    endfunction


    // Canonicalize an MPF C0 TX request
    function automatic t_if_cci_mpf_c0_Tx cci_mpf_updC0TxCanonical(
        input t_if_cci_mpf_c0_Tx r
        );

        t_if_cci_mpf_c0_Tx r_out = r;
        r_out.hdr = cci_mpf_updC0TxCanonicalHdr(r.hdr, cci_mpf_c0TxIsReadReq(r));

        return r_out;
    endfunction

    function automatic t_cci_mpf_c0_ReqMemHdr cci_mpf_updC0TxCanonicalHdr(
        input t_cci_mpf_c0_ReqMemHdr h,
        input logic isReadReq
        );

        t_cci_mpf_c0_ReqMemHdr h_out = h;
        h_out.base.rsvd1 = 0;
        h_out.base.rsvd0 = 0;

        // Force the physical channel on single channel systems
        if (MPF_PLATFORM_NUM_PHYSICAL_CHANNELS == 1)
        begin
            h_out.base.vc_sel = MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL;
        end

        // Extension flags may only be set on read requests
        if (! isReadReq)
        begin
            h_out.ext.checkLoadStoreOrder = 0;
            h_out.ext.mapVAtoPhysChannel = 0;
            h_out.ext.addrIsVirtual = 0;
            h_out.base.cl_len = eCL_LEN_1;
        end

        return h_out;
    endfunction

    // Canonicalize an MPF C1 TX request
    function automatic t_if_cci_mpf_c1_Tx cci_mpf_updC1TxCanonical(
        input t_if_cci_mpf_c1_Tx r
        );

        t_if_cci_mpf_c1_Tx r_out = r;
        r_out.hdr = cci_mpf_updC1TxCanonicalHdr(r.hdr, cci_mpf_c1TxIsWriteReq(r));

        return r_out;
    endfunction

    function automatic t_cci_mpf_c1_ReqMemHdr cci_mpf_updC1TxCanonicalHdr(
        input t_cci_mpf_c1_ReqMemHdr h,
        input logic isWriteReq
        );

        t_cci_mpf_c1_ReqMemHdr h_out = h;
        h_out.base.rsvd1 = 0;
        h_out.base.rsvd0 = 0;

        // Force the physical channel on single channel systems
        if (MPF_PLATFORM_NUM_PHYSICAL_CHANNELS == 1)
        begin
            h_out.base.vc_sel = MPF_PLATFORM_DEFAULT_PHYSICAL_CHANNEL;
        end

        // Extension flags may only be set on write requests
        if (! isWriteReq)
        begin
            h_out.ext.checkLoadStoreOrder = 0;
            h_out.ext.mapVAtoPhysChannel = 0;
            h_out.ext.addrIsVirtual = 0;
            h_out.base.cl_len = eCL_LEN_1;
        end

        return h_out;
    endfunction


    // Does a C2 TX have a valid response?
    function automatic logic cci_mpf_c2TxIsValid(
        input t_if_cci_c2_Tx r
        );

        return r.mmioRdValid;
    endfunction

    function automatic t_if_cci_c2_Tx cci_mpf_c2Tx_clearValids();
        t_if_cci_c2_Tx r = 'x;
        r.mmioRdValid = 1'b0;
        return r;
    endfunction


    //
    // End of packet (EOP) marks the last response corresponding to a single
    // or multi-beat read request.  This is computed by MPF in module
    // cci_mpf_shim_detect_eop since it is not provided by CCI.  The location
    // used to store EOP is subject to change, so use these test functions.
    //
    // There are no functions for c1Rx because the same eop shim merges
    // unpacked write response flits into a single packed response.
    //

    function automatic logic cci_mpf_c0Rx_isEOP(input t_if_cci_c0_Rx r);
        // EOP stored in hdr.rsvd1
        return cci_c0Rx_isValid(r) && r.hdr.rsvd1;
    endfunction

    function automatic t_if_cci_c0_Rx cci_mpf_c0Rx_updEOP(input t_if_cci_c0_Rx r,
                                                          input logic eop);
        t_if_cci_c0_Rx r_out = r;
        r_out.hdr.rsvd1 = eop;
        return r_out;
    endfunction

endpackage // cci_mpf_if
