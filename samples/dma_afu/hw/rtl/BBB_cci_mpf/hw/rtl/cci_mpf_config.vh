//
// This file contains preprocessor variable definitions that configure some
// parameters internal to MPF modules.  There are too many parameters to
// expose them all in the cci_mpf module, so a wider collection is exposed here.
//
// Projects can update parameters defined here one of two ways:
//
//   1.  Redefine the parameter in the compilation configuration using
//       Quartus "set_global_assignment -name VERILOG_MACRO" and the
//       +define+ argument to simulators.
//
//   2.  Update this file.
//

// ========================================================================
//
// VTP
//
// ========================================================================

//
// Configure the sizes of the L1 TLB caches.  There are separate, direct
// mapped caches for 4KB and 2MB pages and for reads (c0) and writes (c1).
// For most workloads the default values will be optimal. For workloads
// with extremely poor locality, larger L1 caches may improve performance.
//
`ifndef VTP_N_C0_L1_4KB_CACHE_ENTRIES
  `define VTP_N_C0_L1_4KB_CACHE_ENTRIES 512
`endif

`ifndef VTP_N_C0_L1_2MB_CACHE_ENTRIES
  `define VTP_N_C0_L1_2MB_CACHE_ENTRIES 512
`endif

`ifndef VTP_N_C1_L1_4KB_CACHE_ENTRIES
  `define VTP_N_C1_L1_4KB_CACHE_ENTRIES 512
`endif

`ifndef VTP_N_C1_L1_2MB_CACHE_ENTRIES
  `define VTP_N_C1_L1_2MB_CACHE_ENTRIES 512
`endif


`ifndef VTP_N_TLB_4KB_SETS
  // Making this smaller than 512 will save no space since sets are mapped
  // to block RAM and this is the minimum memory depth.
  `define  VTP_N_TLB_4KB_SETS 512
`endif

`ifndef VTP_N_TLB_4KB_WAYS
  // Values larger than 4 are unlikely to meet timing.
  `define  VTP_N_TLB_4KB_WAYS 4
`endif

`ifndef VTP_N_TLB_2MB_SETS
  // Making this smaller than 512 will save no space since sets are mapped
  // to block RAM and this is the minimum memory depth.
  `define  VTP_N_TLB_2MB_SETS 512
`endif

`ifndef VTP_N_TLB_2MB_WAYS
  // Values larger than 4 are unlikely to meet timing.
  `define  VTP_N_TLB_2MB_WAYS 4
`endif


// ========================================================================
//
// WRO
//
// ========================================================================

`ifndef WRO_ADDRESS_HASH_BITS
  // Larger hash spaces reduce false positives but consume more block RAM.
  `ifndef CCI_SIMULATION
    // Building for real hardware.
    `define WRO_ADDRESS_HASH_BITS 14
  `else
    // We make the filter/hash space smaller in simulation solely because
    // the filters are initialized with a loop on reset and running through
    // 16K entries is too slow in simulation.
    `define WRO_ADDRESS_HASH_BITS 11
  `endif
`endif

`ifndef WRO_RD_FILTER_BUCKET_BITS
  // This filter must be at least 3 bits.  See cci_mpf_shim_wro_cam_group.sv.
  `define WRO_RD_FILTER_BUCKET_BITS 5
`endif

