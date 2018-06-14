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

`ifndef CCI_TEST_CSRS_VH
`define CCI_TEST_CSRS_VH

`include "cci_csr_if.vh"

//
// A standard interface for accessing CSRs in test modules.  The general
// CSR interface exports a collection of read and write registers with
// undefined semantics.  Tests can use them as needed.
//

// Counter type defines the limit of a run in cycles.
localparam NUM_COUNTER_BITS = 40;
typedef logic [NUM_COUNTER_BITS - 1 : 0] t_cci_test_counter;


//
// There are NUM_TEST_CSRS for reading and writing.  There is no
// relationship between read and write CSRs, even for the same index.
// The CSR definitions here are generic.  The meanings of CSR locations
// are determined by individual test applications.
//
localparam NUM_TEST_CSRS = 8;

interface test_csrs();

    typedef struct packed
    {
        logic [63:0] data;
    }
    t_cpu_rd_csrs;

    typedef struct packed
    {
        logic [63:0] data;
        logic en;
    }
    t_cpu_wr_csrs;

    // Each test must provide an AFU ID
    logic [127:0] afu_id;

    // The test module is not told when a CSR is read.  Values should be
    // held for reads at any time.
    t_cpu_rd_csrs cpu_rd_csrs[0 : NUM_TEST_CSRS-1];

    // Host writes to CSR space.  cpu_wr_csrs_en and a value in cpu_wr_csrs
    // are set for one cycle when a write is detected.
    t_cpu_wr_csrs cpu_wr_csrs[0 : NUM_TEST_CSRS-1];

    // CSR manager port
    modport csr
       (
        input  afu_id,
        input  cpu_rd_csrs,
        output cpu_wr_csrs
        );

    // Test port
    modport test
       (
        output afu_id,
        output cpu_rd_csrs,
        input  cpu_wr_csrs
        );

endinterface // cci_test_csrs

`endif
