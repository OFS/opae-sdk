//
// Device feature lists describe capabilities of an AFU and the MMIO
// space used to control them from the host.  Feature lists are exported
// in the CCI-P interface as MMIO reads from the host.
//

package ccip_feature_list_pkg;

    parameter CCIP_FEATURE_OFFSET_NEXT_WIDTH = 24;
    parameter CCIP_FEATURE_ID_WIDTH = 12;

    // Base types
    // --------------------------------------------------------------------
    typedef logic [CCIP_FEATURE_OFFSET_NEXT_WIDTH-1:0] t_ccip_feature_next;
    typedef logic [CCIP_FEATURE_ID_WIDTH-1:0] t_ccip_feature_id;

    // Feature type
    typedef enum logic [3:0] {
        eFTYP_AFU = 4'h1,
        eFTYP_BBB = 4'h2,
        eFTYP_PVT = 4'h3
    } t_ccip_feature_type;

    //
    // DFH -- Device Feature Header:
    //   Generic headers describe the meaning and size of a region.  They also
    //   point the next DFH, forming a linked list of features.
    //
    typedef struct packed {
        t_ccip_feature_type f_type;
        logic [18:0]        rsvd0;
        logic               eol;
        t_ccip_feature_next nextFeature;
        logic [3:0]         version;
        t_ccip_feature_id   id;
    } t_ccip_dfh;

    function automatic t_ccip_dfh ccip_dfh_defaultDFH();
        t_ccip_dfh dfh;
        dfh = t_ccip_dfh'(0);

        dfh.f_type = eFTYP_PVT;
        return dfh;
    endfunction

endpackage // ccip_feature_list_pkg
