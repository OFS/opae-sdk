//
// MPF's view of CCI expressed as a SystemVerilog interface.
//

`ifndef CCI_MPF_IF_VH
`define CCI_MPF_IF_VH

`include "cci_mpf_platform.vh"

import cci_mpf_if_pkg::*;
import ccip_if_pkg::*;
import ccip_if_funcs_pkg::*;

// Global log file handle
int cci_mpf_if_log_fd = -1;

interface cci_mpf_if
  #(
    parameter ENABLE_LOG = 0,        // Log events for this instance?
    parameter LOG_NAME = "cci_mpf_if.tsv"
    )
   (
    input logic clk
    );

    // Reset flows from FIU to AFU
    logic              reset;

    // Requests to FIU.  All objects are outputs flowing toward FIU except
    // the almost full ports, which provide flow control.
    t_if_cci_mpf_c0_Tx c0Tx;
    logic              c0TxAlmFull;

    t_if_cci_mpf_c1_Tx c1Tx;
    logic              c1TxAlmFull;

    // MMIO read response channel -- new in CCI-P interface
    t_if_cci_c2_Tx     c2Tx;

    // Responses from FIU.  All objects are inputs from the FIU and flow
    // toward the AFU.  There is no flow control.  The AFU must be prepared
    // to receive responses for all in-flight requests.
    t_if_cci_c0_Rx     c0Rx;
    t_if_cci_c1_Rx     c1Rx;

    // Port directions for connections in the direction of the FIU (platform)
    modport to_fiu
      (
       input  reset,

       output c0Tx,
       input  c0TxAlmFull,

       output c1Tx,
       input  c1TxAlmFull,

       output c2Tx,

       input  c0Rx,
       input  c1Rx
       );

    // Port directions for connections in the direction of the AFU (user code)
    modport to_afu
      (
       output reset,

       input  c0Tx,
       output c0TxAlmFull,

       input  c1Tx,
       output c1TxAlmFull,

       input  c2Tx,

       output c0Rx,
       output c1Rx
       );


    // ====================================================================
    //
    // Snoop equivalents of the above interfaces: all the inputs and none
    // of the outputs.
    //
    // ====================================================================

    modport to_fiu_snoop
      (
       input  reset,

       input  c0TxAlmFull,
       input  c1TxAlmFull,

       input  c0Rx,
       input  c1Rx
       );

    modport to_afu_snoop
      (
       input  reset,

       input  c0Tx,
       input  c1Tx,
       input  c2Tx
       );

    modport snoop
      (
       input  reset,

       input  c0TxAlmFull,
       input  c1TxAlmFull,

       input  c0Rx,
       input  c1Rx,

       input  c0Tx,
       input  c1Tx,
       input  c2Tx
       );


    // ====================================================================
    //
    //   Debugging
    //
    // ====================================================================

`ifdef CCI_SIMULATION

`include "cci_mpf_if_dbg.vh"

`endif

endinterface

`endif //  CCI_MPF_IF_VH
