module write_response_bridge (
  clk,
  reset,
  
  s_address,
  s_writedata,
  s_write,
  s_byteenable,
  s_burst,
  s_response,
  s_write_response_valid,
  s_waitrequest,
  
  m_address,
  m_writedata,
  m_write,
  m_byteenable,
  m_burst,
  m_response,
  m_write_response_valid,
  m_waitrequest
);

parameter ADDRESS_WIDTH = 48;
parameter DATA_WIDTH = 512;
parameter BURST_WIDTH = 3;  // 1+log2(MAX BURST).  MAX BURST must be a power of 2
parameter MAX_PENDING_WRITES_WIDTH = 6;  // 1+log2(MAX PENDING WRITES).  MAX PENDING WRITES must be a power of 2

input clk;
input reset;
  
input [ADDRESS_WIDTH-1:0] s_address;
input [DATA_WIDTH-1:0] s_writedata;
input s_write;
input [(DATA_WIDTH/8)-1:0] s_byteenable;
input [BURST_WIDTH-1:0] s_burst;
output wire [1:0] s_response;
output wire s_write_response_valid;
output wire s_waitrequest;
  
output wire [ADDRESS_WIDTH-1:0] m_address;
output wire [DATA_WIDTH-1:0] m_writedata;
output wire m_write;
output wire [(DATA_WIDTH/8)-1:0] m_byteenable;
output wire [BURST_WIDTH-1:0] m_burst;
input [1:0] m_response;
input m_write_response_valid;
input m_waitrequest;


// downstream FFs
reg [ADDRESS_WIDTH-1:0] address;
reg [DATA_WIDTH-1:0] writedata;
reg write;
reg [(DATA_WIDTH/8)-1:0] byteenable;
reg [BURST_WIDTH-1:0] burst;
wire enable_downstream;

// upstream FFs
reg [1:0] response;
reg write_response_valid; 
reg waitrequest;

// transaction counting logic
reg [MAX_PENDING_WRITES_WIDTH-1:0] transaction_counter;
wire inc_transaction_counter;
wire dec_transaction_counter;
reg [BURST_WIDTH-1:0] burst_counter;
wire load_burst_counter;
wire dec_burst_counter;



always @ (posedge clk)
begin
  if (reset)
  begin
    write <= 1'b0;  // only need the write signal to get cleared out, the rest of the FFs can hold their old state
  end
  else if (enable_downstream)
  begin
    address <= s_address;
    writedata <= s_writedata;
    byteenable <= s_byteenable;
    burst <= s_burst;
    write <= s_write & (transaction_counter[MAX_PENDING_WRITES_WIDTH-1] == 1'b0);  // need to throttle writes when too many are in flight
  end
end


always @ (posedge clk)
begin
  write_response_valid <= m_write_response_valid;
  waitrequest <= m_waitrequest;
  response <= m_response;
end


always @ (posedge clk)
begin
  if (reset)
  begin
    burst_counter <= {BURST_WIDTH{1'b0}};
  end
  else if (load_burst_counter)
  begin
    burst_counter <= s_burst - 1'b1;
  end
  else if (dec_burst_counter)
  begin
    burst_counter <= burst_counter - 1'b1;
  end
end


always @ (posedge clk)
begin
  if (reset)
  begin
    transaction_counter <= {MAX_PENDING_WRITES_WIDTH{1'b0}};
  end
  else
  begin
    case ({inc_transaction_counter, dec_transaction_counter})
      2'b01:  transaction_counter <= transaction_counter - 1'b1;
      2'b10:  transaction_counter <= transaction_counter + 1'b1;
      default:  transaction_counter <= transaction_counter;  // inc and dec so net zero or neither
    endcase
  end
end


assign load_burst_counter = (burst_counter == 0) & (s_write == 1'b1) & (s_waitrequest == 1'b0);
assign dec_burst_counter = (burst_counter != 0) & (s_write == 1'b1) & (s_waitrequest == 1'b0);

assign inc_transaction_counter = load_burst_counter;
assign dec_transaction_counter = s_write_response_valid;


assign enable_downstream = (m_waitrequest == 1'b0) & (transaction_counter[MAX_PENDING_WRITES_WIDTH-1] == 1'b0);


assign m_address = address;
assign m_writedata = writedata;
assign m_write = write;
assign m_byteenable = byteenable;
assign m_burst = burst;

assign s_response = response;
assign s_write_response_valid = write_response_valid;

// using pipelined waitrequest is causing one too many beats to get through, fix this later
// assign s_waitrequest = waitrequest | transaction_counter[MAX_PENDING_WRITES_WIDTH-1];  // backpressure when we have too many writes in flight as well
assign s_waitrequest = m_waitrequest | transaction_counter[MAX_PENDING_WRITES_WIDTH-1];  // backpressure when we have too many writes in flight as well

endmodule
