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


# package require -exact sopc 9.1
package require -exact qsys 13.1

# *********************************************************************************
# JCJB:  This is a duplicate of mSGDMA write master with enhancements and bug fixes
#        added for streaming.  In order to avoid namespace collisions with the  
#        mSGDMA in the ACDS all the modules have been renamed.
#
#        The following enhancements have been added:
#
#        1)  New feature for hardcoding byte enables high for aligned and unaligned
#            transfers.  This causes write sideeffects but hits a higher Fmax.
#
#        2)  AvST sink only accepts data when descriptor is live.  The write master
#            will no longer accept streaming data while in reset or not actively 
#            working on a transfer.
#
#        3)  Bug fix to capture early termination correctly so that it is passed
#            to dispatcher.  When the early termination event occured this status
#            was lost by the time the dispatcher tried to capture it.
# *********************************************************************************


# +-----------------------------------
# | module Write_Master
# | 
set_module_property DESCRIPTION "A module responsible for writing streaming data to memory"
set_module_property NAME dcp_dma_write_master
set_module_property VERSION 17.0
set_module_property INTERNAL false
set_module_property HIDE_FROM_QUARTUS true
#set_module_property GROUP "DMA/mSGDMA Sub-core"
#set_module_property GROUP "Basic Functions/DMA/mSGDMA Sub-core"
set_module_property AUTHOR "Intel Corporation"
set_module_property DISPLAY_NAME "DCP mSGDMA Write Master"
# set_module_property TOP_LEVEL_HDL_FILE write_master.v
# set_module_property TOP_LEVEL_HDL_MODULE write_master
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE false
# set_module_property DATASHEET_URL "file:/[get_module_property MODULE_DIRECTORY]Modular_SGDMA_Write_Master_Core_UG.pdf"

# Device tree fields
#set_module_assignment embeddedsw.dts.vendor "ALTR"
#set_module_assignment embeddedsw.dts.group "msgdma"
#set_module_assignment embeddedsw.dts.name "msgdma-write-master"
#set_module_assignment embeddedsw.dts.compatible "ALTR,msgdma-write-master-1.0"
# | 
# +-----------------------------------

# +-----------------------------------
# | files
# | 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL dcp_write_master
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_write_master.v VERILOG PATH dcp_write_master.v TOP_LEVEL_FILE
add_fileset_file dcp_byte_enable_generator.v VERILOG PATH dcp_byte_enable_generator.v 
add_fileset_file dcp_ST_to_MM_Adapter.v VERILOG PATH dcp_ST_to_MM_Adapter.v 
add_fileset_file dcp_write_burst_control.v VERILOG PATH dcp_write_burst_control.v 

add_fileset SIM_VERILOG SIM_VERILOG "" ""
set_fileset_property SIM_VERILOG TOP_LEVEL dcp_write_master
set_fileset_property SIM_VERILOG ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_write_master.v VERILOG PATH dcp_write_master.v
add_fileset_file dcp_byte_enable_generator.v VERILOG PATH dcp_byte_enable_generator.v 
add_fileset_file dcp_ST_to_MM_Adapter.v VERILOG PATH dcp_ST_to_MM_Adapter.v 
add_fileset_file dcp_write_burst_control.v VERILOG PATH dcp_write_burst_control.v 

add_fileset SIM_VHDL SIM_VHDL "" ""
set_fileset_property SIM_VHDL TOP_LEVEL dcp_write_master
set_fileset_property SIM_VHDL ENABLE_RELATIVE_INCLUDE_PATHS false
add_fileset_file dcp_write_master.v VERILOG PATH dcp_write_master.v
add_fileset_file dcp_byte_enable_generator.v VERILOG PATH dcp_byte_enable_generator.v 
add_fileset_file dcp_ST_to_MM_Adapter.v VERILOG PATH dcp_ST_to_MM_Adapter.v 
add_fileset_file dcp_write_burst_control.v VERILOG PATH dcp_write_burst_control.v 

# | 
# +-----------------------------------



set_module_property ELABORATION_CALLBACK    elaborate_me
set_module_property VALIDATION_CALLBACK     validate_me



# +-----------------------------------
# | parameters
# | 
add_parameter DATA_WIDTH INTEGER 32 "Width of the streaming and memory mapped data path."
set_parameter_property DATA_WIDTH ALLOWED_RANGES {8 16 32 64 128 256 512 1024}
set_parameter_property DATA_WIDTH DISPLAY_NAME "Data Width"
set_parameter_property DATA_WIDTH AFFECTS_GENERATION false
set_parameter_property DATA_WIDTH DERIVED false
set_parameter_property DATA_WIDTH HDL_PARAMETER true
set_parameter_property DATA_WIDTH AFFECTS_ELABORATION true
add_display_item "Transfer Options" DATA_WIDTH parameter

add_parameter GUI_NO_BYTEENABLES INTEGER 0 "Enable to force byte enables to always high."
set_parameter_property GUI_NO_BYTEENABLES DISPLAY_NAME "No Byteenables"
set_parameter_property GUI_NO_BYTEENABLES DISPLAY_HINT boolean
set_parameter_property GUI_NO_BYTEENABLES AFFECTS_GENERATION false
set_parameter_property GUI_NO_BYTEENABLES DERIVED false
set_parameter_property GUI_NO_BYTEENABLES HDL_PARAMETER false
set_parameter_property GUI_NO_BYTEENABLES AFFECTS_ELABORATION false
add_display_item "Transfer Options" GUI_NO_BYTEENABLES parameter

add_parameter LENGTH_WIDTH INTEGER 32 "Width of the length register, reduce this to increase Fmax and the logic footprint."
set_parameter_property LENGTH_WIDTH ALLOWED_RANGES 3:32
set_parameter_property LENGTH_WIDTH DISPLAY_NAME "Length Width"
set_parameter_property LENGTH_WIDTH AFFECTS_GENERATION false
set_parameter_property LENGTH_WIDTH DERIVED false
set_parameter_property LENGTH_WIDTH HDL_PARAMETER true
set_parameter_property LENGTH_WIDTH AFFECTS_ELABORATION false
add_display_item "Transfer Options" LENGTH_WIDTH parameter

add_parameter FIFO_DEPTH INTEGER 32 "Depth of the internal data FIFO."
set_parameter_property FIFO_DEPTH ALLOWED_RANGES {16 32 64 128 256 512 1024 2048 4096}
set_parameter_property FIFO_DEPTH DISPLAY_NAME "FIFO Depth"
set_parameter_property FIFO_DEPTH DERIVED false
set_parameter_property FIFO_DEPTH AFFECTS_GENERATION false
set_parameter_property FIFO_DEPTH HDL_PARAMETER true
set_parameter_property FIFO_DEPTH AFFECTS_ELABORATION false
add_display_item "Transfer Options" FIFO_DEPTH parameter

add_parameter USE_FIX_ADDRESS_WIDTH Integer 0 "Use pre-determined master address width instead of automatically-determined master address width"
set_parameter_property USE_FIX_ADDRESS_WIDTH DISPLAY_NAME "Use pre-determined master address width"
set_parameter_property USE_FIX_ADDRESS_WIDTH DISPLAY_HINT boolean
set_parameter_property USE_FIX_ADDRESS_WIDTH AFFECTS_GENERATION false
set_parameter_property USE_FIX_ADDRESS_WIDTH DERIVED false
set_parameter_property USE_FIX_ADDRESS_WIDTH HDL_PARAMETER false
set_parameter_property USE_FIX_ADDRESS_WIDTH AFFECTS_ELABORATION true
add_display_item "Transfer Options" USE_FIX_ADDRESS_WIDTH parameter

add_parameter FIX_ADDRESS_WIDTH INTEGER 32 "Minimum master address width that is required to address memory slave."
set_parameter_property FIX_ADDRESS_WIDTH DISPLAY_NAME "Pre-determined master address width:"
set_parameter_property FIX_ADDRESS_WIDTH ALLOWED_RANGES 1:64
set_parameter_property FIX_ADDRESS_WIDTH AFFECTS_GENERATION true
set_parameter_property FIX_ADDRESS_WIDTH DERIVED false
set_parameter_property FIX_ADDRESS_WIDTH HDL_PARAMETER false
set_parameter_property FIX_ADDRESS_WIDTH AFFECTS_ELABORATION true
set_parameter_property FIX_ADDRESS_WIDTH VISIBLE true
add_display_item "Transfer Options" FIX_ADDRESS_WIDTH parameter

add_parameter STRIDE_ENABLE Integer 0 "Enable stride to control the address incrementing, when off the master increments the address sequentially one word at a time.  Stride cannot be used with burst capabilities enabled."
set_parameter_property STRIDE_ENABLE DISPLAY_NAME "Stride Addressing Enable"
set_parameter_property STRIDE_ENABLE DISPLAY_HINT boolean
set_parameter_property STRIDE_ENABLE AFFECTS_GENERATION false
set_parameter_property STRIDE_ENABLE DERIVED false
set_parameter_property STRIDE_ENABLE HDL_PARAMETER true
set_parameter_property STRIDE_ENABLE AFFECTS_ELABORATION false
add_display_item "Transfer Options" STRIDE_ENABLE parameter

add_parameter GUI_STRIDE_WIDTH INTEGER 1  "Set the stride width for whatever maximum address increment you want to use.  A stride of 0 performs fixed accesses, 1 performs sequential, 2 reads from every other word, etc..."
set_parameter_property GUI_STRIDE_WIDTH ALLOWED_RANGES {1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16}
set_parameter_property GUI_STRIDE_WIDTH DISPLAY_NAME "Stride Width (words)"
set_parameter_property GUI_STRIDE_WIDTH AFFECTS_GENERATION false
set_parameter_property GUI_STRIDE_WIDTH DERIVED false
set_parameter_property GUI_STRIDE_WIDTH HDL_PARAMETER false
set_parameter_property GUI_STRIDE_WIDTH AFFECTS_ELABORATION false
add_display_item "Transfer Options" GUI_STRIDE_WIDTH parameter

add_parameter BURST_ENABLE INTEGER 0 "Burst enable will turn on the bursting capabilities of the read master.  Bursting must not be enabled when stride is also enabled." 
set_parameter_property BURST_ENABLE DISPLAY_NAME "Burst Enable"
set_parameter_property BURST_ENABLE DISPLAY_HINT boolean
set_parameter_property BURST_ENABLE AFFECTS_GENERATION false
set_parameter_property BURST_ENABLE HDL_PARAMETER true
set_parameter_property BURST_ENABLE DERIVED false
set_parameter_property BURST_ENABLE AFFECTS_ELABORATION true
add_display_item "Transfer Options" BURST_ENABLE parameter

add_parameter GUI_MAX_BURST_COUNT INTEGER 2 "Maximum burst count."
set_parameter_property GUI_MAX_BURST_COUNT ALLOWED_RANGES {2 4 8 16 32 64 128 256 512 1024}
set_parameter_property GUI_MAX_BURST_COUNT DISPLAY_NAME "Maximum Burst Count"
set_parameter_property GUI_MAX_BURST_COUNT AFFECTS_GENERATION false
set_parameter_property GUI_MAX_BURST_COUNT HDL_PARAMETER false
set_parameter_property GUI_MAX_BURST_COUNT DERIVED false
set_parameter_property GUI_MAX_BURST_COUNT AFFECTS_ELABORATION false
add_display_item "Transfer Options" GUI_MAX_BURST_COUNT parameter

add_parameter GUI_PROGRAMMABLE_BURST_ENABLE INTEGER 0 "Enable re-programming of the maximum burst count.  Burst counts can only be reprogrammed between 2-128.  Make sure the maximum burst count is set large enough for the burst counts you want to re-program.  You cannot use this setting and burst alignment support concurrently."
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE DISPLAY_NAME "Programmable Burst Enable"
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE DISPLAY_HINT boolean
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE AFFECTS_GENERATION false
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE HDL_PARAMETER false
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE DERIVED false
set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE AFFECTS_ELABORATION false
add_display_item "Transfer Options" GUI_PROGRAMMABLE_BURST_ENABLE parameter

add_parameter GUI_BURST_WRAPPING_SUPPORT INTEGER 0 "Enable to force the read master to align to the next burst boundary.  This setting must be enabled when the read master is connected to burst wrapping slave ports (SDRAM for example).  You cannot use this setting and programmable burst capabilities concurrently."
set_parameter_property GUI_BURST_WRAPPING_SUPPORT DISPLAY_NAME "Force Burst Alignment Enable"
set_parameter_property GUI_BURST_WRAPPING_SUPPORT DISPLAY_HINT boolean
set_parameter_property GUI_BURST_WRAPPING_SUPPORT AFFECTS_GENERATION false
set_parameter_property GUI_BURST_WRAPPING_SUPPORT HDL_PARAMETER false
set_parameter_property GUI_BURST_WRAPPING_SUPPORT DERIVED false
set_parameter_property GUI_BURST_WRAPPING_SUPPORT AFFECTS_ELABORATION false
add_display_item "Transfer Options" GUI_BURST_WRAPPING_SUPPORT parameter





# this parameter will be displayed instead of "UNALIGNED_ACCESS_ENABLE" and "ONLY_FULL_ACCESS_ENABLE".  It will be used to control the unaligned and only full access enable parameters
add_parameter TRANSFER_TYPE STRING "Aligned Accesses" "Setting the access types will allow you to reduce the hardware footprint and increase the fmax when unnecessary features like unaligned accesses are not necessary for your system."
set_parameter_property TRANSFER_TYPE ALLOWED_RANGES { "Full Word Accesses Only" "Aligned Accesses" "Unaligned Accesses" }
set_parameter_property TRANSFER_TYPE DISPLAY_NAME "Transfer Type"
set_parameter_property TRANSFER_TYPE DISPLAY_HINT radio
set_parameter_property TRANSFER_TYPE AFFECTS_GENERATION false
set_parameter_property TRANSFER_TYPE DERIVED false
set_parameter_property TRANSFER_TYPE HDL_PARAMETER false
set_parameter_property TRANSFER_TYPE AFFECTS_ELABORATION false
add_display_item "Memory Access Options" TRANSFER_TYPE parameter





# following parameters are for the data streaming port of the master
add_parameter PACKET_ENABLE INTEGER 0 "Enable packet support to add packetized streaming outputs."
set_parameter_property PACKET_ENABLE DISPLAY_NAME "Packet Support Enable"
set_parameter_property PACKET_ENABLE DISPLAY_HINT boolean
set_parameter_property PACKET_ENABLE AFFECTS_GENERATION false
set_parameter_property PACKET_ENABLE DERIVED false
set_parameter_property PACKET_ENABLE HDL_PARAMETER true
set_parameter_property PACKET_ENABLE AFFECTS_ELABORATION true
add_display_item "Streaming Options" PACKET_ENABLE parameter

add_parameter ERROR_ENABLE Integer 0 "Enable error support to include a streaming error output."
set_parameter_property ERROR_ENABLE DISPLAY_NAME "Error Enable"
set_parameter_property ERROR_ENABLE DISPLAY_HINT boolean
set_parameter_property ERROR_ENABLE AFFECTS_GENERATION false
set_parameter_property ERROR_ENABLE DERIVED false
set_parameter_property ERROR_ENABLE HDL_PARAMETER true
set_parameter_property ERROR_ENABLE AFFECTS_ELABORATION true
add_display_item "Streaming Options" ERROR_ENABLE parameter

add_parameter ERROR_WIDTH INTEGER 8 "Set the error width according to the number of error lines connected to the data source port."
set_parameter_property ERROR_WIDTH ALLOWED_RANGES {1 2 3 4 5 6 7 8}
set_parameter_property ERROR_WIDTH DISPLAY_NAME "Error Width"
set_parameter_property ERROR_WIDTH AFFECTS_GENERATION false
set_parameter_property ERROR_WIDTH DERIVED false
set_parameter_property ERROR_WIDTH HDL_PARAMETER true
set_parameter_property ERROR_WIDTH AFFECTS_ELABORATION true
add_display_item "Streaming Options" ERROR_WIDTH parameter



# The remaining parameters are not displayed in the GUI
add_parameter BYTE_ENABLE_WIDTH INTEGER 4
set_parameter_property BYTE_ENABLE_WIDTH AFFECTS_GENERATION false
set_parameter_property BYTE_ENABLE_WIDTH DERIVED true
set_parameter_property BYTE_ENABLE_WIDTH HDL_PARAMETER true
set_parameter_property BYTE_ENABLE_WIDTH AFFECTS_ELABORATION true
set_parameter_property BYTE_ENABLE_WIDTH VISIBLE false

add_parameter BYTE_ENABLE_WIDTH_LOG2 INTEGER 2
set_parameter_property BYTE_ENABLE_WIDTH_LOG2 AFFECTS_GENERATION false
set_parameter_property BYTE_ENABLE_WIDTH_LOG2 DERIVED true
set_parameter_property BYTE_ENABLE_WIDTH_LOG2 HDL_PARAMETER true
set_parameter_property BYTE_ENABLE_WIDTH_LOG2 AFFECTS_ELABORATION false
set_parameter_property BYTE_ENABLE_WIDTH_LOG2 VISIBLE false

add_parameter NO_BYTEENABLES INTEGER 0
set_parameter_property NO_BYTEENABLES AFFECTS_GENERATION false
set_parameter_property NO_BYTEENABLES DERIVED true
set_parameter_property NO_BYTEENABLES HDL_PARAMETER true
set_parameter_property NO_BYTEENABLES AFFECTS_ELABORATION true
set_parameter_property NO_BYTEENABLES VISIBLE false

add_parameter AUTO_ADDRESS_WIDTH INTEGER 32
set_parameter_property AUTO_ADDRESS_WIDTH AFFECTS_GENERATION false
set_parameter_property AUTO_ADDRESS_WIDTH DERIVED true
set_parameter_property AUTO_ADDRESS_WIDTH HDL_PARAMETER false
set_parameter_property AUTO_ADDRESS_WIDTH AFFECTS_ELABORATION true
set_parameter_property AUTO_ADDRESS_WIDTH VISIBLE false
set_parameter_property AUTO_ADDRESS_WIDTH SYSTEM_INFO {ADDRESS_WIDTH Data_Write_Master}

add_parameter ADDRESS_WIDTH INTEGER 32
set_parameter_property ADDRESS_WIDTH AFFECTS_GENERATION false
set_parameter_property ADDRESS_WIDTH DERIVED true
set_parameter_property ADDRESS_WIDTH HDL_PARAMETER true
set_parameter_property ADDRESS_WIDTH AFFECTS_ELABORATION true
set_parameter_property ADDRESS_WIDTH VISIBLE false

add_parameter FIFO_DEPTH_LOG2 INTEGER 5
set_parameter_property FIFO_DEPTH_LOG2 AFFECTS_GENERATION false
set_parameter_property FIFO_DEPTH_LOG2 DERIVED true
set_parameter_property FIFO_DEPTH_LOG2 HDL_PARAMETER true
set_parameter_property FIFO_DEPTH_LOG2 AFFECTS_ELABORATION false
set_parameter_property FIFO_DEPTH_LOG2 VISIBLE false

add_parameter SYMBOL_WIDTH INTEGER 8
set_parameter_property SYMBOL_WIDTH AFFECTS_GENERATION false
set_parameter_property SYMBOL_WIDTH DERIVED true
set_parameter_property SYMBOL_WIDTH HDL_PARAMETER true
set_parameter_property SYMBOL_WIDTH AFFECTS_ELABORATION true
set_parameter_property SYMBOL_WIDTH VISIBLE false

add_parameter NUMBER_OF_SYMBOLS INTEGER 4
set_parameter_property NUMBER_OF_SYMBOLS AFFECTS_GENERATION false
set_parameter_property NUMBER_OF_SYMBOLS DERIVED true
set_parameter_property NUMBER_OF_SYMBOLS HDL_PARAMETER true
set_parameter_property NUMBER_OF_SYMBOLS AFFECTS_ELABORATION true
set_parameter_property NUMBER_OF_SYMBOLS VISIBLE false

add_parameter NUMBER_OF_SYMBOLS_LOG2 INTEGER 2
set_parameter_property NUMBER_OF_SYMBOLS_LOG2 AFFECTS_GENERATION false
set_parameter_property NUMBER_OF_SYMBOLS_LOG2 DERIVED true
set_parameter_property NUMBER_OF_SYMBOLS_LOG2 HDL_PARAMETER true
set_parameter_property NUMBER_OF_SYMBOLS_LOG2 AFFECTS_ELABORATION true
set_parameter_property NUMBER_OF_SYMBOLS_LOG2 VISIBLE false

add_parameter MAX_BURST_COUNT_WIDTH INTEGER 2
set_parameter_property MAX_BURST_COUNT_WIDTH AFFECTS_GENERATION false
set_parameter_property MAX_BURST_COUNT_WIDTH DERIVED true
set_parameter_property MAX_BURST_COUNT_WIDTH HDL_PARAMETER true
set_parameter_property MAX_BURST_COUNT_WIDTH AFFECTS_ELABORATION true
set_parameter_property MAX_BURST_COUNT_WIDTH VISIBLE false

add_parameter UNALIGNED_ACCESSES_ENABLE INTEGER 0
set_parameter_property UNALIGNED_ACCESSES_ENABLE AFFECTS_GENERATION false
set_parameter_property UNALIGNED_ACCESSES_ENABLE DERIVED true
set_parameter_property UNALIGNED_ACCESSES_ENABLE HDL_PARAMETER true
set_parameter_property UNALIGNED_ACCESSES_ENABLE AFFECTS_ELABORATION false
set_parameter_property UNALIGNED_ACCESSES_ENABLE VISIBLE false

add_parameter ONLY_FULL_ACCESS_ENABLE Integer 0
set_parameter_property ONLY_FULL_ACCESS_ENABLE AFFECTS_GENERATION false
set_parameter_property ONLY_FULL_ACCESS_ENABLE DERIVED true
set_parameter_property ONLY_FULL_ACCESS_ENABLE HDL_PARAMETER true
set_parameter_property ONLY_FULL_ACCESS_ENABLE AFFECTS_ELABORATION false
set_parameter_property ONLY_FULL_ACCESS_ENABLE VISIBLE false

add_parameter BURST_WRAPPING_SUPPORT INTEGER 1
set_parameter_property BURST_WRAPPING_SUPPORT AFFECTS_GENERATION false
set_parameter_property BURST_WRAPPING_SUPPORT DERIVED true
set_parameter_property BURST_WRAPPING_SUPPORT HDL_PARAMETER true
set_parameter_property BURST_WRAPPING_SUPPORT AFFECTS_ELABORATION true
set_parameter_property BURST_WRAPPING_SUPPORT VISIBLE false

add_parameter PROGRAMMABLE_BURST_ENABLE INTEGER 0
set_parameter_property PROGRAMMABLE_BURST_ENABLE AFFECTS_GENERATION false
set_parameter_property PROGRAMMABLE_BURST_ENABLE DERIVED true
set_parameter_property PROGRAMMABLE_BURST_ENABLE HDL_PARAMETER true
set_parameter_property PROGRAMMABLE_BURST_ENABLE AFFECTS_ELABORATION false
set_parameter_property PROGRAMMABLE_BURST_ENABLE VISIBLE false

add_parameter MAX_BURST_COUNT INTEGER 2
set_parameter_property MAX_BURST_COUNT AFFECTS_GENERATION false
set_parameter_property MAX_BURST_COUNT DERIVED true
set_parameter_property MAX_BURST_COUNT HDL_PARAMETER true
set_parameter_property MAX_BURST_COUNT AFFECTS_ELABORATION true
set_parameter_property MAX_BURST_COUNT VISIBLE false

add_parameter FIFO_SPEED_OPTIMIZATION Integer 1
set_parameter_property FIFO_SPEED_OPTIMIZATION AFFECTS_GENERATION false
set_parameter_property FIFO_SPEED_OPTIMIZATION DERIVED false
set_parameter_property FIFO_SPEED_OPTIMIZATION HDL_PARAMETER true
set_parameter_property FIFO_SPEED_OPTIMIZATION AFFECTS_ELABORATION false
set_parameter_property FIFO_SPEED_OPTIMIZATION VISIBLE false

add_parameter STRIDE_WIDTH INTEGER 1
set_parameter_property STRIDE_WIDTH AFFECTS_GENERATION false
set_parameter_property STRIDE_WIDTH DERIVED true
set_parameter_property STRIDE_WIDTH HDL_PARAMETER true
set_parameter_property STRIDE_WIDTH AFFECTS_ELABORATION false
set_parameter_property STRIDE_WIDTH VISIBLE false

# Need to add one just in case the length counter hits 0 and a few extra bytes get transferred (early termination logic will prevent the length register from rolling over)
add_parameter ACTUAL_BYTES_TRANSFERRED_WIDTH INTEGER 32
set_parameter_property ACTUAL_BYTES_TRANSFERRED_WIDTH AFFECTS_GENERATION false
set_parameter_property ACTUAL_BYTES_TRANSFERRED_WIDTH DERIVED true
set_parameter_property ACTUAL_BYTES_TRANSFERRED_WIDTH HDL_PARAMETER true
set_parameter_property ACTUAL_BYTES_TRANSFERRED_WIDTH AFFECTS_ELABORATION false
set_parameter_property ACTUAL_BYTES_TRANSFERRED_WIDTH VISIBLE false
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point Clock
# | 
add_interface Clock clock end
set_interface_property Clock ptfSchematicName ""

set_interface_property Clock ENABLED true

add_interface_port Clock clk clk Input 1
add_interface_port Clock reset reset Input 1
set_interface_property Clock_reset synchronousEdges BOTH
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point Data_Write_Master
# | 
add_interface Data_Write_Master avalon start
set_interface_property Data_Write_Master adaptsTo ""
set_interface_property Data_Write_Master doStreamReads false
set_interface_property Data_Write_Master doStreamWrites false
set_interface_property Data_Write_Master linewrapBursts false

set_interface_property Data_Write_Master ASSOCIATED_CLOCK Clock
set_interface_property Data_Write_Master ENABLED true

add_interface_port Data_Write_Master master_address address Output -1
add_interface_port Data_Write_Master master_write write Output 1
add_interface_port Data_Write_Master master_byteenable byteenable Output -1
add_interface_port Data_Write_Master master_writedata writedata Output -1
add_interface_port Data_Write_Master master_waitrequest waitrequest Input 1
add_interface_port Data_Write_Master master_burstcount burstcount Output -1
add_interface_port Data_Write_Master master_response response Input 2
add_interface_port Data_Write_Master master_writeresponsevalid writeresponsevalid Input 1


set_port_property master_burstcount VHDL_TYPE STD_LOGIC_VECTOR
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point Data_Sink
# | 
add_interface Data_Sink avalon_streaming end
set_interface_property Data_Sink dataBitsPerSymbol 8
set_interface_property Data_Sink errorDescriptor ""
set_interface_property Data_Sink maxChannel 0
set_interface_property Data_Sink readyLatency 0
set_interface_property Data_Sink symbolsPerBeat 4

set_interface_property Data_Sink ASSOCIATED_CLOCK Clock
set_interface_property Data_Sink ENABLED true

# add_interface_port Data_Sink snk_data data Input -1
# add_interface_port Data_Sink snk_valid valid Input 1
# add_interface_port Data_Sink snk_ready ready Output 1
# add_interface_port Data_Sink snk_sop startofpacket Input 1
# add_interface_port Data_Sink snk_eop endofpacket Input 1
# add_interface_port Data_Sink snk_empty empty Input -1
# add_interface_port Data_Sink snk_error error Input -1
add_interface_port Data_Sink snk_data data Input DATA_WIDTH
add_interface_port Data_Sink snk_valid valid Input 1
add_interface_port Data_Sink snk_ready ready Output 1
add_interface_port Data_Sink snk_sop startofpacket Input 1
add_interface_port Data_Sink snk_eop endofpacket Input 1
add_interface_port Data_Sink snk_empty empty Input NUMBER_OF_SYMBOLS_LOG2
add_interface_port Data_Sink snk_error error Input ERROR_WIDTH

# | 
# +-----------------------------------

# +-----------------------------------
# | connection point Command_Sink
# | 
add_interface Command_Sink avalon_streaming end
set_interface_property Command_Sink dataBitsPerSymbol 256
set_interface_property Command_Sink errorDescriptor ""
set_interface_property Command_Sink maxChannel 0
set_interface_property Command_Sink readyLatency 0
set_interface_property Command_Sink symbolsPerBeat 1

set_interface_property Command_Sink ASSOCIATED_CLOCK Clock
set_interface_property Command_Sink ENABLED true

add_interface_port Command_Sink snk_command_data data Input 256
add_interface_port Command_Sink snk_command_valid valid Input 1
add_interface_port Command_Sink snk_command_ready ready Output 1
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point Response_Source
# | 
add_interface Response_Source avalon_streaming start
set_interface_property Response_Source dataBitsPerSymbol 256
set_interface_property Response_Source errorDescriptor ""
set_interface_property Response_Source maxChannel 0
set_interface_property Response_Source readyLatency 0
set_interface_property Response_Source symbolsPerBeat 1

set_interface_property Response_Source ASSOCIATED_CLOCK Clock
set_interface_property Response_Source ENABLED true

add_interface_port Response_Source src_response_data data Output 256
add_interface_port Response_Source src_response_valid valid Output 1
add_interface_port Response_Source src_response_ready ready Input 1
# | 
# +-----------------------------------


# the elaboration callback will be used to enable/disable the error, empty, eop, sop signals
# based on user input as well as control the width of all the signals that have variable width
proc elaborate_me {}  {

  set_port_property snk_data WIDTH_EXPR [get_parameter_value DATA_WIDTH]
  set_port_property snk_error WIDTH_EXPR [get_parameter_value ERROR_WIDTH]
  set_port_property master_burstcount WIDTH_EXPR [get_parameter_value MAX_BURST_COUNT_WIDTH]


  # need to make sure the empty signal isn't using a width of 0 which would be the case if DATA_WIDTH is 8.  This port will be terminated if DATA_WIDTH is 8.
  if { [get_parameter_value DATA_WIDTH] == 8 }  {
    set_port_property snk_empty WIDTH_EXPR 1
  } else {
    set_port_property snk_empty WIDTH_EXPR [get_parameter_value NUMBER_OF_SYMBOLS_LOG2]
  }
  set_port_property master_byteenable WIDTH_EXPR [get_parameter_value BYTE_ENABLE_WIDTH]
  set_port_property master_address WIDTH_EXPR [get_parameter_value ADDRESS_WIDTH]
  set_port_property master_writedata WIDTH_EXPR [get_parameter_value DATA_WIDTH]

  set_interface_property Data_Sink symbolsPerBeat [get_parameter_value NUMBER_OF_SYMBOLS]  

  if { [get_parameter_value ERROR_ENABLE] == 1 }  { 
    set_port_property snk_error TERMINATION false
  } else {
    set_port_property snk_error TERMINATION true
  }

  # don't need byte enables if the data width is 8
  if { [get_parameter_value DATA_WIDTH] > 8 }  {
    set_port_property master_byteenable TERMINATION false
  } else {
    set_port_property master_byteenable TERMINATION true
  }

  if { [get_parameter_value PACKET_ENABLE] == 1 }  {
    set_port_property snk_sop TERMINATION false
    set_port_property snk_eop TERMINATION false
  } else {
    set_port_property snk_sop TERMINATION true
    set_port_property snk_eop TERMINATION true
  }

  if { ([get_parameter_value PACKET_ENABLE] == 1) && ([get_parameter_value DATA_WIDTH] > 8) }  {
    set_port_property snk_empty TERMINATION false
  } else {
    set_port_property snk_empty TERMINATION true
  }

  if { [get_parameter_value BURST_ENABLE] == 1 }  {
    set_port_property master_burstcount TERMINATION false
  } else {
    set_port_property master_burstcount TERMINATION true
  }

  # when the forced burst alignment is enabled the master will post bursts of 1 until the next burst boundary is reached so this is safe to enable.
  if { [get_parameter_value BURST_WRAPPING_SUPPORT] == 1 }  {
    set_interface_property Data_Write_Master burstOnBurstBoundariesOnly true
  } else {
    set_interface_property Data_Write_Master burstOnBurstBoundariesOnly false
  }

}


# the validation callback will be enabling/disabling GUI controls based on user input
proc validate_me {}  {

  #  need to use full word accesses when data width is 8-bits wide
  if { ([get_parameter_value TRANSFER_TYPE] == "Full Word Accesses Only") || ([get_parameter_value DATA_WIDTH] == 8) }  {
    set_parameter_value UNALIGNED_ACCESSES_ENABLE 0
    set_parameter_value ONLY_FULL_ACCESS_ENABLE 1
    set_parameter_property GUI_NO_BYTEENABLES ENABLED false
    set_parameter_value NO_BYTEENABLES 0
  } else {
    if { [get_parameter_value TRANSFER_TYPE] == "Aligned Accesses" }  {
      set_parameter_value UNALIGNED_ACCESSES_ENABLE 0
      set_parameter_value ONLY_FULL_ACCESS_ENABLE 0
      set_parameter_property GUI_NO_BYTEENABLES ENABLED true
      set_parameter_value NO_BYTEENABLES [get_parameter_value GUI_NO_BYTEENABLES]
    } else {
      set_parameter_value UNALIGNED_ACCESSES_ENABLE 1
      set_parameter_value ONLY_FULL_ACCESS_ENABLE 0
      set_parameter_property GUI_NO_BYTEENABLES ENABLED false
      set_parameter_value NO_BYTEENABLES 0
    }
  }


  if { [get_parameter_value BURST_ENABLE] == 1 }  {
    set_parameter_property GUI_MAX_BURST_COUNT ENABLED true
    set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE ENABLED true
    set_parameter_property GUI_BURST_WRAPPING_SUPPORT ENABLED true
    set_parameter_value MAX_BURST_COUNT [get_parameter_value GUI_MAX_BURST_COUNT]
    set_parameter_value MAX_BURST_COUNT_WIDTH [expr {(log([get_parameter_value GUI_MAX_BURST_COUNT]) / log(2)) + 1}]
    set_parameter_value PROGRAMMABLE_BURST_ENABLE [get_parameter_value GUI_PROGRAMMABLE_BURST_ENABLE]
    set_parameter_value BURST_WRAPPING_SUPPORT [get_parameter_value GUI_BURST_WRAPPING_SUPPORT]
  } else {
    set_parameter_property GUI_MAX_BURST_COUNT ENABLED false
    set_parameter_property GUI_PROGRAMMABLE_BURST_ENABLE ENABLED false
    set_parameter_property GUI_BURST_WRAPPING_SUPPORT ENABLED false
    set_parameter_value MAX_BURST_COUNT 1
    set_parameter_value MAX_BURST_COUNT_WIDTH 1
    set_parameter_value PROGRAMMABLE_BURST_ENABLE 0
    set_parameter_value BURST_WRAPPING_SUPPORT 0
  }


  if { [get_parameter_value STRIDE_ENABLE] == 1 }  {
    set_parameter_property GUI_STRIDE_WIDTH ENABLED true
    set_parameter_value STRIDE_WIDTH [get_parameter_value GUI_STRIDE_WIDTH]
  } else {
    set_parameter_property GUI_STRIDE_WIDTH ENABLED false
    set_parameter_value STRIDE_WIDTH 1
  }

  if { [get_parameter_value ERROR_ENABLE] == 1 }  {
    set_parameter_property ERROR_WIDTH ENABLED true
  } else {
    set_parameter_property ERROR_WIDTH ENABLED false
  }

  if { [get_parameter_value PACKET_ENABLE] == 1 }  {
    # actual bytes transferred can be up to 32 bits wide.  Need to set this register width to be one wider than the length since the early termination may let a few extra bytes slip through before the length hits 0
    if { [get_parameter_value LENGTH_WIDTH] == 32 }  {
      set_parameter_value ACTUAL_BYTES_TRANSFERRED_WIDTH 32
    } else {
      set_parameter_value ACTUAL_BYTES_TRANSFERRED_WIDTH [expr {[get_parameter_value LENGTH_WIDTH] + 1}]
    }
  } else {
    set_parameter_value ACTUAL_BYTES_TRANSFERRED_WIDTH 32
  }

  if { [get_parameter_value USE_FIX_ADDRESS_WIDTH] == 1 }  {
    set_parameter_property FIX_ADDRESS_WIDTH ENABLED true
    set_parameter_value ADDRESS_WIDTH [get_parameter_value FIX_ADDRESS_WIDTH]
  } else {
    set_parameter_property FIX_ADDRESS_WIDTH ENABLED false
    set_parameter_value ADDRESS_WIDTH [get_parameter_value AUTO_ADDRESS_WIDTH]
  }

  set_parameter_value BYTE_ENABLE_WIDTH [expr {[get_parameter_value DATA_WIDTH] / 8}]
  set_parameter_value NUMBER_OF_SYMBOLS [expr {[get_parameter_value DATA_WIDTH] / 8}]
  set_parameter_value FIFO_DEPTH_LOG2 [expr {(log([get_parameter_value FIFO_DEPTH]) / log(2))}]


  # in the case of 8 bit data need to make sure that the log2 values don't result in 0
  if { [get_parameter_value DATA_WIDTH] == 8 }  {
    set_parameter_value BYTE_ENABLE_WIDTH_LOG2 1
    set_parameter_value NUMBER_OF_SYMBOLS_LOG2 1
  } else {
    set_parameter_value BYTE_ENABLE_WIDTH_LOG2 [expr {(log([get_parameter_value DATA_WIDTH] / 8) / log(2))}]
    set_parameter_value NUMBER_OF_SYMBOLS_LOG2 [expr {(log([get_parameter_value DATA_WIDTH] / 8) / log(2))}]
  }


  if { [get_parameter_value ADDRESS_WIDTH] < [get_parameter_value LENGTH_WIDTH] }  {
    send_message Info "You have selected a length width that spans a larger transfer size than is addressable by the master port.  Reducing the length width will improve the Fmax of this component."
  }

#  if { ([get_parameter_value BURST_ENABLE] == 1) && ([get_parameter_value BURST_WRAPPING_SUPPORT] == 0) }  {
#    send_message Warning "If you connect the read master to a burst wrapping slave like SDRAM your system may experience data corruption without the Forced Burst Alignment setting enabled."
#  }

  if { ([get_parameter_value TRANSFER_TYPE] == "Unaligned Accesses") && ([get_parameter_value STRIDE_ENABLE] == 1) }  {
    send_message Error "Unaligned accesses and stride support cannot be enabled concurrently."
  }

  if { ([get_parameter_value STRIDE_ENABLE] == 1) && ([get_parameter_value BURST_ENABLE] == 1) }  {
    send_message Error "Burst and stride support cannot be enabled concurrently."
  }

  if { ([get_parameter_value BURST_ENABLE] == 1) && ([get_parameter_value PROGRAMMABLE_BURST_ENABLE] == 1) && ([get_parameter_value MAX_BURST_COUNT_WIDTH] > 8) }  {
    send_message Warning "You have selected programmable burst support but the maximum programmable burst count size is 128.  If you re-program the burst count you will be limited to bursts of up to 128 beats."
  }

  if { ([get_parameter_value BURST_ENABLE] == 1) && ([get_parameter_value MAX_BURST_COUNT] > [expr [get_parameter_value FIFO_DEPTH] / 4]) }  {
    send_message Error "You must select a FIFO size that is at least four times the maximum burst length."
  }

  if { ([get_parameter_value PROGRAMMABLE_BURST_ENABLE] == 1) && ([get_parameter_value BURST_WRAPPING_SUPPORT] == 1) }  {
    send_message Error "You cannot use programmable bursts and forced burst alignment concurrently." 
  }

}

## Add documentation links for user guide and/or release notes
add_documentation_link "User Guide" https://documentation.altera.com/#/link/dmi1420813268955/dmi1421419198649
add_documentation_link "Release Notes" https://documentation.altera.com/#/link/hco1421698042087/hco1421698013408
