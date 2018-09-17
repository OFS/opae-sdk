// (C) 2001-2018 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other 
// software and tools, and its AMPP partner logic functions, and any output 
// files from any of the foregoing (including device programming or simulation 
// files), and any associated documentation or information are expressly subject 
// to the terms and conditions of the Intel Program License Subscription 
// Agreement, Intel FPGA IP License Agreement, or other applicable 
// license agreement, including, without limitation, that your use is for the 
// sole purpose of programming logic devices manufactured by Intel and sold by 
// Intel or its authorized distributors.  Please refer to the applicable 
// agreement for further details.


/*

Custom pattern generator core

Author:  JCJB
Date:    04/14/2010

Version:  2.1

Description:  This component is programmed via a host or master through the pattern slave port to program the internal test memory.  When the host
              writes to the start bit of the CSR slave port the component will begin to send the contents from the internal memory to the data streaming
              port.  You should use the custom pattern checker component with this component.

Register map:

|-------------------------------------------------------------------------------------------------------------------------------------------------------|
|  Address   |   Access Type   |                                                            Bits                                                        |
|            |                 |------------------------------------------------------------------------------------------------------------------------|
|            |                 |            31..24            |            23..16            |            15..8            |            7..0            |
|-------------------------------------------------------------------------------------------------------------------------------------------------------|
|     0      |       r/w       |                                                       Payload Length                                                   |
|     4      |       r/w       |                        Pattern Position                                             Pattern Length                     |
|     8      |       r/w       |                                                          Control                                                       |
|     12     |       N/A       |                                                          Status                                                        |
|-------------------------------------------------------------------------------------------------------------------------------------------------------|


Address 0  --> Bits 31:0 are used store the payload length as well as read it back while the generator core is operating.  This field should only be written to while
               the generator is stopped.
Address 4  --> Bits 15:0 is used to store pattern length, bits 31:16 used to store a new position in the pattern.  The position will update as the generator is operating.
               These fields should only be written to while the generator core is stopped.
Address 8  --> Bit 0 (infinite length test enable)is used to instruct the generator to ignore the payload length field and generate the pattern until stopped.
               Bit 1 (generation complete IRQ mask) is the generation complete interrupt mask bit.  This bit is only applicable to length bound pattern generation.
               Bit 2 (sop generation) when set, the generator will output SOP on the first beat of the pattern.
               Bit 3 (eop generation) when set, the generator will output EOP on the last beat of the pattern.  EOP will also be generated if the generator is stopped.
               Bits 15:8 (last transfer size) specifies the number of symbols (bytes) are valid on the last beat.  This value must be set between 1 and DATA_WIDTH/8.
               Bit 31 (generator enable) is used to start the generator core so that it begins receiving data.  This field must be set at the same time or after all the
                      other fields bits are programmed.  This field must be cleared before modifying other fields.  This field is cleared automatically when checking
                      completes.
Address 12 --> Bit 0 (generator operating) when set the generator is operational.
               Bit 1 (generator complete) is set when the generator completes generating the pattern.
               Bit 2 (irq) is set when the IRQ fires, write a 1 to this bit to clear the interrupt.



Version History:

1.0  (04/14/2010)  Initial version used in the Qsys tutorial

2.0  (09/18/2015)  New version that adds interrupts, sop/eop, and a new CSR mapping (software is not compatible with versions 1.0 and 2.0) Almost a complete re-write
                   except the comparison pipeline.
                   
2.1  (04/10/2018)  Added the last transfer size to control bits 15:8 and the src_empty signal.  Any time EOP is sent the empty signal will be set to DATA_WIDTH/8 -
                   last transfer size.  For example for a 32-bit data path if you want only symbol 0 to be valid during the EOP beat you set the last transfer size to
                   1 and make sure eop generation is enabled (control[3]).  In this case the empty value will be set to 3 since empty specifies how many symbols are invalid.
*/



module mtm_custom_pattern_generator (
  clk,
  reset,

  csr_address,
  csr_writedata,
  csr_write,
  csr_readdata,
  csr_read,
  csr_byteenable,
  
  irq,

  pattern_address,
  pattern_writedata,
  pattern_write,
  pattern_byteenable,

  src_data,
  src_valid,
  src_ready,
  src_empty,
  src_sop,
  src_eop
);


  parameter DATA_WIDTH = 128;          // must be an even multiple of 8 and is used to determine the width of the 2nd port of the on-chip RAM
  parameter MAX_PATTERN_LENGTH = 64;   // used to determine the depth of the on-chip RAM and modulo counter width, set to a multiple of 2
  parameter ADDRESS_WIDTH = 6;         // log2(MAX_PATTERN_LENGTH) set by the .tcl file
  parameter EMPTY_WIDTH = 4;           // log2(DATA_WIDTH/8) set by the .tcl file

  localparam NUM_OF_SYMBOLS = DATA_WIDTH / 8;

  input clk;
  input reset;

  input [1:0] csr_address;
  input [31:0] csr_writedata;
  input csr_write;
  output reg [31:0] csr_readdata;
  input csr_read;
  input [3:0] csr_byteenable;
  
  output reg irq;

  input [ADDRESS_WIDTH-1:0] pattern_address;
  input [DATA_WIDTH-1:0] pattern_writedata;
  input pattern_write;
  input [(DATA_WIDTH/8)-1:0] pattern_byteenable;

  output wire [DATA_WIDTH-1:0] src_data;
  output wire src_valid;
  input src_ready;
  output wire [EMPTY_WIDTH-1:0] src_empty;
  output wire src_sop;
  output wire src_eop;

  
  reg [31:0] payload_length;
  reg [15:0] pattern_length;
  reg [15:0] pattern_position;
  reg [31:0] control;
  
  reg [15:0] pattern_length_minus_one;
  wire load_pattern_length_minus_one;
  reg [31:0] payload_length_counter;
  wire load_payload_length_counter;
  wire decrement_payload_length_counter;
  reg [15:0] pattern_position_counter;
  wire load_pattern_position_counter;
  wire increment_pattern_position_counter;
  wire sop;
  wire eop;
  wire [EMPTY_WIDTH-1:0] empty;
  reg complete;
  wire set_complete;
  wire clear_complete;
  wire set_irq;
  wire clear_irq;
  reg running;
  wire set_running;
  wire clear_running;
  reg first_beat;
  wire set_first_beat;
  wire clear_first_beat;
  
  wire [ADDRESS_WIDTH-1:0] internal_memory_address;
  wire read_memory;
  reg sop_d1;
  reg sop_d2;
  reg sop_d3;
  reg eop_d1;
  reg eop_d2;
  reg eop_d3;
  reg read_memory_d1;
  reg read_memory_d2;
  reg read_memory_d3;
  wire [DATA_WIDTH-1:0] memory_data;
  reg [DATA_WIDTH-1:0] memory_data_d1;
  reg [EMPTY_WIDTH-1:0] empty_d1;
  reg [EMPTY_WIDTH-1:0] empty_d2;
  reg [EMPTY_WIDTH-1:0] empty_d3;
  




  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      payload_length <= 32'h00000000;
    end
    else
    begin
      if ((csr_address == 2'b00) & (csr_write == 1) & (csr_byteenable[0] == 1))
        payload_length[7:0] <= csr_writedata[7:0];
      if ((csr_address == 2'b00) & (csr_write == 1) & (csr_byteenable[1] == 1))
        payload_length[15:8] <= csr_writedata[15:8];
      if ((csr_address == 2'b00) & (csr_write == 1) & (csr_byteenable[2] == 1))
        payload_length[23:16] <= csr_writedata[23:16];
      if ((csr_address == 2'b00) & (csr_write == 1) & (csr_byteenable[3] == 1))
        payload_length[31:24] <= csr_writedata[31:24];
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      pattern_length <= 16'h0000;
    end
    else
    begin
      if ((csr_address == 2'b01) & (csr_write == 1) & (csr_byteenable[0] == 1))
        pattern_length[7:0] <= csr_writedata[7:0];
      if ((csr_address == 2'b01) & (csr_write == 1) & (csr_byteenable[1] == 1))
        pattern_length[15:8] <= csr_writedata[15:8];
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      pattern_position <= 16'h0000;
    end
    else
    begin
      if ((csr_address == 2'b01) & (csr_write == 1) & (csr_byteenable[2] == 1))
        pattern_position[7:0] <= csr_writedata[23:16];
      if ((csr_address == 2'b01) & (csr_write == 1) & (csr_byteenable[3] == 1))
        pattern_position[15:8] <= csr_writedata[31:24];
    end
  end
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      control <= 32'h00000000;
    end
    else
    begin
      if ((csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[0] == 1))
        control[7:0] <= csr_writedata[7:0];
      if ((csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[1] == 1))
        control[15:8] <= csr_writedata[15:8];
      if ((csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[2] == 1))
        control[23:16] <= csr_writedata[23:16];
      if ((csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[3] == 1))
        control[31:24] <= csr_writedata[31:24];
    end
  end 
  
  // shadow register of the payload length, when software reads back the payload length this is the value returned since it tracks the progress
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      payload_length_counter <= 32'h00000000;
    end
    else
    begin
      if (load_payload_length_counter == 1)
      begin
        payload_length_counter <= payload_length;
      end
      else if (decrement_payload_length_counter == 1)
      begin
        payload_length_counter <= payload_length_counter - 1'b1;
      end
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      pattern_length_minus_one <= 16'h0000;
    end
    else if (load_pattern_length_minus_one == 1)
    begin
      pattern_length_minus_one <= pattern_length - 1'b1;
    end
  end
  
  // shadow register of the pattern position, when software reads back the pattern position this is the value returned since it tracks the progress
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      pattern_position_counter <= 16'h0000;
    end
    else
    begin
      if (load_pattern_position_counter == 1)
      begin
        pattern_position_counter <= pattern_position;
      end
      else if (increment_pattern_position_counter == 1)
      begin
        pattern_position_counter <= (pattern_position_counter == pattern_length_minus_one)? 16'h0000: pattern_position_counter + 1'b1;  // when the end of the pattern is reached need to roll around
      end
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      csr_readdata <= 32'h00000000;
    end
    else if (csr_read == 1)
    begin
      case (csr_address)
        2'b00:  csr_readdata <= payload_length_counter;
        2'b01:  csr_readdata <= {pattern_position_counter, pattern_length};
        2'b10:  csr_readdata <= {running, control[30:0]};  // running is a shadow register
        2'b11:  csr_readdata <= {29'h00000000, irq, complete, running};
        default: csr_readdata <= {29'h00000000, irq, complete, running};
      endcase
    end
  end
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      complete <= 0;
    end
    else
    begin
      if (set_complete == 1)
      begin
        complete <= 1;
      end
      else if (clear_complete == 1)
      begin
        complete <= 0;
      end
    end
  end
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      irq <= 0;
    end
    else
    begin
      if (set_irq == 1)
      begin
        irq <= 1;
      end
      else if (clear_irq == 1)
      begin
        irq <= 0;
      end
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      running <= 0;
    end
    else
    begin
      if (set_running == 1)
      begin
        running <= 1;
      end
      else if (clear_running == 1)
      begin
        running <= 0;
      end
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      first_beat <= 0;
    end
    else
    begin
      if (set_first_beat == 1)
      begin
        first_beat <= 1;
      end
      else if (clear_first_beat == 1)
      begin
        first_beat <= 0;
      end
    end
  end
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      read_memory_d1 <= 0;
      read_memory_d2 <= 0;
      read_memory_d3 <= 0;
      memory_data_d1 <= 0;
      sop_d1 <= 0;
      sop_d2 <= 0;
      sop_d3 <= 0;
      eop_d1 <= 0;
      eop_d2 <= 0;
      eop_d3 <= 0;
      empty_d1 <= 'h0;
      empty_d2 <= 'h0;
      empty_d3 <= 'h0;
    end
    else
    begin
      read_memory_d1 <= read_memory;
      read_memory_d2 <= read_memory_d1;
      read_memory_d3 <= read_memory_d2;
      memory_data_d1 <= memory_data;
      sop_d1 <= sop;
      sop_d2 <= sop_d1;
      sop_d3 <= sop_d2;
      eop_d1 <= eop;
      eop_d2 <= eop_d1;
      eop_d3 <= eop_d2;
      empty_d1 <= empty;
      empty_d2 <= empty_d1;
      empty_d3 <= empty_d2;
    end
  end



  // Port A used to access pattern from slave port, Port B used to send data out the source port
  altsyncram pattern_memory (
    .clock0 (clk),
    .data_a (pattern_writedata),
    .wren_a (pattern_write),
    .byteena_a (pattern_byteenable),
    .address_a (pattern_address),
    .address_b (internal_memory_address),
    .q_b (memory_data)
  );
  defparam pattern_memory.operation_mode = "DUAL_PORT";
  defparam pattern_memory.lpm_type = "altsyncram";
  defparam pattern_memory.read_during_write_mode_mixed_ports = "DONT_CARE";
  defparam pattern_memory.power_up_uninitialized = "TRUE";
  defparam pattern_memory.byte_size = 8;
  defparam pattern_memory.width_a = DATA_WIDTH;
  defparam pattern_memory.width_b = DATA_WIDTH;
  defparam pattern_memory.widthad_a = ADDRESS_WIDTH;
  defparam pattern_memory.widthad_b = ADDRESS_WIDTH;
  defparam pattern_memory.width_byteena_a = (DATA_WIDTH/8);
  defparam pattern_memory.numwords_a = MAX_PATTERN_LENGTH;
  defparam pattern_memory.numwords_b = MAX_PATTERN_LENGTH;
  defparam pattern_memory.address_reg_b = "CLOCK0";
  defparam pattern_memory.outdata_reg_b = "CLOCK0";



// Avalon-ST is typically network ordered (most significant symbol in lowest order bits)
genvar i;
generate
  for (i = 0; i < NUM_OF_SYMBOLS; i = i + 1)
  begin : byte_reversal
    assign src_data[((8*(i+1))-1):(8*i)] = memory_data_d1[((8*((NUM_OF_SYMBOLS-1-i)+1))-1):(8*(NUM_OF_SYMBOLS-1-i))];
  end
endgenerate



  assign set_irq = ((set_complete == 1) & (control[1] == 1));
  assign clear_irq = (set_running == 1) | ((csr_address == 2'b11) & (csr_write == 1) & (csr_byteenable[0] == 1) & (csr_writedata[2] == 1));

  assign set_running = (csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[3] == 1) & (csr_writedata[31] == 1);
  assign clear_running = (set_complete == 1);
  
  
  assign set_complete = (running == 1) &
                        (((control[0] == 0) & (payload_length_counter == 32'h00000001) & (src_ready == 1)) |    // last beat in a length terminated pattern generation completed
                        ((control[31] == 0) & (src_ready == 1)));         // host disabled generator in the middle of pattern generation, need to let one final beat out to drive out EOP
  assign clear_complete = (csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[3] == 1) & (csr_writedata[31] == 0);

  assign read_memory = (running == 1) & (src_ready == 1);

  assign set_first_beat = (set_running == 1);
  assign clear_first_beat = (running == 1) & (src_ready == 1) & (first_beat == 1);  // as soon as the first beat enters the pipeline we disable the first_beat register

  assign sop = (first_beat == 1) & (control[2] == 1);
  assign eop = (set_complete == 1)& (control[3] == 1);
  assign empty = (eop == 1)? ((DATA_WIDTH/8) - control[15:8]) : 'h0;  // empty must be 0 except on the last beat

  assign load_payload_length_counter = (set_running == 1);
  assign decrement_payload_length_counter = (running == 1) & (src_ready == 1);
  assign load_pattern_position_counter = (set_running == 1);
  assign increment_pattern_position_counter = (running == 1) & (src_ready == 1);
  assign load_pattern_length_minus_one = (set_running == 1);


  
  // memory_data_d1 has three cycles of latency just like the valid, sop, eop, and empty bits
  assign src_valid = read_memory_d3;
  assign src_sop = sop_d3;
  assign src_eop = eop_d3;
  assign src_empty = empty_d3;
  
  assign internal_memory_address = pattern_position_counter;

endmodule
