/* ****************************************************************************
 * Copyright(c) 2011-2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * **************************************************************************
 *
 * Module Name: ase_svfifo
 *              A systemverilog built-in compatible SVFIFO, that is
 *              potentially faster
 * Language   : System{Verilog}
 * Owner      : Rahul R Sharma
 *              rahul.r.sharma@intel.com
 *              Intel Corporation
 *
 * FIFO implementation for use in ASE only
 * Generics:
 * - DEPTH_BASE2    : Radix of element array, used for counting elements
 * - ALMFULL_THRESH : AlmostFull threshold
 *
 * Description:
 * - WRITE: Data is written when write_enable is HIGH
 * - READ : Data is available in next clock it is in RAM
 *          Empty is an ASYNC signal & must be check
 *          read_enable must be used asynchronously with EMPTY signal
 * - Overflow and underflow signals are asserted
 *
 */

module ase_svfifo
  #(
    parameter int DATA_WIDTH = 64,
    parameter int DEPTH_BASE2 = 8,
    parameter int ALMFULL_THRESH = 5
    )
   (
    input logic                     clk,
    input logic                     rst,
    input logic                     wr_en,
    input logic [DATA_WIDTH-1:0]    data_in,
    input logic                     rd_en,
    output logic [DATA_WIDTH-1:0]   data_out,
    output logic                    data_out_v,
    output logic                    alm_full,
    output logic                    full,
    output logic                    empty,
    output logic [DEPTH_BASE2:0]    count,
    output logic                    overflow,
    output logic                    underflow
    );

    // Calculate depth
    parameter int                   DEPTH = 2**DEPTH_BASE2;

    // FIFO instance
    logic [DATA_WIDTH-1:0]          fifo[$:DEPTH-1];

    always @(posedge clk) begin
        if (wr_en) begin
            fifo.push_back(data_in);
        end
    end

    // Empty signal
    always @(posedge clk) begin
        if (fifo.size() == 0) begin
            empty <= 1;
        end
        else begin
            empty <= 0;
        end
    end

    // Full signal
    always @(posedge clk) begin
        if (fifo.size() == (DEPTH-1)) begin
            full <= 1;
        end
        else begin
           full <= 0;
        end
    end

    // Almfull signal
    always @(posedge clk) begin
        if (fifo.size() >= (DEPTH - ALMFULL_THRESH)) begin
            alm_full <= 1;
        end
        else begin
            alm_full <= 0;
        end
    end

    // Read process
    always @(posedge clk) begin
        if (rst) begin
            data_out_v <= 0;
        end
        else if (rd_en && (fifo.size() != 0)) begin
            data_out_v <= 1;
            data_out <= fifo.pop_front();
        end
        else begin
            data_out_v <= 0;
        end
    end

    // Overflow
    assign overflow = full & wr_en;

    // Underflow
    assign underflow = empty & rd_en;

    // Count
    always @(posedge clk) begin
        count <= fifo.size();
    end

endmodule
