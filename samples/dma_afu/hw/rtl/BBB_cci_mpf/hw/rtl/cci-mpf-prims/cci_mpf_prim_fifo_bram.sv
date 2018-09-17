//
// Copyright (c) 2015, Intel Corporation
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

//
// FIFO --
//   A FIFO with N_ENTRIES storage elements and signaling almostFull when
//   THRESHOLD or fewer slots are free, stored in block RAM.
//

module cci_mpf_prim_fifo_bram
  #(
    parameter N_DATA_BITS = 32,
    parameter N_ENTRIES = 2,
    parameter THRESHOLD = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [N_DATA_BITS-1 : 0] enq_data,
    input  logic                     enq_en,
    output logic                     notFull,
    output logic                     almostFull,

    output logic [N_DATA_BITS-1 : 0] first,
    input  logic                     deq_en,
    output logic                     notEmpty
    );

    logic sc_full;
    assign notFull = ! sc_full;

    logic sc_empty;
    logic sc_rdreq;

    // Read from BRAM FIFO "first" is available and the FIFO has data.
    assign sc_rdreq = (! notEmpty || deq_en) && ! sc_empty;

    always_ff @(posedge clk)
    begin
        // Not empty if first was already valid and there was no deq or the
        // BRAM FIFO has valid data.
        notEmpty <= (notEmpty && ! deq_en) || ! sc_empty;

        if (reset)
        begin
            notEmpty <= 1'b0;
        end
    end

    scfifo
      #(
        .lpm_numwords(N_ENTRIES),
        .lpm_showahead("OFF"),
        .lpm_type("scfifo"),
        .lpm_width(N_DATA_BITS),
        .lpm_widthu($clog2(N_ENTRIES)),
        .almost_full_value(N_ENTRIES - THRESHOLD),
        .overflow_checking("OFF"),
        .underflow_checking("OFF"),
        .use_eab("ON"),
        .add_ram_output_register("ON")
        )
      scfifo_component
       (
        .clock(clk),
        .sclr(reset),

        .data(enq_data),
        .wrreq(enq_en),
        .full(sc_full),
        .almost_full(almostFull),

        .rdreq(sc_rdreq),
        .q(first),
        .empty(sc_empty),
        .almost_empty(),

        .aclr(),
        .usedw()
`ifndef MPF_PLATFORM_OME
        , .eccstatus()
`endif
        );

    // synthesis translate_off

    always_ff @(posedge clk)
    begin
        if (! reset)
        begin
            assert (! (sc_full && enq_en)) else
                $fatal(2, "ENQ to full SCFIFO");

            assert (notEmpty || ! deq_en) else
                $fatal(2, "DEQ from empty SCFIFO");
        end
    end

    // synthesis translate_on

endmodule // cci_mpf_prim_fifo_bram
