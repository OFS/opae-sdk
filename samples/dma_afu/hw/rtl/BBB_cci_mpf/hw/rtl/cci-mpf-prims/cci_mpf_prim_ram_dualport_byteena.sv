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

//
// Dual port Block RAM -- the same as cci_mpf_ram_dualport but with byte
// enable control.
//

module cci_mpf_prim_ram_dualport_byteena
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    // Number of extra stages of output register buffering to add
    parameter N_OUTPUT_REG_STAGES = 0,

    // Size of a byte
    parameter N_BYTE_BITS = 8,

    // Operation mode, either "BIDIR_DUAL_PORT" or "DUAL_PORT".
    // For DUAL_PORT configure only writes on port a and reads
    // on port b.  This mode can be useful since M20K RAM does
    // not allow 512 x 32 or 512 x 40 modes in bidirectional mode.
    parameter OPERATION_MODE = "BIDIR_DUAL_PORT",

    // Other options are "OLD_DATA" and "NEW_DATA"
    parameter READ_DURING_WRITE_MODE_MIXED_PORTS = "DONT_CARE",

    // Default returns new data for reads on same port as a write.
    // (No NBE read means return X in masked bytes.)
    // Set to OLD_DATA to return the current value.
    parameter READ_DURING_WRITE_MODE_PORT_A = "NEW_DATA_NO_NBE_READ",
    parameter READ_DURING_WRITE_MODE_PORT_B = "NEW_DATA_NO_NBE_READ"
    )
   (
    input  logic clk0,
    input  logic [$clog2(N_ENTRIES)-1 : 0] addr0,
    input  logic wen0,
    input  tri1  [(N_DATA_BITS / N_BYTE_BITS)-1 : 0] byteena0,
    input  logic [N_DATA_BITS-1 : 0] wdata0,
    output logic [N_DATA_BITS-1 : 0] rdata0,

    input  logic clk1,
    input  logic [$clog2(N_ENTRIES)-1 : 0] addr1,
    input  logic wen1,
    input  tri1  [(N_DATA_BITS / N_BYTE_BITS)-1 : 0] byteena1,
    input  logic [N_DATA_BITS-1 : 0] wdata1,
    output logic [N_DATA_BITS-1 : 0] rdata1
    );

    logic [N_DATA_BITS-1 : 0] mem_rd0[0 : N_OUTPUT_REG_STAGES];
    assign rdata0 = mem_rd0[N_OUTPUT_REG_STAGES];

    logic [N_DATA_BITS-1 : 0] mem_rd1[0 : N_OUTPUT_REG_STAGES];
    assign rdata1 = mem_rd1[N_OUTPUT_REG_STAGES];

    // If the output data is registered then request a register stage in
    // the megafunction, giving it an opportunity to optimize the location.
    //
    localparam OUTDATA_REGISTERED0 = (N_OUTPUT_REG_STAGES == 0) ? "UNREGISTERED" :
                                                                  "CLOCK0";
    localparam OUTDATA_REGISTERED1 = (N_OUTPUT_REG_STAGES == 0) ? "UNREGISTERED" :
                                                                  "CLOCK1";
    localparam OUTDATA_IDX = (N_OUTPUT_REG_STAGES == 0) ? 0 : 1;


    altsyncram
      #(
        .operation_mode(OPERATION_MODE),
        .byte_size(N_BYTE_BITS),
        .width_a(N_DATA_BITS),
        .width_byteena_a(N_DATA_BITS / N_BYTE_BITS),
        .widthad_a($clog2(N_ENTRIES)),
        .numwords_a(N_ENTRIES),
        .width_b(N_DATA_BITS),
        .width_byteena_b(N_DATA_BITS / N_BYTE_BITS),
        .widthad_b($clog2(N_ENTRIES)),
        .numwords_b(N_ENTRIES),
        .outdata_reg_a(OUTDATA_REGISTERED0),
        .rdcontrol_reg_b("CLOCK1"),
        .address_reg_b("CLOCK1"),
        .outdata_reg_b(OUTDATA_REGISTERED1),
        .read_during_write_mode_mixed_ports(READ_DURING_WRITE_MODE_MIXED_PORTS),
        .read_during_write_mode_port_a(READ_DURING_WRITE_MODE_PORT_A),
        .read_during_write_mode_port_b(READ_DURING_WRITE_MODE_PORT_B)
        )
      data
       (
        .clock0(clk0),
        .clock1(clk1),

        .wren_a(wen0),
        .byteena_a(byteena0),
        .address_a(addr0),
        .data_a(wdata0),
        .q_a(mem_rd0[OUTDATA_IDX]),

        .wren_b(wen1),
        .byteena_b(byteena1),
        .address_b(addr1),
        .data_b(wdata1),
        .q_b(mem_rd1[OUTDATA_IDX]),

        // Legally unconnected ports -- get rid of lint errors
        .rden_a(),
        .rden_b(),
        .clocken0(),
        .clocken1(),
        .clocken2(),
        .clocken3(),
        .aclr0(),
        .aclr1(),
        .addressstall_a(),
        .addressstall_b(),
        .eccstatus()
        );


    genvar s;
    generate
        for (s = 1; s < N_OUTPUT_REG_STAGES; s = s + 1)
        begin: r
            always_ff @(posedge clk0)
            begin
                mem_rd0[s+1] <= mem_rd0[s];
            end

            always_ff @(posedge clk1)
            begin
                mem_rd1[s+1] <= mem_rd1[s];
            end
        end
    endgenerate

endmodule // cci_mpf_prim_ram_dualport_byteena


//
// Dual port byte masked RAM initialized with a constant on reset.
//
module cci_mpf_prim_ram_dualport_byteena_init
  #(
    parameter N_ENTRIES = 32,
    parameter N_DATA_BITS = 64,
    // Number of extra stages of output register buffering to add
    parameter N_OUTPUT_REG_STAGES = 0,

    // Size of a byte
    parameter N_BYTE_BITS = 8,

    // Other options are "OLD_DATA" and "NEW_DATA"
    parameter READ_DURING_WRITE_MODE_MIXED_PORTS = "DONT_CARE",

    // Default returns new data for reads on same port as a write.
    // (No NBE read means return X in masked bytes.)
    // Set to OLD_DATA to return the current value.
    parameter READ_DURING_WRITE_MODE_PORT_A = "NEW_DATA_NO_NBE_READ",
    parameter READ_DURING_WRITE_MODE_PORT_B = "NEW_DATA_NO_NBE_READ",

    parameter INIT_VALUE = N_DATA_BITS'(0)
    )
   (
    input  logic reset,
    // Goes high after initialization complete and stays high.
    output logic rdy,

    input  logic clk0,
    input  logic [$clog2(N_ENTRIES)-1 : 0] addr0,
    input  logic wen0,
    input  tri1  [(N_DATA_BITS / N_BYTE_BITS)-1 : 0] byteena0,
    input  logic [N_DATA_BITS-1 : 0] wdata0,
    output logic [N_DATA_BITS-1 : 0] rdata0,

    input  logic clk1,
    input  logic [$clog2(N_ENTRIES)-1 : 0] addr1,
    input  logic wen1,
    input  tri1  [(N_DATA_BITS / N_BYTE_BITS)-1 : 0] byteena1,
    input  logic [N_DATA_BITS-1 : 0] wdata1,
    output logic [N_DATA_BITS-1 : 0] rdata1
    );

    logic wen0_local;

    logic [$clog2(N_ENTRIES)-1 : 0] addr1_local;
    logic wen1_local;
    logic [N_DATA_BITS-1 : 0] wdata1_local;
    logic [(N_DATA_BITS / N_BYTE_BITS)-1 : 0] byteena1_local;

    cci_mpf_prim_ram_dualport_byteena
      #(
        .N_ENTRIES(N_ENTRIES),
        .N_DATA_BITS(N_DATA_BITS),
        .N_OUTPUT_REG_STAGES(N_OUTPUT_REG_STAGES),
        .N_BYTE_BITS(N_BYTE_BITS),
        .READ_DURING_WRITE_MODE_MIXED_PORTS(READ_DURING_WRITE_MODE_MIXED_PORTS),
        .READ_DURING_WRITE_MODE_PORT_A(READ_DURING_WRITE_MODE_PORT_A),
        .READ_DURING_WRITE_MODE_PORT_B(READ_DURING_WRITE_MODE_PORT_B)
        )
      ram
       (
        .clk0,
        .addr0,
        .byteena0,
        .wen0,
        .wdata0,
        .rdata0,

        .clk1,
        .addr1(addr1_local),
        .byteena1(byteena1_local),
        .wen1(wen1_local),
        .wdata1(wdata1_local),
        .rdata1
        );


    //
    // Initialization loop
    //

    logic [$clog2(N_ENTRIES)-1 : 0] addr1_init;

    assign addr1_local = rdy ? addr1 : addr1_init;
    assign byteena1_local = rdy ? byteena1 : ~(($bits(byteena1))'(0));
    assign wen1_local = rdy ? wen1 : 1'b1;
    assign wdata1_local = rdy ? wdata1 : INIT_VALUE;

    always_ff @(posedge clk1)
    begin
        if (reset)
        begin
            rdy <= 1'b0;
            addr1_init <= 0;
        end
        else if (! rdy)
        begin
            addr1_init <= addr1_init + 1;
            rdy <= (addr1_init == (N_ENTRIES-1));
        end
    end

endmodule // cci_mpf_prim_ram_dualport_byteena_init
