//
// Copyright (c) 2016, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

`ifndef CCI_MPF_CSRS_VH
`define CCI_MPF_CSRS_VH

`include "cci_csr_if.vh"
import cci_mpf_csrs_pkg::*;

//
// MPF implements a single CSR read/write module that connects to the host
// through MMIO reads and writes.  A single module is more efficient, since
// MMIO lacks flow control and requires message buffering.  The CCI MPF CSRs
// interface defines a set of signals that are managed by the CSR module.
// Private modports are defined for each class of MPF shims.
//
//   *** Directions are relative to a shim ***
//

interface cci_mpf_csrs();

    //
    // VTP -- virtual to physical translation
    //

    // Input: page table mode (see cci_mpf_csrs.h)
    t_cci_mpf_vtp_csr_mode vtp_in_mode;
    // Input: page table base address (line address)
    t_cci_clAddr vtp_in_page_table_base;
    logic        vtp_in_page_table_base_valid;
    // Input: invalidate the translation for one page
    t_cci_clAddr vtp_in_inval_page;
    logic        vtp_in_inval_page_valid;

    // Events: these wires fire to indicate an event. The CSR shim sums
    // events into counters.
    logic vtp_out_event_4kb_hit;
    logic vtp_out_event_4kb_miss;
    logic vtp_out_event_2mb_hit;
    logic vtp_out_event_2mb_miss;
    logic vtp_out_event_pt_walk_busy;
    logic vtp_out_event_failed_translation;
    t_cci_clAddr vtp_out_pt_walk_last_vaddr;

    //
    // VC MAP -- Mapping eVC_VA to real physical channels.
    //
    logic [63:0] vc_map_ctrl;
    logic        vc_map_ctrl_valid;

    logic [63:0] vc_map_history;
    logic vc_map_out_event_mapping_changed;


    //
    // Latency QoS -- Throttle request rate to maintain bandwidth but reduce
    //                latency.
    //
    logic [63:0] latency_qos_ctrl;
    logic        latency_qos_ctrl_valid;

    //
    // WRO -- write/read ordering
    //

    // Configuration
    logic [63:0] wro_ctrl;
    logic wro_ctrl_valid;

    // Conflict events
    logic wro_out_event_rr_conflict;  // New read conflicts with old read
    logic wro_out_event_rw_conflict;  // New read conflicts with old write
    logic wro_out_event_wr_conflict;  // New write conflicts with old read
    logic wro_out_event_ww_conflict;  // New write conflicts with old write


    //
    // PWRITE -- Partial writes
    //

    logic pwrite_out_event_pwrite;    // Partial write processed


    // CSR manager port
    modport csr
       (
        output vtp_in_mode,
        output vtp_in_page_table_base,
        output vtp_in_page_table_base_valid,
        output vtp_in_inval_page,
        output vtp_in_inval_page_valid,

        output vc_map_ctrl,
        output vc_map_ctrl_valid,
        input  vc_map_history,

        output latency_qos_ctrl,
        output latency_qos_ctrl_valid,

        output wro_ctrl,
        output wro_ctrl_valid
        );
    modport csr_events
       (
        input  vtp_out_event_4kb_hit,
        input  vtp_out_event_4kb_miss,
        input  vtp_out_event_2mb_hit,
        input  vtp_out_event_2mb_miss,
        input  vtp_out_event_pt_walk_busy,
        input  vtp_out_event_failed_translation,
        input  vtp_out_pt_walk_last_vaddr,

        input  vc_map_out_event_mapping_changed,

        input  wro_out_event_rr_conflict,
        input  wro_out_event_rw_conflict,
        input  wro_out_event_wr_conflict,
        input  wro_out_event_ww_conflict,

        input  pwrite_out_event_pwrite
        );

    modport vtp
       (
        input  vtp_in_mode,
        input  vtp_in_page_table_base,
        input  vtp_in_page_table_base_valid,
        input  vtp_in_inval_page,
        input  vtp_in_inval_page_valid
        );
    modport vtp_events
       (
        output vtp_out_event_4kb_hit,
        output vtp_out_event_4kb_miss,
        output vtp_out_event_2mb_hit,
        output vtp_out_event_2mb_miss,
        output vtp_out_event_pt_walk_busy,
        output vtp_out_event_failed_translation,
        output vtp_out_pt_walk_last_vaddr
        );

    modport vc_map
       (
        input  vc_map_ctrl,
        input  vc_map_ctrl_valid,
        output vc_map_history
        );
    modport vc_map_events
       (
        output vc_map_out_event_mapping_changed
        );

    modport latency_qos
       (
        input  latency_qos_ctrl,
        input  latency_qos_ctrl_valid
        );

    modport wro
       (
        input  wro_ctrl,
        input  wro_ctrl_valid
        );
    modport wro_events
       (
        output wro_out_event_rr_conflict,
        output wro_out_event_rw_conflict,
        output wro_out_event_wr_conflict,
        output wro_out_event_ww_conflict
        );

    modport pwrite_events
       (
        output pwrite_out_event_pwrite
        );

endinterface // cci_mpf_csrs

`endif
