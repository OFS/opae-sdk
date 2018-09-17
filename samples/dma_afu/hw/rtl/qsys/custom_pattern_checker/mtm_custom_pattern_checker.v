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

Custom pattern checker core

Author:  JCJB
Date:    04/14/2010

Version:  2.1

Description:  This component is programmed via a host or master through the pattern slave port to program the internal test memory.  When the host
              writes to the start bit of the CSR slave port the component will begin to send the contents from the internal memory to the data streaming
              port.  You should use the custom pattern generator component with this component.

Register map:

|-------------------------------------------------------------------------------------------------------------------------------------------------------|
|  Address   |   Access Type   |                                                            Bits                                                        |
|            |                 |------------------------------------------------------------------------------------------------------------------------|
|            |                 |            31..24            |            23..16            |            15..8            |            7..0            |
|-------------------------------------------------------------------------------------------------------------------------------------------------------|
|     0      |       r/w       |                                                       Payload Length                                                   |
|     4      |       r/w       |                        Pattern Position                                             Pattern Length                     |
|     8      |       r/w       |                                                          Control                                                       |
|     12     |      r/wclr     |                                                          Status                                                        |
|-------------------------------------------------------------------------------------------------------------------------------------------------------|


Address 0  --> Bits 31:0 are used store the payload length.  This field should only be written to while the checker is stopped.  This field is ignored if the
               infinite length test enable bit is set in the control register.
Address 4  --> Bits 15:0 is used to store pattern length, bits 31:16 used to store a new position in the pattern.  The position will update as the checker is
               operating.
               These fields should only be written to while the checker core is stopped.
Address 8  --> Bit 0 (infinite length test enable)is used to instruct the checker to ignore the payload length field and check the pattern until stopped.
               Bit 1 (checking complete IRQ mask) is the checking complete interrupt mask bit.
               Bit 2 (failure detected IRQ mask) is the checking failure interrupt mask bit.
               Bit 3 (accept only packet data enable) is used to instruct the checker to ignore any data received before SOP and after EOP.
               Bit 4 (stop on failure detection enable) is used to stop the checker when a failure is detected
               Bit 31 (checker enable) is used to start the checker core so that it begins receiving data.  This field must be set at the same time or after all the
                      other fields bits are programmed.  This field must be cleared before modifying other fields.  This field is cleared automatically when checking
                      completes.
Address 12 --> Bit 0 (checker operating) when set the checker is operational.
               Bit 1 (checker complete) is set when the checker completes the test.
               Bit 2 (error detected) is set when the checker detects an error during the test.
               Bit 3 (irq) is set when the IRQ fires, write a 1 to this bit to clear the interrupt
               Bit 4 (error count overflow) when the error counter overflows this bit is set.  This bit is cleared when the checker is started.
               Bits 31:16 (error count) each time an error is detected this counter is incremented.  This counter is cleared when the checker is started. 

               
Version History:

1.0  (04/14/2010)  Initial version used in the Qsys tutorial

2.0  (09/15/2015)  New version that adds interrupts, sop/eop, and a new CSR mapping (software is not compatible with versions 1.0 and 2.0) Almost a complete re-write
                   except the comparison pipeline. 

2.1  (04/10/2018)  Added snk_empty so that when snk_eop is high the checker will only perform the pattern checking across symbols (bytes) that are valid.                   
*/



module mtm_custom_pattern_checker (
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

  snk_data,
  snk_valid,
  snk_ready,
  snk_empty,
  snk_sop,
  snk_eop
);


  parameter DATA_WIDTH = 128;           // must be an even multiple of 8 and is used to determine the width of the 2nd port of the on-chip RAM and streaming port
  parameter MAX_PATTERN_LENGTH = 64;    // used to determine the depth of the on-chip RAM and modulo counter width, set to a multiple of 2
  parameter ADDRESS_WIDTH = 6;          // log2(MAX_PATTERN_LENGTH) will be set by the .tcl file
  parameter EMPTY_WIDTH = 4;            // log2(DATA_WIDTH/8) set by the .tcl file
  
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

  input [DATA_WIDTH-1:0] snk_data;
  input snk_valid;
  output wire snk_ready;
  input [EMPTY_WIDTH-1:0] snk_empty;
  input snk_sop;
  input snk_eop;

  
  reg [31:0] payload_length;
  reg [15:0] pattern_length;
  reg [15:0] pattern_position;
  reg [31:0] control;
  
  reg [15:0] pattern_length_minus_one;
  wire load_pattern_length_minus_one;
  reg [31:0] payload_length_counter;
  reg [15:0] pattern_position_counter;
  wire load_payload_length_counter;
  wire decrement_payload_length_counter;
  wire load_pattern_position_counter;
  wire increment_pattern_position_counter;
  wire set_irq;
  wire clear_irq;
  reg running;
  reg complete;
  reg error;
  wire set_running;
  wire clear_running;
  wire set_complete;
  wire clear_complete;
  wire set_error;
  wire clear_error;
  reg sop_seen;
  wire set_sop_seen;
  wire clear_sop_seen;
  wire packet_filter;
  wire valid_data;
  reg valid_data_d1;
  reg valid_data_d2;
  reg valid_data_d3;
  reg valid_data_d4;
  reg valid_data_d5;
  wire data_compare;
  wire [DATA_WIDTH-1:0] data_in;
  reg [DATA_WIDTH-1:0] data_in_d1;
  reg [DATA_WIDTH-1:0] data_in_d2;
  reg [DATA_WIDTH-1:0] data_in_d3;
  wire [DATA_WIDTH-1:0] memory_data;
  reg [DATA_WIDTH-1:0] memory_data_d1;
  reg [EMPTY_WIDTH-1:0] empty_d1;
  reg [EMPTY_WIDTH-1:0] empty_d2;
  reg [EMPTY_WIDTH-1:0] empty_d3;
  reg eop_d1;
  reg eop_d2;
  reg eop_d3;
  reg [15:0] error_counter;
  wire increment_error_counter;
  wire clear_error_counter;
  reg error_counter_overflow;
  wire set_error_counter_overflow;
  wire clear_error_counter_overflow;
  

  
  
  // this is a shadow register that will get loaded into pattern_length_counter when the checker enable bit is written to
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
  
  // this is a shadow register that will get loaded into pattern_position_counter when the checker enable bit is written to
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
        control[30:24] <= csr_writedata[30:24];    // control bit 31 is handled separate since it's self clearing and will be represented by the 'running' register
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
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      payload_length_counter <= 32'h00000000;
    end
    else if (load_payload_length_counter == 1)
    begin
      payload_length_counter <= payload_length;
    end
    else if (decrement_payload_length_counter == 1)
    begin
      payload_length_counter <= payload_length_counter - 1'b1;
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      pattern_position_counter <= 16'h0000;
    end
    else if (load_pattern_position_counter == 1)
    begin
      pattern_position_counter <= pattern_position;
    end
    else if(increment_pattern_position_counter == 1)
    begin
      pattern_position_counter <= (pattern_position_counter == pattern_length_minus_one)? 16'h0000: pattern_position_counter + 1'b1;  // when the end of the pattern is reached need to roll around
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      error_counter_overflow <= 0;
    end
    else
    begin
      if (clear_error_counter_overflow == 1)
      begin
        error_counter_overflow <= 0;
      end
      else if (set_error_counter_overflow == 1)
      begin
        error_counter_overflow <= 1;
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
        2'b00:  csr_readdata <= payload_length_counter;   // output the counter instead of the value the host set which doesn't move
        2'b01:  csr_readdata <= {pattern_position_counter, pattern_length};  // output the counter instead of the value the host set which doesn't move
        2'b10:  csr_readdata <= {running, control[30:0]};  // the enable bit is self clearing so it is handled separately
        2'b11:  csr_readdata <= {28'h0000000, irq, error, complete, running};
        default:  csr_readdata <= {error_counter, 11'h000, error_counter_overflow, irq, error, complete, running};
      endcase
    end
  end


  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      data_in_d1 <= 0;
      data_in_d2 <= 0;
      data_in_d3 <= 0;
      valid_data_d1 <= 0;
      valid_data_d2 <= 0;
      valid_data_d3 <= 0;
      valid_data_d4 <= 0;
      valid_data_d5 <= 0;
      eop_d1 <= 0;
      eop_d2 <= 0;
      eop_d3 <= 0;
      empty_d1 <= 'h0;
      empty_d2 <= 'h0;
      empty_d3 <= 'h0;
    end
    else
    begin
      data_in_d1 <= data_in;
      data_in_d2 <= data_in_d1;
      data_in_d3 <= data_in_d2;
      valid_data_d1 <= valid_data;
      valid_data_d2 <= valid_data_d1;
      valid_data_d3 <= valid_data_d2;
      valid_data_d4 <= valid_data_d3;
      valid_data_d5 <= valid_data_d4;
      eop_d1 <= snk_eop;
      eop_d2 <= eop_d1;
      eop_d3 <= eop_d2;
      empty_d1 <= snk_empty;
      empty_d2 <= empty_d1;
      empty_d3 <= empty_d2;
    end
  end


  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      memory_data_d1 <= 0;
    end
    else if (valid_data_d2 == 1)
    begin
      memory_data_d1 <= memory_data;
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
        irq <= 1;
      else if (clear_irq == 1)
        irq <= 0;
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
        running <= 1;
      else if (clear_running == 1)
        running <= 0;
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
        complete <= 1;
      else if (clear_complete == 1)
        complete <= 0;
    end
  end  
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      error <= 0;
    end
    else
    begin
      if (set_error == 1)
        error <= 1;
      else if (clear_error == 1)
        error <= 0;
    end
  end   
  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      sop_seen <= 0;
    end
    else
    begin
      if (set_sop_seen == 1)
        sop_seen <= 1;
      else if (clear_sop_seen == 1)  // can't support packets that start and end on a single beat
        sop_seen <= 0;
    end
  end

  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      error_counter <= 16'h0000;
    end
    else
    begin
      if (clear_error_counter == 1)
      begin
        error_counter <= 16'h0000;
      end
      else if (increment_error_counter == 1)
      begin
        error_counter <= error_counter + 1'b1;
      end
    end
  end
  
  
  // Port A used to access pattern from slave port, Port B internally to compare incoming data against.
  altsyncram pattern_memory (
    .clock0 (clk),
    .data_a (pattern_writedata),
    .wren_a (pattern_write),
    .byteena_a (pattern_byteenable),
    .address_a (pattern_address),
    .address_b (pattern_position_counter[ADDRESS_WIDTH-1:0]),
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


  // two cycle comparator that compares the input data against the memory value
  mtm_custom_pattern_checker_comparator the_mtm_custom_pattern_checker_comparator (
    .clk            (clk),
    .reset          (reset),
    .data_a         (memory_data_d1),
    .data_b         (data_in_d3),
    .empty          (empty_d3),
    .eop            (eop_d3),
    .compare_result (data_compare)
  );
  defparam the_mtm_custom_pattern_checker_comparator.DATA_WIDTH = DATA_WIDTH;
  defparam the_mtm_custom_pattern_checker_comparator.EMPTY_WIDTH = EMPTY_WIDTH;

  
// Avalon-ST is network ordered (usually) so putting most significant symbol in lowest bits
genvar i;
generate
  for (i = 0; i < NUM_OF_SYMBOLS; i = i + 1)
  begin : byte_reversal
    assign data_in[((8*(i+1))-1):(8*i)] = snk_data[((8*((NUM_OF_SYMBOLS-1-i)+1))-1):(8*(NUM_OF_SYMBOLS-1-i))];
  end
endgenerate



  //control bits 1 and 2 are the masks for completion and error interrupts
  assign set_irq = ((set_complete == 1) & (control[1] == 1)) |
                   ((set_error == 1) & (control[2] == 1));
  assign clear_irq = (set_running == 1) | ((csr_address == 2'b11) & (csr_write == 1) & (csr_byteenable[0] == 1) & (csr_writedata[3] == 1));
  
  assign set_running = (csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[0] == 1) & (csr_writedata[31] == 1);
  assign clear_running = (set_complete == 1);  // this will trigger one cycle after the pattern is completed or an error is detected
  
  assign set_complete = (running == 1) &
                        ((csr_address == 2'b10) & (csr_write == 1) & (csr_byteenable[0] == 1) & (csr_writedata[31] == 0) |  // host stops the checker
                        ((set_error == 1) & (control[4] == 1)) |                                                            // error detected and stop of error is enabled
                        ((payload_length_counter == 32'h00000001) & (control[0] == 0) & (valid_data == 1)));                // non-infinite test and the payload length counter expired
  assign clear_complete = (set_running == 1);
  
  assign set_error = (data_compare == 0) & (valid_data_d5 == 1);
  assign clear_error = (set_running == 1);
  
  assign increment_error_counter = (set_error == 1);
  assign clear_error_counter = (set_running == 1); 
  assign set_error_counter_overflow = (error_counter == 16'hFFFF) & (increment_error_counter == 1);
  assign clear_error_counter_overflow = (clear_error_counter == 1);  
  
  assign set_sop_seen = (snk_ready == 1) & (snk_valid == 1) & (snk_sop == 1);
  assign clear_sop_seen = (snk_ready == 1) & (snk_valid == 1) & (snk_eop == 1);
  assign packet_filter = (control[3] == 0) |  // let any traffic through
                         (control[3] == 1) & (sop_seen == 1) | (set_sop_seen == 1);  // need to use set_sop_seen since it sets before sop_seen is set so we don't want to filter out the SOP beat
                         
  assign valid_data = (packet_filter == 1) & (snk_valid == 1) & (snk_ready == 1);
  assign decrement_payload_length_counter = (valid_data == 1) & (control[0] == 0);   // no point decrementing this counter if an infinite transfer is occuring
  assign increment_pattern_position_counter = (valid_data == 1);
  assign load_payload_length_counter = (set_running == 1);
  assign load_pattern_position_counter = (set_running == 1);
  assign load_pattern_length_minus_one = (set_running == 1);

  assign snk_ready = (running == 1);  // checker is always ready when enabled
  
endmodule


// this module will be used to compare two values that have data widths 32, 64, 128, 256, 512, 1024.  The latency is 2 cycles.
module mtm_custom_pattern_checker_comparator (
  clk,
  reset,

  data_a,
  data_b,
  empty,
  eop,
  
  compare_result
);

  parameter DATA_WIDTH = 128;
  parameter EMPTY_WIDTH = 4;
  localparam NUM_COMPARES = DATA_WIDTH/8;
  
  input clk;
  input reset;
  
  input [DATA_WIDTH-1:0] data_a;
  input [DATA_WIDTH-1:0] data_b;
  input [EMPTY_WIDTH-1:0] empty;
  input eop;
  
  output reg compare_result;
  
  reg [NUM_COMPARES-1:0] symbol_mask;           // used to mask off comparisons of lanes that are not value (empty not zero)
  reg [NUM_COMPARES-1:0] intermediate_compare;  // used to store the individual 32-bit comparisons
  
  
genvar i;
generate
  for(i=0; i < NUM_COMPARES; i=i+1)
  begin: eight_bit_comparisons
    always @ (posedge clk)
    begin
      if (reset == 1)
      begin
        intermediate_compare[i] <= 0;
      end
      else
      begin
        intermediate_compare[i] <= (data_a[((8 * i) + 7) : (8 * i)] == data_b[((8 * i) + 7) : (8 * i)]);
      end
    end 
  end
endgenerate  
  
  /*
    symbol_mask will have bits asserted for symbols that are valid.  We then take this mask, invert it and OR it the intermediate compares which
    are performed on each symbol.  This will make sure that symbols with symbol_mask set low will be forced high for the final comparison against
    all ones.  Essentially we are forcing the symbol comparion to pass any time symbol_mask for that comparion is zero so that it will not be factored
    into the ones check at the end.
  */  
  always @ (posedge clk)
  begin
    if (reset == 1)
    begin
      compare_result <= 0;
      symbol_mask <= 'h0;
    end
    else
    begin
      symbol_mask <= (eop == 1)? ({NUM_COMPARES{1'b1}} >> empty) : {NUM_COMPARES{1'b1}};  // empty must only be observed on last beat
      compare_result <= ((intermediate_compare | ~symbol_mask) == {NUM_COMPARES{1'b1}});  // only set high when all the individual comparisons pass
    end
  end
  
endmodule
