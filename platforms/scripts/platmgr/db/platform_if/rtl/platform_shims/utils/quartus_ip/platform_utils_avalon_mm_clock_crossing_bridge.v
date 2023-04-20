//
// Copyright (c) 2018, Intel Corporation
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


// $Id: //acds/rel/18.0/ip/merlin/altera_avalon_mm_clock_crossing_bridge/altera_avalon_mm_clock_crossing_bridge.v#2 $
// $Revision: #2 $
// $Date: 2018/02/20 $
// $Author: ashwinya $
// --------------------------------------
// Avalon-MM clock crossing bridge
//
// Clock crosses MM commands and responses with the
// help of asynchronous FIFOs.
//
// This bridge will stop emitting read commands when
// too many read commands are in flight to avoid 
// response FIFO overflow.
// --------------------------------------

`timescale 1 ns / 1 ns
module platform_utils_avalon_mm_clock_crossing_bridge
#(
    parameter DATA_WIDTH            = 32,
    parameter SYMBOL_WIDTH          = 8,
    parameter HDL_ADDR_WIDTH        = 10,
    parameter BURSTCOUNT_WIDTH      = 1,

    parameter COMMAND_FIFO_DEPTH    = 4,
    parameter RESPONSE_FIFO_DEPTH   = 4,

    parameter MASTER_SYNC_DEPTH     = 2,
    parameter SLAVE_SYNC_DEPTH      = 2,
    parameter SYNC_RESET            = 1,

    // --------------------------------------
    // Derived parameters
    // --------------------------------------
    parameter BYTEEN_WIDTH = DATA_WIDTH / SYMBOL_WIDTH,
    parameter COMMAND_COUNT_WIDTH = log2ceil(COMMAND_FIFO_DEPTH)
)
(
    input                           s0_clk,
    input                           s0_reset,

    input                           m0_clk,
    input                           m0_reset,

    output                          s0_waitrequest,
    output [DATA_WIDTH-1:0]         s0_readdata,
    output                          s0_readdatavalid,
    input  [BURSTCOUNT_WIDTH-1:0]   s0_burstcount,
    input  [DATA_WIDTH-1:0]         s0_writedata,
    input  [HDL_ADDR_WIDTH-1:0]     s0_address, 
    input                           s0_write,  
    input                           s0_read,  
    input  [BYTEEN_WIDTH-1:0]       s0_byteenable,  
    input                           s0_debugaccess,
    // Added for OPAE to the base bridge implementation in order to build
    // almost full protocols, using the command FIFO as a buffer.
    output [COMMAND_COUNT_WIDTH:0]  s0_space_avail_data,

    input                           m0_waitrequest,
    input  [DATA_WIDTH-1:0]         m0_readdata,
    input                           m0_readdatavalid,
    output [BURSTCOUNT_WIDTH-1:0]   m0_burstcount,
    output [DATA_WIDTH-1:0]         m0_writedata,
    output [HDL_ADDR_WIDTH-1:0]     m0_address, 
    output                          m0_write,  
    output                          m0_read,  
    output [BYTEEN_WIDTH-1:0]       m0_byteenable,
    output                          m0_debugaccess
);

    localparam CMD_WIDTH = BURSTCOUNT_WIDTH + DATA_WIDTH + HDL_ADDR_WIDTH 
                    + BYTEEN_WIDTH 
                    + 3;        // read, write, debugaccess

    localparam NUMSYMBOLS    = DATA_WIDTH / SYMBOL_WIDTH;
    localparam RSP_WIDTH     = DATA_WIDTH;
    localparam MAX_BURST     = (1 << (BURSTCOUNT_WIDTH-1));
    localparam COUNTER_WIDTH = log2ceil(RESPONSE_FIFO_DEPTH) + 1;
    localparam NON_BURSTING  = (MAX_BURST == 1);
    localparam BURST_WORDS_W = BURSTCOUNT_WIDTH;

    // --------------------------------------
    // Signals
    // --------------------------------------
    wire [CMD_WIDTH-1:0]     s0_cmd_payload;
    wire [CMD_WIDTH-1:0]     m0_cmd_payload;
    wire                     s0_cmd_valid;
    wire                     m0_cmd_valid;
    wire                     m0_internal_write;
    wire                     m0_internal_read;
    wire                     s0_cmd_ready;
    wire                     m0_cmd_ready;
    reg  [COUNTER_WIDTH-1:0] pending_read_count;
    wire [COUNTER_WIDTH-1:0] space_avail;
    wire                     stop_cmd;
    reg                      stop_cmd_r;
    wire                     m0_read_accepted;
    wire                     m0_rsp_ready;
    reg                      old_read;
    wire [BURST_WORDS_W-1:0] m0_burstcount_words;

    // --------------------------------------
    // Command FIFO
    // --------------------------------------
    (* altera_attribute = "-name ALLOW_ANY_RAM_SIZE_FOR_RECOGNITION ON" *) platform_utils_avalon_dc_fifo
    #(
        .SYMBOLS_PER_BEAT (1),
        .BITS_PER_SYMBOL  (CMD_WIDTH),
        .FIFO_DEPTH       (COMMAND_FIFO_DEPTH),
        .WR_SYNC_DEPTH    (MASTER_SYNC_DEPTH),
        .RD_SYNC_DEPTH    (SLAVE_SYNC_DEPTH),
        .BACKPRESSURE_DURING_RESET (1),
        // Added for OPAE to drive s0_space_avail_data
        .USE_SPACE_AVAIL_IF (1)
    ) 
    cmd_fifo
    (
        .in_clk          (s0_clk),
        .in_reset_n      (~s0_reset),
        .out_clk         (m0_clk),
        .out_reset_n     (~m0_reset),

        .in_data         (s0_cmd_payload),
        .in_valid        (s0_cmd_valid),
        .in_ready        (s0_cmd_ready),

        .out_data        (m0_cmd_payload),
        .out_valid       (m0_cmd_valid),
        .out_ready       (m0_cmd_ready),

        .in_startofpacket   (1'b0),
        .in_endofpacket     (1'b0),
        .in_empty           (1'b0),
        .in_error           (1'b0),
        .in_channel         (1'b0),
        .in_csr_address     (1'b0),
        .in_csr_read        (1'b0),
        .in_csr_write       (1'b0),
        .in_csr_writedata   (32'b0),
        .out_csr_address    (1'b0),
        .out_csr_read       (1'b0),
        .out_csr_write      (1'b0),
        .out_csr_writedata  (32'b0),

        .out_startofpacket(),
        .out_endofpacket(),
        .out_empty(),
        .out_error(),
        .out_channel(),
        .in_csr_readdata(),
        .out_csr_readdata(),
        .almost_full_valid(),
        .almost_full_data(),
        .almost_empty_valid(),
        .almost_empty_data(),
        .space_avail_data(s0_space_avail_data)
        
    );


    // Generation of internal reset synchronization
   reg internal_sclr;
   generate if (SYNC_RESET == 1) begin // rst_syncronizer
      always @ (posedge m0_clk) begin
         internal_sclr <= m0_reset;
      end
   end
   endgenerate

    // --------------------------------------
    // Command payload
    // --------------------------------------
    assign s0_waitrequest = ~s0_cmd_ready;
    assign s0_cmd_valid   = s0_write | s0_read;

    assign s0_cmd_payload = {s0_address, 
                             s0_burstcount, 
                             s0_read, 
                             s0_write, 
                             s0_writedata, 
                             s0_byteenable,
                             s0_debugaccess};
    assign {m0_address, 
            m0_burstcount, 
            m0_internal_read, 
            m0_internal_write,
            m0_writedata, 
            m0_byteenable,
            m0_debugaccess} = m0_cmd_payload;

    assign m0_cmd_ready = ~m0_waitrequest & 
                            ~(m0_internal_read & stop_cmd_r & ~old_read);
    assign m0_write =  m0_internal_write & m0_cmd_valid;
    assign m0_read  =  m0_internal_read & m0_cmd_valid & (~stop_cmd_r | old_read);
    assign m0_read_accepted = m0_read & ~m0_waitrequest;


    // ---------------------------------------------
    // the non-bursting case
    // ---------------------------------------------
    generate if (NON_BURSTING)
    begin
      if (SYNC_RESET == 0) begin  
    
           always @(posedge m0_clk, posedge m0_reset) begin
               if (m0_reset) begin
                   pending_read_count <= 0;
               end
               else begin
                   if (m0_read_accepted & m0_readdatavalid)
                       pending_read_count <= pending_read_count;
                   else if (m0_readdatavalid)
                       pending_read_count <= pending_read_count - 1'd1;
                   else if (m0_read_accepted)
                       pending_read_count <= pending_read_count + 1'd1;
               end
           end
      end // async_rst0

      else begin 
           always @(posedge m0_clk) begin
               if (internal_sclr) begin
                   pending_read_count <= 0;
               end
               else begin
                   if (m0_read_accepted & m0_readdatavalid)
                       pending_read_count <= pending_read_count;
                   else if (m0_readdatavalid)
                       pending_read_count <= pending_read_count - 1'd1;
                   else if (m0_read_accepted)
                       pending_read_count <= pending_read_count + 1'd1;
               end
           end
      end // sync_rst0
    end // if non_bursting

    // ---------------------------------------------
    // the bursting case
    // ---------------------------------------------
    else begin
        assign m0_burstcount_words = m0_burstcount;
      if (SYNC_RESET == 0 ) begin 
           always @(posedge m0_clk, posedge m0_reset) begin
               if (m0_reset) begin
                   pending_read_count <= 0;
               end
               else begin
                   if (m0_read_accepted & m0_readdatavalid)
                       pending_read_count <= pending_read_count +
                                               m0_burstcount_words - 1'd1;
                   else if (m0_readdatavalid)
                       pending_read_count <= pending_read_count - 1'd1;
                   else if (m0_read_accepted)
                       pending_read_count <= pending_read_count +
                                               m0_burstcount_words;  
               end
           end
      end // async_rst1
      else begin 
           always @(posedge m0_clk) begin
               if (internal_sclr) begin
                   pending_read_count <= 0;
               end
               else begin
                   if (m0_read_accepted & m0_readdatavalid)
                       pending_read_count <= pending_read_count +
                                               m0_burstcount_words - 1'd1;
                   else if (m0_readdatavalid)
                       pending_read_count <= pending_read_count - 1'd1;
                   else if (m0_read_accepted)
                       pending_read_count <= pending_read_count +
                                               m0_burstcount_words;  
               end
           end // @always
      end // sync_rst1


    end // else bursting
    endgenerate

    assign stop_cmd = (pending_read_count + 2*MAX_BURST) > space_avail;
   
    generate
    if (SYNC_RESET == 0) begin // async_rst2
       always @(posedge m0_clk, posedge m0_reset) begin
           if (m0_reset) begin
               stop_cmd_r <= 1'b0;
               old_read   <= 1'b0;
           end
           else begin
               stop_cmd_r <= stop_cmd;
               old_read   <= m0_read & m0_waitrequest;
           end
       end
     end // async_rst2

     else begin
       always @(posedge m0_clk) begin
           if (internal_sclr) begin
               stop_cmd_r <= 1'b0;
               old_read   <= 1'b0;
           end
           else begin
               stop_cmd_r <= stop_cmd;
               old_read   <= m0_read & m0_waitrequest;
           end
       end
     end // sync_rst2
     endgenerate
    // --------------------------------------
    // Response FIFO
    // --------------------------------------
    (* altera_attribute = "-name ALLOW_ANY_RAM_SIZE_FOR_RECOGNITION ON" *) platform_utils_avalon_dc_fifo
    #(
        .SYMBOLS_PER_BEAT   (1),
        .BITS_PER_SYMBOL    (RSP_WIDTH),
        .FIFO_DEPTH         (RESPONSE_FIFO_DEPTH),
        .WR_SYNC_DEPTH      (SLAVE_SYNC_DEPTH),
        .RD_SYNC_DEPTH      (MASTER_SYNC_DEPTH),
        .USE_SPACE_AVAIL_IF (1)
    ) 
    rsp_fifo
    (
        .in_clk           (m0_clk),
        .in_reset_n       (~m0_reset),
        .out_clk          (s0_clk),
        .out_reset_n      (~s0_reset),

        .in_data          (m0_readdata),
        .in_valid         (m0_readdatavalid),

        // ------------------------------------
        // must never overflow, or we're in trouble
        // (we cannot backpressure the response)
        // ------------------------------------
        .in_ready         (m0_rsp_ready),

        .out_data         (s0_readdata),
        .out_valid        (s0_readdatavalid),
        .out_ready        (1'b1),

        .space_avail_data (space_avail),

        .in_startofpacket   (1'b0),
        .in_endofpacket     (1'b0),
        .in_empty           (1'b0),
        .in_error           (1'b0),
        .in_channel         (1'b0),
        .in_csr_address     (1'b0),
        .in_csr_read        (1'b0),
        .in_csr_write       (1'b0),
        .in_csr_writedata   (32'b0),
        .out_csr_address    (1'b0),
        .out_csr_read       (1'b0),
        .out_csr_write      (1'b0),
        .out_csr_writedata  (32'b0),

        .out_startofpacket(),
        .out_endofpacket(),
        .out_empty(),
        .out_error(),
        .out_channel(),
        .in_csr_readdata(),
        .out_csr_readdata(),
        .almost_full_valid(),
        .almost_full_data(),
        .almost_empty_valid(),
        .almost_empty_data()
    );

// synthesis translate_off
    always @(posedge m0_clk) begin
        if (~m0_rsp_ready & m0_readdatavalid) begin
            $display("%t %m: internal error, response fifo overflow", $time);
        end

        if (pending_read_count > space_avail) begin
            $display("%t %m: internal error, too many pending reads", $time);
        end
    end
// synthesis translate_on

    // --------------------------------------------------
    // Calculates the log2ceil of the input value
    // --------------------------------------------------
    function integer log2ceil;
        input integer val;
        integer i;

        begin
            i = 1;
            log2ceil = 0;

            while (i < val) begin
                log2ceil = log2ceil + 1;
                i = i << 1; 
            end
        end
    endfunction

endmodule  
