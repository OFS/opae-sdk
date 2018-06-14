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
//

//
// LUT RAM
//

module cci_mpf_prim_lutram
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,

    // Read before write or write before read?  Valid values are OLD_DATA,
    // NEW_DATA and DONT_CARE.
    parameter READ_DURING_WRITE = "OLD_DATA"
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr,
    output logic [N_DATA_BITS-1 : 0] rdata,

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );

    // Quartus uses Block RAM when N_DATA_BITS is 1 even when MLAB is
    // specified, perhaps because the MUX required for 1 bit access is slow.
    // The resulting timing is quite different.  In the spirit of giving
    // coders what they asked for, code here forces 1 bit memory to LUTRAM.
    // Changing N_DATA_BITS to 2 and passing in a dummy bit causes Quartus
    // to generate LUTRAM and doesn't waste space because the extra bit is
    // dropped in synthesis.
    localparam REQ_DATA_WIDTH = (N_DATA_BITS == 1) ? 2 : N_DATA_BITS;

    logic [REQ_DATA_WIDTH-1 : 0] data[0 : N_ENTRIES-1] /* synthesis ramstyle = "MLAB, no_rw_check" */;

    generate
        if (READ_DURING_WRITE == "OLD_DATA")
        begin : rbw
            //
            // Delay writes by a cycle for timing and add a bypass to guarantee
            // there is never a read/write of the same location in a single cycle.
            //
            logic [$clog2(N_ENTRIES)-1 : 0] waddr_q;
            logic wen_q;
            logic [N_DATA_BITS-1 : 0] wdata_q;

            // Bypass read response when writing the same location
            assign rdata = (! wen_q || (raddr != waddr_q)) ?
                           N_DATA_BITS'(data[raddr]) :
                           wdata_q;

            always_ff @(posedge clk)
            begin
                waddr_q <= waddr;
                wen_q <= wen;
                wdata_q <= wdata;

                if (wen_q)
                begin
                    data[waddr_q] <= REQ_DATA_WIDTH'(wdata_q);
                end
            end
        end
        else if (READ_DURING_WRITE == "NEW_DATA")
        begin : wbr
            //
            // Bypass from writes this cycle to reads this cycle, forwarding
            // the new value to the reader.
            //
            assign rdata = (! wen || (raddr != waddr)) ?
                           N_DATA_BITS'(data[raddr]) :
                           wdata;

            always_ff @(posedge clk)
            begin
                if (wen)
                begin
                    data[waddr] <= REQ_DATA_WIDTH'(wdata);
                end
            end
        end
        else
        begin : dc
            //
            // Don't care.
            //
            assign rdata = N_DATA_BITS'(data[raddr]);

            always_ff @(posedge clk)
            begin
                if (wen)
                begin
                    data[waddr] <= REQ_DATA_WIDTH'(wdata);
                end
            end
        end
    endgenerate

endmodule // cci_mpf_prim_lutram



//
// LUT RAM initialized with a constant on reset.
//
module cci_mpf_prim_lutram_init
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,

    parameter INIT_VALUE = N_DATA_BITS'(0),
    parameter READ_DURING_WRITE = "OLD_DATA"
    )
   (
    input  logic clk,
    input  logic reset,
    // Goes high after initialization complete and stays high.
    output logic rdy,


    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr,
    output logic [N_DATA_BITS-1 : 0] rdata,

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_local;
    logic wen_local;
    logic [N_DATA_BITS-1 : 0] wdata_local;

    cci_mpf_prim_lutram
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .READ_DURING_WRITE(READ_DURING_WRITE)
        )
      ram
       (
        .clk,
        .reset,

        .raddr,
        .rdata,

        .waddr(waddr_local),
        .wen(wen_local),
        .wdata(wdata_local)
        );


    //
    // Initialization loop
    //

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_init;

    assign waddr_local = rdy ? waddr : waddr_init;
    assign wen_local = rdy ? wen : 1'b1;
    assign wdata_local = rdy ? wdata : INIT_VALUE;

    always_ff @(posedge clk)
    begin
        if (! rdy)
        begin
            waddr_init <= waddr_init + 1;
            rdy <= &(waddr_init);
        end

        if (reset)
        begin
            rdy <= 1'b0;
            waddr_init <= 0;
        end
    end

endmodule // cci_mpf_prim_lutram_init



// ========================================================================
//
//   Banked, multi-cycle read LUTRAM that moves some of the read MUX to
//   the second cycle.  This allows larger memories to work at high speed.
//
// ========================================================================

module cci_mpf_prim_lutram_banked
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    parameter READ_DURING_WRITE = "OLD_DATA",

    // Both this and N_ENTRIES should be powers of 2
    parameter N_BANKS = 2
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr,
    output logic [N_DATA_BITS-1 : 0] T1_rdata,

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );

    typedef logic [$clog2(N_BANKS)-1 : 0] t_bidx;
    typedef logic [$clog2(N_ENTRIES)-$bits(t_bidx)-1 : 0] t_baddr;

    //
    // Underlying banks used for storage.  The banks are in DONT_CARE mode.
    // Scheduling the write and managing read/write to the same address
    // are handled later.
    //

    t_bidx b_ridx, b_ridx_q;
    t_baddr b_raddr;
    assign {b_ridx, b_raddr} = raddr;

    t_bidx b_widx;
    t_baddr b_waddr;
    logic [N_DATA_BITS-1 : 0] b_wdata;
    logic b_wen[0 : N_BANKS-1];

    logic [N_DATA_BITS-1 : 0] rdata[0 : N_BANKS-1];
    // Preserve this to prevent Quartus from merging the register into the RAM.
    // We have balanced the work expecting that the data will be read and
    // extracted from the MLAB in the first cycle and demultiplexed from
    // rdata_q in the second read cycle.
    logic [N_DATA_BITS-1 : 0] rdata_q[0 : N_BANKS-1] /* synthesis preserve */;

    // Registered read response from each bank
    always_ff @(posedge clk)
    begin
        b_ridx_q <= b_ridx;
        rdata_q <= rdata;
    end

    genvar p;
    generate
        for (p = 0; p < N_BANKS; p = p + 1)
        begin : m
            cci_mpf_prim_lutram
              #(
                .N_ENTRIES(N_ENTRIES / N_BANKS),
                .N_DATA_BITS(N_DATA_BITS),
                .READ_DURING_WRITE("DONT_CARE")
                )
            ram
               (
                .clk,
                .reset,

                .raddr(b_raddr),
                .rdata(rdata[p]),

                // Write parameters depend on the hazard mode and are set below.
                .waddr(b_waddr),
                .wen(b_wen[p]),
                .wdata(b_wdata)
                );
        end
    endgenerate

    generate
        if (READ_DURING_WRITE == "OLD_DATA")
        begin : rbw
            //
            // Delay writes by a cycle for timing and add a bypass to guarantee
            // there is never a read/write of the same location in a single cycle.
            //
            logic bypass_en;
            logic [$clog2(N_ENTRIES)-1 : 0] waddr_q;
            logic wen_q;
            logic [N_DATA_BITS-1 : 0] wdata_q;

            assign T1_rdata = (! bypass_en) ? rdata_q[b_ridx_q] : wdata_q;

            always_ff @(posedge clk)
            begin
                bypass_en <= wen_q && (raddr == waddr_q);

                waddr_q <= waddr;
                wen_q <= wen;
                wdata_q <= wdata;

                // Set the bank write details
                {b_widx, b_waddr} <= waddr;
            end

            always_comb
            begin
                b_wdata = wdata_q;

                for (int b = 0; b < N_BANKS; b = b + 1)
                begin
                    b_wen[b] = wen_q && (b_widx == t_bidx'(b));
                end
            end
        end
        else if (READ_DURING_WRITE == "NEW_DATA")
        begin : wbr
            //
            // Bypass from writes this cycle to reads this cycle, forwarding
            // the new value to the reader.
            //
            logic bypass_en;
            logic [N_DATA_BITS-1 : 0] wdata_q;

            always_ff @(posedge clk)
            begin
                bypass_en <= wen && (raddr == waddr);
                wdata_q <= wdata;
            end

            assign T1_rdata = (! bypass_en) ? rdata_q[b_ridx_q] : wdata_q;

            assign {b_widx, b_waddr} = waddr;
            assign b_wdata = wdata;

            always_comb
            begin
                for (int b = 0; b < N_BANKS; b = b + 1)
                begin
                    b_wen[b] = wen && (b_widx == t_bidx'(b));
                end
            end
        end
        else
        begin : dc
            //
            // Don't care.
            //
            assign T1_rdata = rdata_q[b_ridx_q];
            assign {b_widx, b_waddr} = waddr;
            assign b_wdata = wdata;

            always_comb
            begin
                for (int b = 0; b < N_BANKS; b = b + 1)
                begin
                    b_wen[b] = wen && (b_widx == t_bidx'(b));
                end
            end
        end
    endgenerate

endmodule // cci_mpf_prim_lutram_banked


module cci_mpf_prim_lutram_init_banked
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    parameter INIT_VALUE = N_DATA_BITS'(0),
    parameter READ_DURING_WRITE = "OLD_DATA",

    // Both this and N_ENTRIES should be powers of 2
    parameter N_BANKS = 2
    )
   (
    input  logic clk,
    input  logic reset,
    output logic rdy,

    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr,
    output logic [N_DATA_BITS-1 : 0] T1_rdata,

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_local;
    logic wen_local;
    logic [N_DATA_BITS-1 : 0] wdata_local;

    cci_mpf_prim_lutram_banked
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .READ_DURING_WRITE(READ_DURING_WRITE),
        .N_BANKS(N_BANKS)
        )
      ram
       (
        .clk,
        .reset,

        .raddr,
        .T1_rdata,

        .waddr(waddr_local),
        .wen(wen_local),
        .wdata(wdata_local)
        );


    //
    // Initialization loop
    //

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_init;

    assign waddr_local = rdy ? waddr : waddr_init;
    assign wen_local = rdy ? wen : 1'b1;
    assign wdata_local = rdy ? wdata : INIT_VALUE;

    always_ff @(posedge clk)
    begin
        if (! rdy)
        begin
            waddr_init <= waddr_init + 1;
            rdy <= &(waddr_init);
        end

        if (reset)
        begin
            rdy <= 1'b0;
            waddr_init <= 0;
        end
    end

endmodule // cci_mpf_prim_lutram_init_banked


// ========================================================================
//
//   LUTRAM with multiple read ports using simple replication.
//
// ========================================================================


module cci_mpf_prim_lutram_multiread
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    parameter N_READERS = 1
    )
   (
    input  logic clk,
    input  logic reset,

    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr[0 : N_READERS-1],
    output logic [N_DATA_BITS-1 : 0] rdata[0 : N_READERS-1],

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );
    genvar p;
    generate
        for (p = 0; p < N_READERS; p = p + 1)
        begin : rp
            cci_mpf_prim_lutram
              #(
                .N_ENTRIES(N_ENTRIES),
                .N_DATA_BITS(N_DATA_BITS)
                )
              b
               (
                .clk,
                .reset,
                .raddr(raddr[p]),
                .rdata(rdata[p]),
                .waddr,
                .wen,
                .wdata
                );
        end
    endgenerate

endmodule // cci_mpf_prim_lutram_multiread


//
// LUT RAM initialized with a constant on reset.
//
module cci_mpf_prim_lutram_multiread_init
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    parameter N_READERS = 1,

    parameter INIT_VALUE = N_DATA_BITS'(0)
    )
   (
    input  logic clk,
    input  logic reset,
    // Goes high after initialization complete and stays high.
    output logic rdy,


    input  logic [$clog2(N_ENTRIES)-1 : 0] raddr[0 : N_READERS-1],
    output logic [N_DATA_BITS-1 : 0] rdata[0 : N_READERS-1],

    input  logic [$clog2(N_ENTRIES)-1 : 0] waddr,
    input  logic wen,
    input  logic [N_DATA_BITS-1 : 0] wdata
    );

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_local;
    logic wen_local;
    logic [N_DATA_BITS-1 : 0] wdata_local;

    cci_mpf_prim_lutram_multiread
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .N_READERS(N_READERS)
        )
      ram
       (
        .clk,
        .reset,

        .raddr,
        .rdata,

        .waddr(waddr_local),
        .wen(wen_local),
        .wdata(wdata_local)
        );


    //
    // Initialization loop
    //

    logic [$clog2(N_ENTRIES)-1 : 0] waddr_init;

    assign waddr_local = rdy ? waddr : waddr_init;
    assign wen_local = rdy ? wen : 1'b1;
    assign wdata_local = rdy ? wdata : INIT_VALUE;

    always_ff @(posedge clk)
    begin
        if (reset)
        begin
            rdy <= 1'b0;
            waddr_init <= 0;
        end
        else if (! rdy)
        begin
            waddr_init <= waddr_init + 1;
            rdy <= &(waddr_init);
        end
    end

endmodule // cci_mpf_prim_lutram_multiread_init
