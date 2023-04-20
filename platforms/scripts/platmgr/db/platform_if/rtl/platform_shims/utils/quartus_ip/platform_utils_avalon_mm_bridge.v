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

// --------------------------------------
// Avalon-MM pipeline bridge
//
// Optionally registers Avalon-MM command and response signals
// --------------------------------------

`timescale 1 ns / 1 ns
module platform_utils_avalon_mm_bridge 
#(
    parameter DATA_WIDTH           = 32,
    parameter SYMBOL_WIDTH         = 8,
    parameter RESPONSE_WIDTH       = 2,
    parameter HDL_ADDR_WIDTH       = 10,
    parameter BURSTCOUNT_WIDTH     = 1,

    parameter PIPELINE_COMMAND     = 1,
    parameter PIPELINE_RESPONSE    = 1,
    parameter SYNC_RESET           = 1,

    // --------------------------------------
    // Derived parameters
    // --------------------------------------
    parameter BYTEEN_WIDTH = DATA_WIDTH / SYMBOL_WIDTH
)
(
    input                         clk,
    input                         reset,

    output                        s0_waitrequest,
    output [DATA_WIDTH-1:0]       s0_readdata,
    output                        s0_readdatavalid,
    output [RESPONSE_WIDTH-1:0]   s0_response,
    input  [BURSTCOUNT_WIDTH-1:0] s0_burstcount,
    input  [DATA_WIDTH-1:0]       s0_writedata,
    input  [HDL_ADDR_WIDTH-1:0]   s0_address, 
    input                         s0_write, 
    input                         s0_read, 
    input  [BYTEEN_WIDTH-1:0]     s0_byteenable, 
    input                         s0_debugaccess,

    input                         m0_waitrequest,
    input  [DATA_WIDTH-1:0]       m0_readdata,
    input                         m0_readdatavalid,
    input  [RESPONSE_WIDTH-1:0]   m0_response,
    output [BURSTCOUNT_WIDTH-1:0] m0_burstcount,
    output [DATA_WIDTH-1:0]       m0_writedata,
    output [HDL_ADDR_WIDTH-1:0]   m0_address, 
    output                        m0_write, 
    output                        m0_read, 
    output [BYTEEN_WIDTH-1:0]     m0_byteenable,
    output                        m0_debugaccess
);
    // --------------------------------------
    // Registers & signals
    // --------------------------------------
    reg [BURSTCOUNT_WIDTH-1:0]   cmd_burstcount;
    reg [DATA_WIDTH-1:0]         cmd_writedata;
    reg [HDL_ADDR_WIDTH-1:0]     cmd_address; 
    reg                          cmd_write;  
    reg                          cmd_read;  
    reg [BYTEEN_WIDTH-1:0]       cmd_byteenable;
    wire                         cmd_waitrequest;
    reg                          cmd_debugaccess;

    reg [BURSTCOUNT_WIDTH-1:0]   wr_burstcount;
    reg [DATA_WIDTH-1:0]         wr_writedata;
    reg [HDL_ADDR_WIDTH-1:0]     wr_address; 
    reg                          wr_write;  
    reg                          wr_read;  
    reg [BYTEEN_WIDTH-1:0]       wr_byteenable;
    reg                          wr_debugaccess;

    reg [BURSTCOUNT_WIDTH-1:0]   wr_reg_burstcount;
    reg [DATA_WIDTH-1:0]         wr_reg_writedata;
    reg [HDL_ADDR_WIDTH-1:0]     wr_reg_address; 
    reg                          wr_reg_write;  
    reg                          wr_reg_read;  
    reg [BYTEEN_WIDTH-1:0]       wr_reg_byteenable;
    reg                          wr_reg_waitrequest;
    reg                          wr_reg_debugaccess;

    reg                          use_reg;
    wire                         wait_rise;

    reg [DATA_WIDTH-1:0]         rsp_readdata;
    reg                          rsp_readdatavalid;
    reg [RESPONSE_WIDTH-1:0]     rsp_response;   


   // generating sync reset 
    reg internal_sclr;
    generate if (SYNC_RESET == 1) begin // rst_syncronizer
       always @ (posedge clk) begin
          internal_sclr <= reset;
       end
    end
    endgenerate

    // --------------------------------------
    // Command pipeline
    //
    // Registers all command signals, including waitrequest
    // --------------------------------------
    generate if (PIPELINE_COMMAND == 1) begin

        // --------------------------------------
        // Waitrequest Pipeline Stage
        //
        // Output waitrequest is delayed by one cycle, which means
        // that a master will see waitrequest assertions one cycle 
        // too late.
        //
        // Solution: buffer the command when waitrequest transitions
        // from low->high. As an optimization, we can safely assume 
        // waitrequest is low by default because downstream logic
        // in the bridge ensures this.
        //
        // Note: this implementation buffers idle cycles should 
        // waitrequest transition on such cycles. This is a potential
        // cause for throughput loss, but ye olde pipeline bridge did
        // the same for years and no one complained. Not buffering idle
        // cycles costs logic on the waitrequest path.
        // --------------------------------------
        assign s0_waitrequest = wr_reg_waitrequest;
        assign wait_rise      = ~wr_reg_waitrequest & cmd_waitrequest;
   
        always @(posedge clk) begin
         if (wait_rise) begin
         wr_reg_writedata  <= s0_writedata;
         wr_reg_byteenable <= s0_byteenable;
         wr_reg_address    <= s0_address;
         end
        end
      
        if (SYNC_RESET == 0) begin // async_reg0 

            always @(posedge clk, posedge reset) begin
                if (reset) begin
                    wr_reg_waitrequest <= 1'b1;
                    // --------------------------------------
                    // Bit of trickiness here, deserving of a long comment.
                    //
                    // On the first cycle after reset, the pass-through
                    // must not be used or downstream logic may sample
                    // the same command twice because of the delay in
                    // transmitting a falling waitrequest.
                    //
                    // Using the registered command works on the condition
                    // that downstream logic deasserts waitrequest
                    // immediately after reset, which is true of the 
                    // next stage in this bridge.
                    // --------------------------------------
                    use_reg            <= 1'b1;
                    wr_reg_burstcount <= 1'b1;
                    wr_reg_write      <= 1'b0;
                    wr_reg_read       <= 1'b0;
                    wr_reg_debugaccess <= 1'b0;
                end else begin
                    wr_reg_waitrequest <= cmd_waitrequest;

                    if (wait_rise) begin
                        wr_reg_write      <= s0_write;
                        wr_reg_read       <= s0_read;
                        wr_reg_burstcount <= s0_burstcount;
                        wr_reg_debugaccess <= s0_debugaccess;
                    end

                    // stop using the buffer when waitrequest is low
                    if (~cmd_waitrequest)
                         use_reg <= 1'b0;
                    else if (wait_rise) begin
                        use_reg <= 1'b1;
                    end     

                end
            end
        end else begin // end aysnc_reg0
        // sync_reset     
            always @(posedge clk) begin
                  if (internal_sclr) begin
                      wr_reg_waitrequest <= 1'b1;
                      // --------------------------------------
                      // Bit of trickiness here, deserving of a long comment.
                      //
                      // On the first cycle after reset, the pass-through
                      // must not be used or downstream logic may sample
                      // the same command twice because of the delay in
                      // transmitting a falling waitrequest.
                      //
                      // Using the registered command works on the condition
                      // that downstream logic deasserts waitrequest
                      // immediately after reset, which is true of the 
                      // next stage in this bridge.
                      // --------------------------------------
                      use_reg            <= 1'b1;

                      wr_reg_burstcount <= 1'b1;
                      wr_reg_write      <= 1'b0;
                      wr_reg_read       <= 1'b0;
                      wr_reg_debugaccess <= 1'b0;
                  end else begin
                      wr_reg_waitrequest <= cmd_waitrequest;

                      if (wait_rise) begin
                          wr_reg_write      <= s0_write;
                          wr_reg_read       <= s0_read;
                          wr_reg_burstcount <= s0_burstcount;
                          wr_reg_debugaccess <= s0_debugaccess;
                      end

                      // stop using the buffer when waitrequest is low
                      if (~cmd_waitrequest)
                           use_reg <= 1'b0;
                      else if (wait_rise) begin
                          use_reg <= 1'b1;
                      end     

                  end
            end
        
        end // if sync_reset

        always @* begin
            wr_burstcount  =  s0_burstcount;
            wr_writedata   =  s0_writedata;
            wr_address     =  s0_address;
            wr_write       =  s0_write;
            wr_read        =  s0_read;
            wr_byteenable  =  s0_byteenable;
            wr_debugaccess =  s0_debugaccess;
     
            if (use_reg) begin
                wr_burstcount  =  wr_reg_burstcount;
                wr_writedata   =  wr_reg_writedata;
                wr_address     =  wr_reg_address;
                wr_write       =  wr_reg_write;
                wr_read        =  wr_reg_read;
                wr_byteenable  =  wr_reg_byteenable;
                wr_debugaccess =  wr_reg_debugaccess;
            end
        end
     
        // --------------------------------------
        // Master-Slave Signal Pipeline Stage 
        //
        // One notable detail is that cmd_waitrequest is deasserted
        // when this stage is idle. This allows us to make logic
        // optimizations in the waitrequest pipeline stage.
        // 
        // Also note that cmd_waitrequest is deasserted during reset,
        // which is not spec-compliant, but is ok for an internal
        // signal.
        // --------------------------------------
        wire no_command;
        assign no_command      = ~(cmd_read || cmd_write);
        assign cmd_waitrequest = m0_waitrequest & ~no_command;

        always @(posedge clk) begin
         if (~cmd_waitrequest) begin
         cmd_writedata  <= wr_writedata;
         cmd_byteenable <= wr_byteenable;
         cmd_address    <= wr_address;
         end
        end
 
        if (SYNC_RESET == 0) begin // async_reg1
       
          always @(posedge clk, posedge reset) begin
              if (reset) begin
                  cmd_burstcount <= 1'b1;
                  cmd_write      <= 1'b0;
                  cmd_read       <= 1'b0;
                  cmd_debugaccess <= 1'b0;
              end 
              else begin 
                  if (~cmd_waitrequest) begin
                      cmd_write      <= wr_write;
                      cmd_read       <= wr_read;
                      cmd_burstcount <= wr_burstcount;
                      cmd_debugaccess <= wr_debugaccess;
                  end
              end
          end

        end else begin // aysnc_reg1

          always @(posedge clk) begin //sync_reg1
              if (internal_sclr) begin
                  cmd_burstcount <= 1'b1;
                  cmd_write      <= 1'b0;
                  cmd_read       <= 1'b0;
                  cmd_debugaccess <= 1'b0;
              end 
              else begin 
                  if (~cmd_waitrequest) begin
                      cmd_write      <= wr_write;
                      cmd_read       <= wr_read;
                      cmd_burstcount <= wr_burstcount;
                      cmd_debugaccess <= wr_debugaccess;
                  end
              end
          end 
      end // sync_reg1
    end  // conditional command pipeline
    else begin

        assign s0_waitrequest   = m0_waitrequest;

        always @* begin
            cmd_burstcount   = s0_burstcount;
            cmd_writedata    = s0_writedata;
            cmd_address      = s0_address;
            cmd_write        = s0_write;
            cmd_read         = s0_read;
            cmd_byteenable   = s0_byteenable;
            cmd_debugaccess  = s0_debugaccess;
        end

    end
    endgenerate

    assign m0_burstcount    = cmd_burstcount;
    assign m0_writedata     = cmd_writedata;
    assign m0_address       = cmd_address;
    assign m0_write         = cmd_write;
    assign m0_read          = cmd_read;
    assign m0_byteenable    = cmd_byteenable;
    assign m0_debugaccess   = cmd_debugaccess;

    // --------------------------------------
    // Response pipeline
    //
    // Registers all response signals
    // --------------------------------------
    generate if (PIPELINE_RESPONSE == 1) begin

       always @(posedge clk) begin
       rsp_readdata      <= m0_readdata;
       end

       if (SYNC_RESET == 0) begin // async_reg2
        always @(posedge clk, posedge reset) begin
            if (reset) begin
                rsp_readdatavalid <= 1'b0;
                rsp_response      <= 0;               
            end 
            else begin
                rsp_readdatavalid <= m0_readdatavalid;
                rsp_response      <= m0_response;               
            end
        end
       end //async_reg2
       else begin // sync reg2
        always @(posedge clk) begin
            if (internal_sclr) begin
                rsp_readdatavalid <= 1'b0;
                rsp_response      <= 0;               
            end 
            else begin
                rsp_readdatavalid <= m0_readdatavalid;
                rsp_response      <= m0_response;               
            end
        end  
       end // end  sync_reg2 
         
    end  // conditional response pipeline
    
    else begin

        always @* begin
            rsp_readdatavalid = m0_readdatavalid;
            rsp_readdata      = m0_readdata;
            rsp_response      = m0_response;           
        end
    end
    endgenerate

    assign s0_readdatavalid = rsp_readdatavalid;
    assign s0_readdata      = rsp_readdata;
    assign s0_response      = rsp_response;   

endmodule
