// (C) 2018 Intel Corporation. All rights reserved.
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
Author:  JCJB
Date:    05/04/2018

This component is a passthrough streaming module that stops the flow of data automatically.  A host enables the flow of data
and this component can stop the flow by either allowing a finite amount of data to be transferred or when a packet completes
(EOP sent).  After the data flow is stopped, the host must re-enable the flow of data again.


Address   Name                      Access     Description
-------   ----                      ------     -----------
0x0       bytes_transferred[31:0]   R          Reads from this location will cause bytes_transferred[63:32] to be captured and retained.
0x4       bytes_transferred[63:32]  R          Ensure address 0x0 is read before this location to ensure this value is retained.
0x8       bytes_to_transfer[31:0]   R/W        Set this value when also setting control bit 1 otherwise this value is ignored.  Must be a multiple of (DATA_WIDTH/8)
0xC       control[3:0]              R/W        See descriptions below
0x10      status[1:0]               R/WClr     See descriptions below
0x14      <unused>                  R          Reads from this location will contain undefined data
0x18      <unused>                  R          Reads from this location will contain undefined data
0x1C      <unused>                  R          Reads from this location will contain undefined data


control[0] – Enable data flow – Defaults to cleared.  When set, the adapter will allow a deterministic transfer (control[1]) or 
             a non-deterministic transfer {control[2]), or both types.  If control[1] and control[2] are both cleared when 
             control[2] is set then the adapter acts as a passthrough with no way of automatically stopping the flow of data.
             The host can clear this bit at any time to stop the flow of data.  When the adapter terminates the data flow this
             bit will be cleared automatically by the hardware.

control[1] – Enable length terminated transfer – Defaults to cleared.  When set, the adapter will stop the flow of data once
             ‘bytes_transferred’ equals ‘bytes_to_transfer’.  When cleared, ‘bytes_to_transfer’ is not used by the adapter.

control[2] – Enable packet terminated transfer – Defaults to cleared.  When set, the adapter will stop the flow of data after
             it sends the incoming EOP to the component downstream.  When cleared, the adapter will ignore incoming EOPs.

control[3] – Clear bytes_transferred – Defaults to cleared.  When set, the adapter will reset the value of bytes_transferred
             to 0.  This bit is self-clearing and will always be 0 when read.

status[0] – Length terminated transfer occurred – Defaults to cleared.  When set, the adapter stopped the flow of data due to
            control[1] being set and the adapter passing ‘bytes_to_transfer’ amount of data.  To reset this bit the host must
            write a 1 to this bit location.

status[1] – Packet terminated transfer occurred – Defaults to cleared.  When set, the adapter stopped the flow of data due to
            control[1] being set and the adapter passing the end of packet downstream.  To reset this bit the host must write a
            1 to this bit location.
*/

module s2m_streaming_valve # (
  parameter DATA_WIDTH = 512,
  parameter EMPTY_WIDTH = 6
)
(
  input clk,
  input reset,
  
  // csr slave port with a 1 cycle read latency
  input [2:0] csr_address,
  input [31:0] csr_writedata,
  input csr_write,
  input [3:0] csr_byteenable,
  input csr_read,
  output reg [31:0] csr_readdata,
  
  input [DATA_WIDTH-1:0] snk_data,
  input snk_valid,
  output wire snk_ready,
  input snk_sop,
  input snk_eop,
  input [EMPTY_WIDTH-1:0] snk_empty,

  output wire [DATA_WIDTH-1:0] src_data,
  output wire src_valid,
  input src_ready,
  output wire src_sop,
  output wire src_eop,
  output wire [EMPTY_WIDTH-1:0] src_empty
);


  reg [63:0] bytes_transferred;
  wire increment_bytes_transferred;
  reg clear_bytes_transferred;
  reg [31:0] snap_bytes_transferred;
  wire enable_snap_bytes_transferred;

  
  reg [31:0] bytes_to_transfer;
  reg [31:0] bytes_to_transfer_counter;
  reg load_bytes_to_transfer_counter;
  wire decrement_bytes_to_transfer_counter;

  reg enable_data_flow;
  reg set_enable_data_flow;
  reg set_enable_data_flow_d1;
  wire clear_enable_data_flow;

  reg enable_length_termination;
  
  reg enable_packet_termination;

  reg length_terminated;
  wire set_length_terminated;
  wire clear_length_terminated;
  
  reg packet_terminated;
  wire set_packet_terminated;
  wire clear_packet_terminated;


  
  always @ (posedge clk)
  begin
    if (clear_bytes_transferred == 1)
    begin
      bytes_transferred <= 64'h0;
    end
    else if (increment_bytes_transferred == 1)
    begin
      bytes_transferred <= bytes_transferred + (DATA_WIDTH/8);
    end
  end

  
  always @ (posedge clk)
  begin
    if (enable_snap_bytes_transferred == 1)
    begin
      snap_bytes_transferred <= bytes_transferred[63:32];
    end
  end
  
  
  always @ (posedge clk)
  begin
    if ((csr_address == 3'h2) & (csr_byteenable[0] == 1) & (csr_write == 1))
    begin
      bytes_to_transfer <= csr_writedata[7:0];
    end
    if ((csr_address == 3'h2) & (csr_byteenable[1] == 1) & (csr_write == 1))
    begin
      bytes_to_transfer <= csr_writedata[15:8];
    end
    if ((csr_address == 3'h2) & (csr_byteenable[2] == 1) & (csr_write == 1))
    begin
      bytes_to_transfer <= csr_writedata[23:16];
    end
    if ((csr_address == 3'h2) & (csr_byteenable[3] == 1) & (csr_write == 1))
    begin
      bytes_to_transfer <= csr_writedata[31:24];
    end
  end


  always @ (posedge clk)
  begin
    load_bytes_to_transfer_counter <= (csr_address == 3'h2) & (csr_byteenable[3] == 1) & (csr_write == 1);  // loading counter one cycle after data has been written
    clear_bytes_transferred <= (csr_address == 3'h3) & (csr_byteenable[0] == 1) & (csr_write == 1) & (csr_writedata[3] == 1);
    set_enable_data_flow <= (csr_address == 3'h3) & (csr_byteenable[0] == 1) & (csr_write == 1) & (csr_writedata[0] == 1);
    set_enable_data_flow_d1 <= set_enable_data_flow;  // need to use a delayed set in case host clears the bytes_transferred at the same time as starting the data flow
  end
  
  
  always @ (posedge clk)
  begin
    if (load_bytes_to_transfer_counter == 1)
    begin
      bytes_to_transfer_counter <= bytes_to_transfer;
    end
    else if (decrement_bytes_to_transfer_counter == 1)
    begin
      bytes_to_transfer_counter <= bytes_to_transfer_counter - (DATA_WIDTH/8);
    end
  end

  
  always @ (posedge clk)
  begin
    if (reset | clear_enable_data_flow)
    begin
      enable_data_flow <= 1'b0;
    end
    else if (set_enable_data_flow_d1)
    begin
      enable_data_flow <= 1'b1;
    end
  end
  
  
  always @ (posedge clk)
  begin
    if (reset)
    begin
      enable_length_termination <= 0;
      enable_packet_termination <= 0;
    end
    else if ((csr_address == 3'h4) & (csr_byteenable[0] == 1) & (csr_write == 1))
    begin
      enable_length_termination <= csr_writedata[1];
      enable_packet_termination <= csr_writedata[2];
    end
  end
  
  
  always @ (posedge clk)
  begin
    if (csr_read == 1)
    begin
      case (csr_address)
        3'h0:  csr_readdata <= bytes_transferred[31:0];
        3'h1:  csr_readdata <= snap_bytes_transferred;
        3'h2:  csr_readdata <= bytes_to_transfer;
        3'h3:  csr_readdata <= {{28{1'b0}}, 1'b0, enable_packet_termination, enable_length_termination, enable_data_flow}; 
        default:  csr_readdata <= {{30{1'b0}}, length_terminated, packet_terminated};
      endcase
    end
  end


  always @ (posedge clk)
  begin
    if (reset | clear_length_terminated)
    begin
      length_terminated <= 1'b0;
    end
    else if (set_length_terminated)
    begin
      length_terminated <= 1'b1;
    end
  end


  always @ (posedge clk)
  begin
    if (reset | clear_packet_terminated)
    begin
      packet_terminated <= 1'b0;
    end
    else if (set_packet_terminated)
    begin
      packet_terminated <= 1'b1;
    end
  end




  

  assign increment_bytes_transferred = src_valid & snk_ready;
  
  assign enable_snap_bytes_transferred = (csr_address == 3'h0) & (csr_byteenable[3] == 1) & (csr_read == 1);
  
  assign decrement_bytes_to_transfer_counter = increment_bytes_transferred & enable_length_termination;

  assign set_length_terminated = (bytes_to_transfer_counter == 32'h64) & (enable_data_flow == 1);
  assign clear_length_terminated = (csr_address == 3'h4) & (csr_byteenable[0] == 1) & (csr_write == 1) & (csr_writedata[0] == 1);
  
  assign set_packet_terminated = (snk_valid == 1) & (snk_ready == 1) & (snk_eop == 1);
  assign clear_packet_terminated = (csr_address == 3'h4) & (csr_byteenable[0] == 1) & (csr_write == 1) & (csr_writedata[1] == 1);
  
  assign clear_enable_data_flow = set_length_terminated | set_packet_terminated;
  
  assign src_data = snk_data;
  assign src_empty = snk_empty;
  assign src_sop = snk_sop;
  assign src_eop = snk_eop;
  assign src_valid = snk_valid & enable_data_flow;
  assign snk_ready = src_ready & enable_data_flow;
  
endmodule
