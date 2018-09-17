# (C) 2001-2018 Intel Corporation. All rights reserved.
# Your use of Intel Corporation's design tools, logic functions and other 
# software and tools, and its AMPP partner logic functions, and any output 
# files from any of the foregoing (including device programming or simulation 
# files), and any associated documentation or information are expressly subject 
# to the terms and conditions of the Intel Program License Subscription 
# Agreement, Intel FPGA IP License Agreement, or other applicable 
# license agreement, including, without limitation, that your use is for the 
# sole purpose of programming logic devices manufactured by Intel and sold by 
# Intel or its authorized distributors.  Please refer to the applicable 
# agreement for further details.


package require -exact qsys 13.1

# +-----------------------------------
# | module SGDMA_dispatcher
# | 
set_module_property DESCRIPTION "SGDMA scheduling block"
set_module_property NAME dcp_modular_sgdma_dispatcher
set_module_property VERSION 17.0
#set_module_property GROUP "DMA/mSGDMA Sub-core"
#set_module_property GROUP "Basic Functions/DMA/mSGDMA Sub-core"
set_module_property AUTHOR "Intel Corporation"
set_module_property DISPLAY_NAME "DCP mSGDMA Dispatcher"
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE false
set_module_property INTERNAL false
set_module_property HIDE_FROM_QUARTUS true
# set_module_property DATASHEET_URL "file:/[get_module_property MODULE_DIRECTORY]Modular_SGDMA_Dispatcher_Core_UG.pdf"

# Device tree fields
#set_module_assignment embeddedsw.dts.vendor "ALTR"
#set_module_assignment embeddedsw.dts.group "msgdma"
#set_module_assignment embeddedsw.dts.name "msgdma-dispatcher"
#set_module_assignment embeddedsw.dts.compatible "ALTR,msgdma-dispatcher-1.0"
# | 
# +-----------------------------------

# +-----------------------------------
# | files
# | 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL dcp_dispatcher
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_dispatcher.v VERILOG PATH dcp_dispatcher.v TOP_LEVEL_FILE
add_fileset_file dcp_descriptor_buffers.v VERILOG PATH dcp_descriptor_buffers.v 
add_fileset_file dcp_csr_block.v VERILOG PATH dcp_csr_block.v 
add_fileset_file dcp_response_block.v VERILOG PATH dcp_response_block.v 
add_fileset_file dcp_fifo_with_byteenables.v VERILOG PATH dcp_fifo_with_byteenables.v 
add_fileset_file dcp_read_signal_breakout.v VERILOG PATH dcp_read_signal_breakout.v 
add_fileset_file dcp_write_signal_breakout.v VERILOG PATH dcp_write_signal_breakout.v 

add_fileset SIM_VERILOG SIM_VERILOG "" ""
set_fileset_property SIM_VERILOG TOP_LEVEL dcp_dispatcher
set_fileset_property SIM_VERILOG ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_dispatcher.v VERILOG PATH dcp_dispatcher.v
add_fileset_file dcp_descriptor_buffers.v VERILOG PATH dcp_descriptor_buffers.v 
add_fileset_file dcp_csr_block.v VERILOG PATH dcp_csr_block.v 
add_fileset_file dcp_response_block.v VERILOG PATH dcp_response_block.v 
add_fileset_file dcp_fifo_with_byteenables.v VERILOG PATH dcp_fifo_with_byteenables.v 
add_fileset_file dcp_read_signal_breakout.v VERILOG PATH dcp_read_signal_breakout.v 
add_fileset_file dcp_write_signal_breakout.v VERILOG PATH dcp_write_signal_breakout.v 

add_fileset SIM_VHDL SIM_VHDL "" ""
set_fileset_property SIM_VHDL TOP_LEVEL dcp_dispatcher
set_fileset_property SIM_VHDL ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_dispatcher.v VERILOG PATH dcp_dispatcher.v
add_fileset_file dcp_descriptor_buffers.v VERILOG PATH dcp_descriptor_buffers.v 
add_fileset_file dcp_csr_block.v VERILOG PATH dcp_csr_block.v 
add_fileset_file dcp_response_block.v VERILOG PATH dcp_response_block.v 
add_fileset_file dcp_fifo_with_byteenables.v VERILOG PATH dcp_fifo_with_byteenables.v 
add_fileset_file dcp_read_signal_breakout.v VERILOG PATH dcp_read_signal_breakout.v 
add_fileset_file dcp_write_signal_breakout.v VERILOG PATH dcp_write_signal_breakout.v 

# | 
# +-----------------------------------


set_module_property ELABORATION_CALLBACK elaborate_me
set_module_property VALIDATION_CALLBACK validate_me



# +-----------------------------------
# | parameters
# |
add_parameter PREFETCHER_USE_CASE INTEGER 0 "Pre-fetching use cases"
set_parameter_property PREFETCHER_USE_CASE DISPLAY_NAME "Enables Pre-Fetching use cases"
set_parameter_property PREFETCHER_USE_CASE DISPLAY_HINT boolean
set_parameter_property PREFETCHER_USE_CASE AFFECTS_GENERATION true
set_parameter_property PREFETCHER_USE_CASE HDL_PARAMETER true
set_parameter_property PREFETCHER_USE_CASE DERIVED false
set_parameter_property PREFETCHER_USE_CASE AFFECTS_ELABORATION true

add_parameter MODE INTEGER 0
set_parameter_property MODE DISPLAY_NAME "Transfer Mode"
set_parameter_property MODE UNITS None
set_parameter_property MODE AFFECTS_ELABORATION true
set_parameter_property MODE AFFECTS_GENERATION true
set_parameter_property MODE DERIVED false
set_parameter_property MODE HDL_PARAMETER true
set_parameter_property MODE ALLOWED_RANGES { "0:Memory-Mapped to Memory-Mapped" "1:Memory-Mapped to Streaming" "2:Streaming to Memory-Mapped" }

add_parameter GUI_RESPONSE_PORT INTEGER 0
set_parameter_property GUI_RESPONSE_PORT DISPLAY_NAME "Response Port"
set_parameter_property GUI_RESPONSE_PORT UNITS None
set_parameter_property GUI_RESPONSE_PORT AFFECTS_ELABORATION true
set_parameter_property GUI_RESPONSE_PORT AFFECTS_GENERATION true
set_parameter_property GUI_RESPONSE_PORT DERIVED false
set_parameter_property GUI_RESPONSE_PORT HDL_PARAMETER false
set_parameter_property GUI_RESPONSE_PORT ALLOWED_RANGES { "0:Memory-Mapped" "1:Streaming" "2:Disabled" }

# this will just be a copy of the GUI_RESPONSE_PORT setting that will be set in the validation callback
add_parameter RESPONSE_PORT INTEGER 0
set_parameter_property RESPONSE_PORT DISPLAY_NAME RESPONSE_PORT
set_parameter_property RESPONSE_PORT UNITS None
set_parameter_property RESPONSE_PORT AFFECTS_ELABORATION true
set_parameter_property RESPONSE_PORT AFFECTS_GENERATION true
set_parameter_property RESPONSE_PORT HDL_PARAMETER true
set_parameter_property RESPONSE_PORT VISIBLE false
set_parameter_property RESPONSE_PORT DERIVED true

add_parameter DESCRIPTOR_INTERFACE INTEGER 0
set_parameter_property DESCRIPTOR_INTERFACE DISPLAY_NAME DESCRIPTOR_INTERFACE
set_parameter_property DESCRIPTOR_INTERFACE UNITS None
set_parameter_property DESCRIPTOR_INTERFACE AFFECTS_ELABORATION true
set_parameter_property DESCRIPTOR_INTERFACE AFFECTS_GENERATION true
set_parameter_property DESCRIPTOR_INTERFACE HDL_PARAMETER true
set_parameter_property DESCRIPTOR_INTERFACE VISIBLE false
set_parameter_property DESCRIPTOR_INTERFACE DERIVED true
set_parameter_property DESCRIPTOR_INTERFACE ALLOWED_RANGES { "0:Avalon MM Slave" "1:Avalon ST Sink" }


add_parameter DESCRIPTOR_FIFO_DEPTH INTEGER 128
set_parameter_property DESCRIPTOR_FIFO_DEPTH DISPLAY_NAME "Descriptor FIFO Depth"
set_parameter_property DESCRIPTOR_FIFO_DEPTH UNITS None
set_parameter_property DESCRIPTOR_FIFO_DEPTH AFFECTS_ELABORATION true
set_parameter_property DESCRIPTOR_FIFO_DEPTH AFFECTS_GENERATION true
set_parameter_property DESCRIPTOR_FIFO_DEPTH DERIVED false
set_parameter_property DESCRIPTOR_FIFO_DEPTH HDL_PARAMETER true
set_parameter_property DESCRIPTOR_FIFO_DEPTH ALLOWED_RANGES { 8 16 32 64 128 256 512 1024 }

add_parameter ENHANCED_FEATURES INTEGER 1
set_parameter_property ENHANCED_FEATURES DISPLAY_NAME "Enable Extended Feature Support"
set_parameter_property ENHANCED_FEATURES UNITS None
set_parameter_property ENHANCED_FEATURES AFFECTS_ELABORATION true
set_parameter_property ENHANCED_FEATURES AFFECTS_GENERATION true
set_parameter_property ENHANCED_FEATURES DERIVED false
set_parameter_property ENHANCED_FEATURES HDL_PARAMETER true
set_parameter_property ENHANCED_FEATURES DISPLAY_HINT boolean

# this will be set according to either 16 or 32 depending on ENHANCED_FEATURES
add_parameter DESCRIPTOR_WIDTH INTEGER 256
set_parameter_property DESCRIPTOR_WIDTH DISPLAY_NAME DESCRIPTOR_WIDTH
set_parameter_property DESCRIPTOR_WIDTH UNITS None
set_parameter_property DESCRIPTOR_WIDTH AFFECTS_ELABORATION true
set_parameter_property DESCRIPTOR_WIDTH AFFECTS_GENERATION true
set_parameter_property DESCRIPTOR_WIDTH VISIBLE false
set_parameter_property DESCRIPTOR_WIDTH HDL_PARAMETER true
set_parameter_property DESCRIPTOR_WIDTH DERIVED true

# this will be set according to either 16 or 32 depending on ENHANCED_FEATURES
add_parameter DESCRIPTOR_BYTEENABLE_WIDTH INTEGER 32
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH DISPLAY_NAME DESCRIPTOR_BYTEENABLE_WIDTH
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH UNITS None
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH AFFECTS_ELABORATION true
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH AFFECTS_GENERATION true
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH HDL_PARAMETER true
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH VISIBLE false
set_parameter_property DESCRIPTOR_BYTEENABLE_WIDTH DERIVED true

# keeping this fixed to 3 even when enhanced features are turned off
add_parameter CSR_ADDRESS_WIDTH INTEGER 3
set_parameter_property CSR_ADDRESS_WIDTH DISPLAY_NAME CSR_ADDRESS_WIDTH
set_parameter_property CSR_ADDRESS_WIDTH UNITS None
set_parameter_property CSR_ADDRESS_WIDTH AFFECTS_ELABORATION true
set_parameter_property CSR_ADDRESS_WIDTH VISIBLE false
set_parameter_property CSR_ADDRESS_WIDTH DERIVED false

# following are dummy parameters that do not affect dispatcher block but just for CMacro reporting
# data width: "Width of the data path. This impacts both Avalon MM as well as Avalon ST"
# case:455171 Due to this case, some of the dummy parameters are now passed into HDL and made visible to SW through register.
add_parameter DATA_WIDTH INTEGER 32 
set_parameter_property DATA_WIDTH VISIBLE false

# Derived version of DATA_WIDTH to be used in component configuration registers
add_parameter DATA_WIDTH_DERIVED INTEGER 0 
set_parameter_property DATA_WIDTH_DERIVED VISIBLE false
set_parameter_property DATA_WIDTH_DERIVED HDL_PARAMETER true
set_parameter_property DATA_WIDTH_DERIVED AFFECTS_ELABORATION true
set_parameter_property DATA_WIDTH_DERIVED AFFECTS_GENERATION true
set_parameter_property DATA_WIDTH_DERIVED DERIVED true

# data fifo depth: "Depth of the internal data FIFO."
add_parameter DATA_FIFO_DEPTH INTEGER 32 
set_parameter_property DATA_FIFO_DEPTH VISIBLE false

# data fifo depth: "Derived version of DATA_FIFO_DEPTH to be used in component configuration registers"
add_parameter DATA_FIFO_DEPTH_DERIVED INTEGER 0 
set_parameter_property DATA_FIFO_DEPTH_DERIVED VISIBLE false
set_parameter_property DATA_FIFO_DEPTH_DERIVED HDL_PARAMETER true
set_parameter_property DATA_FIFO_DEPTH_DERIVED AFFECTS_ELABORATION true
set_parameter_property DATA_FIFO_DEPTH_DERIVED AFFECTS_GENERATION true
set_parameter_property DATA_FIFO_DEPTH_DERIVED DERIVED true

# max byte: "Maximum transfer length"
add_parameter MAX_BYTE INTEGER 1024 
set_parameter_property MAX_BYTE VISIBLE false

# max byte: "Derived version of MAX_BYTE to be used in component configuration registers"
add_parameter MAX_BYTE_DERIVED INTEGER 0 
set_parameter_property MAX_BYTE_DERIVED VISIBLE false
set_parameter_property MAX_BYTE_DERIVED HDL_PARAMETER true
set_parameter_property MAX_BYTE_DERIVED AFFECTS_ELABORATION true
set_parameter_property MAX_BYTE_DERIVED AFFECTS_GENERATION true
set_parameter_property MAX_BYTE_DERIVED DERIVED true

# transfer type: "Setting the access types will allow you to reduce the hardware footprint and increase the fmax when unnecessary features like unaligned accesses are not necessary for your system."
add_parameter TRANSFER_TYPE STRING "Aligned Accesses"
set_parameter_property TRANSFER_TYPE VISIBLE false

# transfer type: "Derived version of TRANSFER_TYPE to be used in component configuration registers"
add_parameter TRANSFER_TYPE_DERIVED INTEGER 0 
set_parameter_property TRANSFER_TYPE_DERIVED VISIBLE false
set_parameter_property TRANSFER_TYPE_DERIVED HDL_PARAMETER true
set_parameter_property TRANSFER_TYPE_DERIVED AFFECTS_ELABORATION true
set_parameter_property TRANSFER_TYPE_DERIVED AFFECTS_GENERATION true
set_parameter_property TRANSFER_TYPE_DERIVED DERIVED true

# burst enable: "Burst enable will turn on the bursting capabilities of the read master.  Bursting must not be enabled when stride is also enabled."
add_parameter BURST_ENABLE INTEGER 0 
set_parameter_property BURST_ENABLE VISIBLE false
set_parameter_property BURST_ENABLE HDL_PARAMETER true

# max burst count: "Maximum burst count."
add_parameter MAX_BURST_COUNT INTEGER 2 
set_parameter_property MAX_BURST_COUNT VISIBLE false

# Derived version of MAX_BURST_COUNT to be used in component configuration registers
add_parameter MAX_BURST_COUNT_DERIVED INTEGER 0 
set_parameter_property MAX_BURST_COUNT_DERIVED VISIBLE false
set_parameter_property MAX_BURST_COUNT_DERIVED HDL_PARAMETER true
set_parameter_property MAX_BURST_COUNT_DERIVED AFFECTS_ELABORATION true
set_parameter_property MAX_BURST_COUNT_DERIVED AFFECTS_GENERATION true
set_parameter_property MAX_BURST_COUNT_DERIVED DERIVED true

# burst wrapping support: "Enable to force the read master to align to the next burst boundary.  This setting must be enabled when the read master is connected to burst wrapping slave ports (SDRAM for example).  You cannot use this setting and programmable burst capabilities concurrently."
add_parameter BURST_WRAPPING_SUPPORT INTEGER 0 
set_parameter_property BURST_WRAPPING_SUPPORT VISIBLE false
set_parameter_property BURST_WRAPPING_SUPPORT HDL_PARAMETER true

# stride enable: "Enable stride to control the address incrementing, when off the master increments the address sequentially one word at a time.  Stride cannot be used with burst capabilities enabled."
add_parameter STRIDE_ENABLE Integer 0 
set_parameter_property STRIDE_ENABLE VISIBLE false

# stride enable: "Derived version of STRIDE_ENABLE for stride enable field in component configuration registers."
add_parameter STRIDE_ENABLE_DERIVED Integer 0 
set_parameter_property STRIDE_ENABLE_DERIVED VISIBLE false
set_parameter_property STRIDE_ENABLE_DERIVED HDL_PARAMETER true

# max stride: "Set the stride words (size of data width) for whatever maximum address increment you want to use.  A stride of 0 performs fixed accesses, 1 performs sequential, 2 reads from every other word/data size, etc..."
add_parameter MAX_STRIDE INTEGER 1  
set_parameter_property MAX_STRIDE VISIBLE false

# Derived version of MAX_STRIDE to be used in component configuration registers
add_parameter MAX_STRIDE_DERIVED INTEGER 0 
set_parameter_property MAX_STRIDE_DERIVED VISIBLE false
set_parameter_property MAX_STRIDE_DERIVED HDL_PARAMETER true
set_parameter_property MAX_STRIDE_DERIVED AFFECTS_ELABORATION true
set_parameter_property MAX_STRIDE_DERIVED AFFECTS_GENERATION true
set_parameter_property MAX_STRIDE_DERIVED DERIVED true

# programmable burst enable: "Enable re-programming of the maximum burst count.  Burst counts can only be reprogrammed between 2-128.  Make sure the maximum burst count is set large enough for the burst counts you want to re-program.  You cannot use this setting and burst alignment support concurrently."
add_parameter PROGRAMMABLE_BURST_ENABLE INTEGER 0 
set_parameter_property PROGRAMMABLE_BURST_ENABLE VISIBLE false
set_parameter_property PROGRAMMABLE_BURST_ENABLE HDL_PARAMETER true

# channel enable: "Enables channel support in data streaming interface."
add_parameter CHANNEL_ENABLE INTEGER 0 
set_parameter_property CHANNEL_ENABLE VISIBLE false
set_parameter_property CHANNEL_ENABLE HDL_PARAMETER true

# channel width: "Number of channel used in data streaming interface."
add_parameter CHANNEL_WIDTH INTEGER 8 
set_parameter_property CHANNEL_WIDTH VISIBLE false
set_parameter_property CHANNEL_WIDTH HDL_PARAMETER false

# channel width: "Derived version of CHANNEL_WIDTH to be used in component configuration registers
add_parameter CHANNEL_WIDTH_DERIVED INTEGER 0 
set_parameter_property CHANNEL_WIDTH_DERIVED VISIBLE false
set_parameter_property CHANNEL_WIDTH_DERIVED HDL_PARAMETER true
set_parameter_property CHANNEL_WIDTH_DERIVED AFFECTS_ELABORATION true
set_parameter_property CHANNEL_WIDTH_DERIVED AFFECTS_GENERATION true
set_parameter_property CHANNEL_WIDTH_DERIVED DERIVED true

# error enable: "Enables error support in data streaming interface."
add_parameter ERROR_ENABLE INTEGER 0 
set_parameter_property ERROR_ENABLE VISIBLE false
set_parameter_property ERROR_ENABLE HDL_PARAMETER true

# error width: "The number of error lines in data streaming interface."
add_parameter ERROR_WIDTH INTEGER 8 
set_parameter_property ERROR_WIDTH VISIBLE false
set_parameter_property ERROR_WIDTH HDL_PARAMETER false

# Derived version of ERROR_WIDTH to be used in component configuration registers
add_parameter ERROR_WIDTH_DERIVED INTEGER 0 
set_parameter_property ERROR_WIDTH_DERIVED VISIBLE false
set_parameter_property ERROR_WIDTH_DERIVED HDL_PARAMETER true
set_parameter_property ERROR_WIDTH_DERIVED AFFECTS_ELABORATION true
set_parameter_property ERROR_WIDTH_DERIVED AFFECTS_GENERATION true
set_parameter_property ERROR_WIDTH_DERIVED DERIVED true

# packet enable: "Enables packetized transfer."
add_parameter PACKET_ENABLE INTEGER 0 
set_parameter_property PACKET_ENABLE VISIBLE false
set_parameter_property PACKET_ENABLE HDL_PARAMETER true

# Derived version of DESCRIPTOR_FIFO_DEPTH to be used in component configuration registers
add_parameter DESCRIPTOR_FIFO_DEPTH_DERIVED INTEGER 0
set_parameter_property DESCRIPTOR_FIFO_DEPTH_DERIVED VISIBLE false
set_parameter_property DESCRIPTOR_FIFO_DEPTH_DERIVED HDL_PARAMETER true
set_parameter_property DESCRIPTOR_FIFO_DEPTH_DERIVED AFFECTS_ELABORATION true
set_parameter_property DESCRIPTOR_FIFO_DEPTH_DERIVED AFFECTS_GENERATION true
set_parameter_property DESCRIPTOR_FIFO_DEPTH_DERIVED DERIVED true

# | 
# +-----------------------------------

# +-----------------------------------
# | connection point clock
# | 
add_interface clock clock end
set_interface_property clock ptfSchematicName ""

add_interface_port clock clk clk Input 1
add_interface_port clock reset reset Input 1
set_interface_property clock_reset synchronousEdges BOTH
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point csr
# | 
add_interface CSR avalon end
set_interface_property CSR addressAlignment DYNAMIC
set_interface_property CSR addressSpan 4
set_interface_property CSR bridgesToMaster ""
set_interface_property CSR burstOnBurstBoundariesOnly false
set_interface_property CSR holdTime 0
set_interface_property CSR isMemoryDevice false
set_interface_property CSR isNonVolatileStorage false
set_interface_property CSR linewrapBursts false
set_interface_property CSR maximumPendingReadTransactions 0
set_interface_property CSR minimumUninterruptedRunLength 1
set_interface_property CSR printableDevice false
set_interface_property CSR readLatency 1
set_interface_property CSR readWaitTime 1
set_interface_property CSR setupTime 0
set_interface_property CSR timingUnits Cycles
set_interface_property CSR writeWaitTime 0

set_interface_property CSR ASSOCIATED_CLOCK clock

add_interface_port CSR csr_writedata writedata Input 32
add_interface_port CSR csr_write write Input 1
add_interface_port CSR csr_byteenable byteenable Input 4
add_interface_port CSR csr_readdata readdata Output 32
add_interface_port CSR csr_read read Input 1
add_interface_port CSR csr_address address Input 3

# | 
# +-----------------------------------

# +-----------------------------------
# | connection point st_response
# | 
add_interface Response_Source avalon_streaming start
set_interface_property Response_Source dataBitsPerSymbol 256
set_interface_property Response_Source errorDescriptor ""
set_interface_property Response_Source maxChannel 0
set_interface_property Response_Source readyLatency 0
set_interface_property Response_Source symbolsPerBeat 1

set_interface_property Response_Source ASSOCIATED_CLOCK clock

add_interface_port Response_Source src_response_data data Output 256
add_interface_port Response_Source src_response_valid valid Output 1
add_interface_port Response_Source src_response_ready ready Input 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point descriptor
# | 
add_interface Descriptor_Slave avalon end
set_interface_property Descriptor_Slave addressAlignment DYNAMIC
set_interface_property Descriptor_Slave bridgesToMaster ""
set_interface_property Descriptor_Slave burstOnBurstBoundariesOnly false
set_interface_property Descriptor_Slave holdTime 0
set_interface_property Descriptor_Slave isMemoryDevice false
set_interface_property Descriptor_Slave isNonVolatileStorage false
set_interface_property Descriptor_Slave linewrapBursts false
set_interface_property Descriptor_Slave maximumPendingReadTransactions 0
set_interface_property Descriptor_Slave minimumUninterruptedRunLength 1
set_interface_property Descriptor_Slave printableDevice false
set_interface_property Descriptor_Slave readLatency 0
set_interface_property Descriptor_Slave readWaitTime 1
set_interface_property Descriptor_Slave setupTime 0
set_interface_property Descriptor_Slave timingUnits Cycles
set_interface_property Descriptor_Slave writeWaitTime 0

set_interface_property Descriptor_Slave ASSOCIATED_CLOCK clock


add_interface_port Descriptor_Slave descriptor_write write Input 1
add_interface_port Descriptor_Slave descriptor_waitrequest waitrequest Output 1
add_interface_port Descriptor_Slave descriptor_writedata writedata Input 256
add_interface_port Descriptor_Slave descriptor_byteenable byteenable Input 32

# +-----------------------------------
# | connection point Descriptor_Sink
# | 
add_interface Descriptor_Sink avalon_streaming end
set_interface_property Descriptor_Sink dataBitsPerSymbol 256
set_interface_property Descriptor_Sink errorDescriptor ""
set_interface_property Descriptor_Sink maxChannel 0
set_interface_property Descriptor_Sink readyLatency 0
set_interface_property Descriptor_Sink symbolsPerBeat 1

set_interface_property Descriptor_Sink ASSOCIATED_CLOCK clock

add_interface_port Descriptor_Sink snk_descriptor_data data Input 256
add_interface_port Descriptor_Sink snk_descriptor_valid valid Input 1
add_interface_port Descriptor_Sink snk_descriptor_ready ready Output 1

# | 
# +-----------------------------------

# +-----------------------------------
# | connection point mm_response
# | 
add_interface Response_Slave avalon end
set_interface_property Response_Slave addressAlignment DYNAMIC
set_interface_property Response_Slave addressSpan 8
set_interface_property Response_Slave bridgesToMaster ""
set_interface_property Response_Slave burstOnBurstBoundariesOnly false
set_interface_property Response_Slave holdTime 0
set_interface_property Response_Slave isMemoryDevice false
set_interface_property Response_Slave isNonVolatileStorage false
set_interface_property Response_Slave linewrapBursts false
set_interface_property Response_Slave maximumPendingReadTransactions 0
set_interface_property Response_Slave minimumUninterruptedRunLength 1
set_interface_property Response_Slave printableDevice false
set_interface_property Response_Slave readLatency 0
set_interface_property Response_Slave readWaitTime 1
set_interface_property Response_Slave setupTime 0
set_interface_property Response_Slave timingUnits Cycles
set_interface_property Response_Slave writeWaitTime 0

set_interface_property Response_Slave ASSOCIATED_CLOCK clock

add_interface_port Response_Slave mm_response_waitrequest waitrequest Output 1
add_interface_port Response_Slave mm_response_byteenable byteenable Input 4
add_interface_port Response_Slave mm_response_address address Input 1
add_interface_port Response_Slave mm_response_readdata readdata Output 32
add_interface_port Response_Slave mm_response_read read Input 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point write_master_descriptor
# | 
add_interface Write_Command_Source avalon_streaming start
set_interface_property Write_Command_Source dataBitsPerSymbol 256
set_interface_property Write_Command_Source errorDescriptor ""
set_interface_property Write_Command_Source maxChannel 0
set_interface_property Write_Command_Source readyLatency 0
set_interface_property Write_Command_Source symbolsPerBeat 1

set_interface_property Write_Command_Source ASSOCIATED_CLOCK clock

add_interface_port Write_Command_Source src_write_master_data data Output 256
add_interface_port Write_Command_Source src_write_master_valid valid Output 1
# | 
add_interface_port Write_Command_Source src_write_master_ready ready Input 1
# +-----------------------------------

# +-----------------------------------
# | connection point write_master_response
# | 
add_interface Write_Response_Sink avalon_streaming end
set_interface_property Write_Response_Sink dataBitsPerSymbol 256
set_interface_property Write_Response_Sink errorDescriptor ""
set_interface_property Write_Response_Sink maxChannel 0
set_interface_property Write_Response_Sink readyLatency 0
set_interface_property Write_Response_Sink symbolsPerBeat 1

set_interface_property Write_Response_Sink ASSOCIATED_CLOCK clock

add_interface_port Write_Response_Sink snk_write_master_data data Input 256
add_interface_port Write_Response_Sink snk_write_master_valid valid Input 1
add_interface_port Write_Response_Sink snk_write_master_ready ready Output 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point read_master_descriptor
# | 
add_interface Read_Command_Source avalon_streaming start
set_interface_property Read_Command_Source dataBitsPerSymbol 256
set_interface_property Read_Command_Source errorDescriptor ""
set_interface_property Read_Command_Source maxChannel 0
set_interface_property Read_Command_Source readyLatency 0
set_interface_property Read_Command_Source symbolsPerBeat 1

set_interface_property Read_Command_Source ASSOCIATED_CLOCK clock

add_interface_port Read_Command_Source src_read_master_data data Output 256
add_interface_port Read_Command_Source src_read_master_valid valid Output 1
add_interface_port Read_Command_Source src_read_master_ready ready Input 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point read_master_response
# | 
add_interface Read_Response_Sink avalon_streaming end
set_interface_property Read_Response_Sink dataBitsPerSymbol 256
set_interface_property Read_Response_Sink errorDescriptor ""
set_interface_property Read_Response_Sink maxChannel 0
set_interface_property Read_Response_Sink readyLatency 0
set_interface_property Read_Response_Sink symbolsPerBeat 1

set_interface_property Read_Response_Sink ASSOCIATED_CLOCK clock

add_interface_port Read_Response_Sink snk_read_master_data data Input 256
add_interface_port Read_Response_Sink snk_read_master_valid valid Input 1
add_interface_port Read_Response_Sink snk_read_master_ready ready Output 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point csr_irq
# | 
add_interface csr_irq interrupt end
set_interface_property csr_irq associatedAddressablePoint CSR

set_interface_property csr_irq ASSOCIATED_CLOCK clock

add_interface_port csr_irq csr_irq irq Output 1
# | 
# +-----------------------------------


# need to set address and byte enable widths depending on parameterization.  Also need to terminate interfaces depending
# on the parameterization.
proc elaborate_me {}  { 
  set the_MODE [get_parameter_value MODE]
  set the_RESPONSE_PORT [get_parameter_value RESPONSE_PORT]

  if { [get_parameter_value PREFETCHER_USE_CASE] == 1 } {
      set_interface_property Descriptor_Slave ENABLED false
      set_interface_property Descriptor_Sink ENABLED true
  } else {
      set_interface_property Descriptor_Slave ENABLED true
      set_interface_property Descriptor_Sink ENABLED false
  }

  switch $the_MODE { 
    0 { 
      ## MM to MM
      set_interface_property Write_Command_Source ENABLED true
      set_interface_property Write_Response_Sink ENABLED true
      set_interface_property Read_Command_Source ENABLED true
      set_interface_property Read_Response_Sink ENABLED true
    } 
    1 { 
      ## MM to ST (remove the write master interfaces and hardcode write port inputs to ground)
      set_interface_property Write_Command_Source ENABLED false
      set_interface_property Write_Response_Sink ENABLED false
      set_interface_property Read_Command_Source ENABLED true
      set_interface_property Read_Response_Sink ENABLED true      
    } 
    default { 
      ## ST to MM (remove the read master interfaces and hardcode read port inputs to ground)
      set_interface_property Write_Command_Source ENABLED true
      set_interface_property Write_Response_Sink ENABLED true
      set_interface_property Read_Command_Source ENABLED false
      set_interface_property Read_Response_Sink ENABLED false
    } 
  }
  
  switch $the_RESPONSE_PORT {
    0 {
      ## MM slave port for response information
      set_interface_property Response_Slave ENABLED true
      set_interface_property Response_Source ENABLED false
      set_interface_property csr_irq ENABLED true    
    }
    1 {
      ## ST source port for response data (note the interrupt port is removed and interrupt info is sent through the response port instead)
      set_interface_property Response_Slave ENABLED false
      set_interface_property Response_Source ENABLED true
      set_interface_property csr_irq ENABLED false   
    }
    default {
      ## No response port
      set_interface_property Response_Slave ENABLED false
      set_interface_property Response_Source ENABLED false
      set_interface_property csr_irq ENABLED true     
    }
  }
  
  # set_port_property descriptor_writedata WIDTH [get_parameter_value DESCRIPTOR_WIDTH]
  set_port_property descriptor_writedata WIDTH_EXPR [get_parameter_value DESCRIPTOR_WIDTH]
  set_interface_property Descriptor_Sink dataBitsPerSymbol [get_parameter_value DESCRIPTOR_WIDTH]
  set_port_property snk_descriptor_data WIDTH_EXPR [get_parameter_value DESCRIPTOR_WIDTH]
  # set_port_property descriptor_byteenable WIDTH [get_parameter_value DESCRIPTOR_BYTEENABLE_WIDTH]
  set_port_property descriptor_byteenable WIDTH_EXPR [get_parameter_value DESCRIPTOR_BYTEENABLE_WIDTH]
  
  set_module_assignment "embeddedsw.CMacro.DESCRIPTOR_FIFO_DEPTH" [get_parameter_value DESCRIPTOR_FIFO_DEPTH]
  set_module_assignment "embeddedsw.CMacro.RESPONSE_FIFO_DEPTH" [expr 2 * [get_parameter_value DESCRIPTOR_FIFO_DEPTH]]
  
  set_module_assignment "embeddedsw.CMacro.DATA_WIDTH" [get_parameter_value DATA_WIDTH]
  set_module_assignment "embeddedsw.CMacro.DATA_FIFO_DEPTH" [get_parameter_value DATA_FIFO_DEPTH]
  set_module_assignment "embeddedsw.CMacro.MAX_BYTE" [get_parameter_value MAX_BYTE]
  set_module_assignment "embeddedsw.CMacro.TRANSFER_TYPE" [get_parameter_value TRANSFER_TYPE]
  set_module_assignment "embeddedsw.CMacro.BURST_ENABLE" [get_parameter_value BURST_ENABLE]
  set_module_assignment "embeddedsw.CMacro.MAX_BURST_COUNT" [get_parameter_value MAX_BURST_COUNT]
  set_module_assignment "embeddedsw.CMacro.BURST_WRAPPING_SUPPORT" [get_parameter_value BURST_WRAPPING_SUPPORT]
  set_module_assignment "embeddedsw.CMacro.STRIDE_ENABLE" [get_parameter_value STRIDE_ENABLE]
  set_module_assignment "embeddedsw.CMacro.MAX_STRIDE" [get_parameter_value MAX_STRIDE]
  set_module_assignment "embeddedsw.CMacro.PROGRAMMABLE_BURST_ENABLE" [get_parameter_value PROGRAMMABLE_BURST_ENABLE]
  set_module_assignment "embeddedsw.CMacro.ENHANCED_FEATURES" [get_parameter_value ENHANCED_FEATURES]
  set_module_assignment "embeddedsw.CMacro.RESPONSE_PORT" [get_parameter_value GUI_RESPONSE_PORT]
} 


# issue out messages to the user about the response port
proc validate_me {} { 
  set the_MODE [get_parameter_value MODE]
  set the_GUI_RESPONSE_PORT [get_parameter_value GUI_RESPONSE_PORT]
  set the_ENHANCED_FEATURES [get_parameter_value ENHANCED_FEATURES]
  set the_PREFETCHER_USE_CASE [get_parameter_value PREFETCHER_USE_CASE]
  set the_MAX_BYTE [get_parameter_value MAX_BYTE]
  set the_TRANSFER_TYPE [get_parameter_value TRANSFER_TYPE]
  set the_CHANNEL_WIDTH [get_parameter_value CHANNEL_WIDTH]
  set the_DATA_FIFO_DEPTH [get_parameter_value DATA_FIFO_DEPTH]
  set the_DATA_WIDTH [get_parameter_value DATA_WIDTH]
  set the_DESCRIPTOR_FIFO_DEPTH [get_parameter_value DESCRIPTOR_FIFO_DEPTH]
  set the_ERROR_WIDTH [get_parameter_value ERROR_WIDTH]
  set the_MAX_BURST_COUNT [get_parameter_value MAX_BURST_COUNT]
  set the_MAX_STRIDE [get_parameter_value MAX_STRIDE]
  
  if { $the_ENHANCED_FEATURES == 1 } { 
    set_parameter_value DESCRIPTOR_WIDTH 256
    set_parameter_value DESCRIPTOR_BYTEENABLE_WIDTH 32
  } else { 
    set_parameter_value DESCRIPTOR_WIDTH 128
    set_parameter_value DESCRIPTOR_BYTEENABLE_WIDTH 16
  }
  
  # Encode MAX_BYTE_DERIVED into 5 bits based on MAX_BYTE definition from mSGDMA top hw.tcl 
  # {"1024:1KB" "2048:2KB" "4096:4KB" "8192:8KB" "16384:16KB" "32768:32KB" "65536:64KB" "131072:128KB" "262144:256KB" "524288:512KB" "1048576:1MB" "2097152:2MB" "4194304:4MB" "8388608:8MB" "16777216:16MB" "33554432:32MB" "67108864:64MB" "134217728:128MB" "368435456:256MB" "536870912:512MB" "1073741824:1GB" "2147483647:2GB"}
  switch $the_MAX_BYTE {
    1024        { set_parameter_value MAX_BYTE_DERIVED 0 }
    2048        { set_parameter_value MAX_BYTE_DERIVED 1 }
    4096        { set_parameter_value MAX_BYTE_DERIVED 2 }
    8192        { set_parameter_value MAX_BYTE_DERIVED 3 }
    16384       { set_parameter_value MAX_BYTE_DERIVED 4 }
    32768       { set_parameter_value MAX_BYTE_DERIVED 5 }
    65536       { set_parameter_value MAX_BYTE_DERIVED 6 }
    131072      { set_parameter_value MAX_BYTE_DERIVED 7 }
    262144      { set_parameter_value MAX_BYTE_DERIVED 8 }
    524288      { set_parameter_value MAX_BYTE_DERIVED 9 }
    1048576     { set_parameter_value MAX_BYTE_DERIVED 10 }
    2097152     { set_parameter_value MAX_BYTE_DERIVED 11 }
    4194304     { set_parameter_value MAX_BYTE_DERIVED 12 }
    8388608     { set_parameter_value MAX_BYTE_DERIVED 13 }
    16777216    { set_parameter_value MAX_BYTE_DERIVED 14 }
    33554432    { set_parameter_value MAX_BYTE_DERIVED 15 }
    67108864    { set_parameter_value MAX_BYTE_DERIVED 16 }
    134217728   { set_parameter_value MAX_BYTE_DERIVED 17 }
    368435456   { set_parameter_value MAX_BYTE_DERIVED 18 }
    536870912   { set_parameter_value MAX_BYTE_DERIVED 19 }
    1073741824  { set_parameter_value MAX_BYTE_DERIVED 20 }
    2147483647  { set_parameter_value MAX_BYTE_DERIVED 21 }
  }
  
  # Encode TRANSFER_TYPE_DERIVED into 2 bits based on TRANSFER_TYPE definition from mSGDMA top hw.tcl
  # { "Full Word Accesses Only" "Aligned Accesses" "Unaligned Accesses" }
  if { $the_TRANSFER_TYPE == "Full Word Accesses Only" } {
    set_parameter_value TRANSFER_TYPE_DERIVED 0
  } elseif { $the_TRANSFER_TYPE == "Aligned Accesses" } {
    set_parameter_value TRANSFER_TYPE_DERIVED 1
  } else {
    set_parameter_value TRANSFER_TYPE_DERIVED 2
  }
  
  # Encode CHANNEL_WIDTH_DERIVED into 3 bits based on CHANNEL_WIDTH definition from mSGDMA top hw.tcl
  # {1 2 3 4 5 6 7 8}
  set_parameter_value CHANNEL_WIDTH_DERIVED [ expr $the_CHANNEL_WIDTH - 1 ]

  # Encode DATA_FIFO_DEPTH_DERIVED into 4 bits based on DATA_FIFO_DEPTH definition from mSGDMA top hw.tcl
  # {16 32 64 128 256 512 1024 2048 4096}
  set_parameter_value DATA_FIFO_DEPTH_DERIVED [ expr [ log2ceil $the_DATA_FIFO_DEPTH ] - 4 ]

  # Encode DATA_WIDTH_DERIVED into 3 bits based on DATA_WIDTH definition from mSGDMA top hw.tcl
  # {8 16 32 64 128 256 512 1024}
  set_parameter_value DATA_WIDTH_DERIVED [ expr [ log2ceil $the_DATA_WIDTH ] - 3 ]

  # Encode DESCRIPTOR_FIFO_DEPTH_DERIVED into 3 bits based on DESCRIPTOR_FIFO_DEPTH definition from mSGDMA top hw.tcl
  # {8 16 32 64 128 256 512 1024}
  set_parameter_value DESCRIPTOR_FIFO_DEPTH_DERIVED [ expr [ log2ceil $the_DESCRIPTOR_FIFO_DEPTH ] - 3 ]

  # Encode ERROR_WIDTH_DERIVED into 3 bits based on ERROR_WIDTH definition from mSGDMA top hw.tcl
  # {1 2 3 4 5 6 7 8}
  set_parameter_value ERROR_WIDTH_DERIVED [ expr $the_ERROR_WIDTH - 1 ]
  
  # Encode MAX_BURST_COUNT_DERIVED into 4 bits based on MAX_BURST_COUNT definition from mSGDMA top hw.tcl
  # {2 4 8 16 32 64 128 256 512 1024}
  set_parameter_value MAX_BURST_COUNT_DERIVED [ expr [ log2ceil $the_MAX_BURST_COUNT ] - 1 ]
  
  # Encode MAX_STRIDE_DERIVED into 15 bits based on MAX_STRIDE definition from mSGDMA top hw.tcl
  # 1:32768
  set_parameter_value MAX_STRIDE_DERIVED [ expr $the_MAX_STRIDE - 1 ]

  if { $the_PREFETCHER_USE_CASE == 1 } {
    ## Response port is automatically set to ST when pre-fetching use cases is enabled.
    ## Descriptor interface is automatically set as ST sink when pre-fetching use cases is enabled
    set_parameter_property GUI_RESPONSE_PORT ENABLED false
    set_parameter_value RESPONSE_PORT 1
    set_parameter_value DESCRIPTOR_INTERFACE 1
  } else {
    set_parameter_value DESCRIPTOR_INTERFACE 0
    ## MM to ST so disable the response port and grey out the pull down box
    if { $the_MODE == 1 }  {
        set_parameter_property GUI_RESPONSE_PORT ENABLED false
        set_parameter_value RESPONSE_PORT 2
    } else {
        set_parameter_property GUI_RESPONSE_PORT ENABLED true
        set_parameter_value RESPONSE_PORT $the_GUI_RESPONSE_PORT
    }
  }

  ## outputing warnings and hints (note:  response information will be allowed for MM->MM transfers since users might put some streaming blocks between the masters that have error support)
  
  #if { $the_GUI_RESPONSE_PORT == 0 } {
  #  if { $the_MODE == 0 } {
  #    send_message Info "For MM to MM transfers you typically do not need a MM response port"
  #  }
  #} elseif { $the_GUI_RESPONSE_PORT == 1 } {
  #  send_message Info "Interrupts must be handled by the component connected to the response source port (such as a descriptor pre-fetching block)"
  #}
  
  # Use $the_RESPONSE_PORT instead of $the_GUI_RESPONSE_PORT
  # This is because $the_GUI_RESPONSE_PORT value can be invalid/grey out during certain configuration (example: when MODE = 1 or when pre-fetching use case = 1), thus
  # causes invalid warnings being fired
  
  set the_RESPONSE_PORT [get_parameter_value RESPONSE_PORT]
  
  if { $the_RESPONSE_PORT == 0 } {
    if { $the_MODE == 0 } {
      send_message Info "For MM to MM transfers you typically do not need a MM response port"
    }
  } elseif { $the_RESPONSE_PORT == 1 } {
    send_message Info "Interrupts must be handled by the component connected to the response source port (such as a descriptor pre-fetching block)"
  }
  
}

# +-----------------------------------
# | Procedure being used
# | 
proc log2ceil {num} {
    set val 0
    set i 1
    while { $i < $num } {
        set val [ expr $val + 1 ]
        set i [ expr 1 << $val ]
    }
    return $val;
}
# | 
# +----------------------------------- 


## Add documentation links for user guide and/or release notes
add_documentation_link "User Guide" https://documentation.altera.com/#/link/sfo1400787952932/lro1402196946061
add_documentation_link "Release Notes" https://documentation.altera.com/#/link/hco1421698042087/hco1421698013408
