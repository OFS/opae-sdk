# (C) 2017 Intel Corporation. All rights reserved.
# Your use of Intel Corporation's design tools, logic functions and other
# software and tools, and its AMPP partner logic functions, and any output
# files any of the foregoing (including device programming or simulation
# files), and any associated documentation or information are expressly subject
# to the terms and conditions of the Intel Program License Subscription
# Agreement, Intel MegaCore Function License Agreement, or other applicable
# license agreement, including, without limitation, that your use is for the
# sole purpose of programming logic devices manufactured by Intel and sold by
# Intel or its authorized distributors.  Please refer to the applicable
# agreement for further details.


# 
# afu_id_avmm_slave "afu_id_avmm_slave" v1.0
#  2017.05.16.18:47:54
# 
# 

# 
# request TCL package from ACDS 17.0
# 
package require -exact qsys 17.0


# 
# module afu_id_avmm_slave
# 
set_module_property DESCRIPTION ""
set_module_property NAME afu_id_avmm_slave
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property AUTHOR ""
set_module_property DISPLAY_NAME afu_id_avmm_slave
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property REPORT_TO_TALKBACK false
set_module_property ALLOW_GREYBOX_GENERATION false
set_module_property REPORT_HIERARCHY false


# 
# file sets
# 
add_fileset QUARTUS_SYNTH QUARTUS_SYNTH "" ""
set_fileset_property QUARTUS_SYNTH TOP_LEVEL afu_id_avmm_slave
set_fileset_property QUARTUS_SYNTH ENABLE_RELATIVE_INCLUDE_PATHS false
set_fileset_property QUARTUS_SYNTH ENABLE_FILE_OVERWRITE_MODE false
add_fileset_file afu_id_avmm_slave.sv SYSTEM_VERILOG PATH afu_id_avmm_slave.sv TOP_LEVEL_FILE


# 
# parameters
# 
add_parameter AFU_ID_H STD_LOGIC_VECTOR 3683296936938783210 ""
set_parameter_property AFU_ID_H DEFAULT_VALUE 3683296936938783210
set_parameter_property AFU_ID_H DISPLAY_NAME AFU_ID_H
set_parameter_property AFU_ID_H WIDTH 64
set_parameter_property AFU_ID_H TYPE STD_LOGIC_VECTOR
set_parameter_property AFU_ID_H UNITS None
set_parameter_property AFU_ID_H DESCRIPTION ""
set_parameter_property AFU_ID_H HDL_PARAMETER true
add_parameter AFU_ID_L STD_LOGIC_VECTOR 10412877091747224746 ""
set_parameter_property AFU_ID_L DEFAULT_VALUE 10412877091747224746
set_parameter_property AFU_ID_L DISPLAY_NAME AFU_ID_L
set_parameter_property AFU_ID_L WIDTH 64
set_parameter_property AFU_ID_L TYPE STD_LOGIC_VECTOR
set_parameter_property AFU_ID_L UNITS None
set_parameter_property AFU_ID_L DESCRIPTION ""
set_parameter_property AFU_ID_L HDL_PARAMETER true
add_parameter DFH_FEATURE_TYPE STD_LOGIC_VECTOR 1
set_parameter_property DFH_FEATURE_TYPE DEFAULT_VALUE 1
set_parameter_property DFH_FEATURE_TYPE DISPLAY_NAME DFH_FEATURE_TYPE
set_parameter_property DFH_FEATURE_TYPE WIDTH 4
set_parameter_property DFH_FEATURE_TYPE TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_FEATURE_TYPE UNITS None
set_parameter_property DFH_FEATURE_TYPE ALLOWED_RANGES {1 2 3}
set_parameter_property DFH_FEATURE_TYPE HDL_PARAMETER true
add_parameter DFH_AFU_MINOR_REV STD_LOGIC_VECTOR 0
set_parameter_property DFH_AFU_MINOR_REV DEFAULT_VALUE 0
set_parameter_property DFH_AFU_MINOR_REV DISPLAY_NAME DFH_AFU_MINOR_REV
set_parameter_property DFH_AFU_MINOR_REV WIDTH 4
set_parameter_property DFH_AFU_MINOR_REV TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_AFU_MINOR_REV UNITS None
set_parameter_property DFH_AFU_MINOR_REV ALLOWED_RANGES 0:15
set_parameter_property DFH_AFU_MINOR_REV HDL_PARAMETER true
add_parameter DFH_AFU_MAJOR_REV STD_LOGIC_VECTOR 0
set_parameter_property DFH_AFU_MAJOR_REV DEFAULT_VALUE 0
set_parameter_property DFH_AFU_MAJOR_REV DISPLAY_NAME DFH_AFU_MAJOR_REV
set_parameter_property DFH_AFU_MAJOR_REV WIDTH 4
set_parameter_property DFH_AFU_MAJOR_REV TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_AFU_MAJOR_REV UNITS None
set_parameter_property DFH_AFU_MAJOR_REV ALLOWED_RANGES 0:15
set_parameter_property DFH_AFU_MAJOR_REV HDL_PARAMETER true
add_parameter DFH_END_OF_LIST STD_LOGIC_VECTOR 1
set_parameter_property DFH_END_OF_LIST DEFAULT_VALUE 1
set_parameter_property DFH_END_OF_LIST DISPLAY_NAME DFH_END_OF_LIST
set_parameter_property DFH_END_OF_LIST WIDTH 1
set_parameter_property DFH_END_OF_LIST TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_END_OF_LIST UNITS None
set_parameter_property DFH_END_OF_LIST ALLOWED_RANGES 0:1
set_parameter_property DFH_END_OF_LIST HDL_PARAMETER true
add_parameter DFH_NEXT_OFFSET STD_LOGIC_VECTOR 0
set_parameter_property DFH_NEXT_OFFSET DEFAULT_VALUE 0
set_parameter_property DFH_NEXT_OFFSET DISPLAY_NAME DFH_NEXT_OFFSET
set_parameter_property DFH_NEXT_OFFSET WIDTH 24
set_parameter_property DFH_NEXT_OFFSET TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_NEXT_OFFSET UNITS None
set_parameter_property DFH_NEXT_OFFSET ALLOWED_RANGES 0:16777215
set_parameter_property DFH_NEXT_OFFSET HDL_PARAMETER true
add_parameter DFH_FEATURE_ID STD_LOGIC_VECTOR 0
set_parameter_property DFH_FEATURE_ID DEFAULT_VALUE 0
set_parameter_property DFH_FEATURE_ID DISPLAY_NAME DFH_FEATURE_ID
set_parameter_property DFH_FEATURE_ID WIDTH 12
set_parameter_property DFH_FEATURE_ID TYPE STD_LOGIC_VECTOR
set_parameter_property DFH_FEATURE_ID UNITS None
set_parameter_property DFH_FEATURE_ID ALLOWED_RANGES 0:4095
set_parameter_property DFH_FEATURE_ID HDL_PARAMETER true
add_parameter NEXT_AFU_OFFSET STD_LOGIC_VECTOR 0
set_parameter_property NEXT_AFU_OFFSET DEFAULT_VALUE 0
set_parameter_property NEXT_AFU_OFFSET DISPLAY_NAME NEXT_AFU_OFFSET
set_parameter_property NEXT_AFU_OFFSET WIDTH 24
set_parameter_property NEXT_AFU_OFFSET TYPE STD_LOGIC_VECTOR
set_parameter_property NEXT_AFU_OFFSET UNITS None
set_parameter_property NEXT_AFU_OFFSET ALLOWED_RANGES 0:16777215
set_parameter_property NEXT_AFU_OFFSET HDL_PARAMETER true
add_parameter CREATE_SCRATCH_REG STD_LOGIC_VECTOR 0 ""
set_parameter_property CREATE_SCRATCH_REG DEFAULT_VALUE 0
set_parameter_property CREATE_SCRATCH_REG DISPLAY_NAME CREATE_SCRATCH_REG
set_parameter_property CREATE_SCRATCH_REG WIDTH 1
set_parameter_property CREATE_SCRATCH_REG TYPE STD_LOGIC_VECTOR
set_parameter_property CREATE_SCRATCH_REG UNITS None
set_parameter_property CREATE_SCRATCH_REG ALLOWED_RANGES 0:1
set_parameter_property CREATE_SCRATCH_REG DESCRIPTION ""
set_parameter_property CREATE_SCRATCH_REG HDL_PARAMETER true


# 
# display items
# 


# 
# connection point clock
# 
add_interface clock clock end
set_interface_property clock clockRate 0
set_interface_property clock ENABLED true
set_interface_property clock EXPORT_OF ""
set_interface_property clock PORT_NAME_MAP ""
set_interface_property clock CMSIS_SVD_VARIABLES ""
set_interface_property clock SVD_ADDRESS_GROUP ""

add_interface_port clock clk clk Input 1


# 
# connection point reset
# 
add_interface reset reset end
set_interface_property reset associatedClock clock
set_interface_property reset synchronousEdges BOTH
set_interface_property reset ENABLED true
set_interface_property reset EXPORT_OF ""
set_interface_property reset PORT_NAME_MAP ""
set_interface_property reset CMSIS_SVD_VARIABLES ""
set_interface_property reset SVD_ADDRESS_GROUP ""

add_interface_port reset reset reset Input 1


# 
# connection point afu_cfg_slave
# 
add_interface afu_cfg_slave avalon end
set_interface_property afu_cfg_slave addressUnits WORDS
set_interface_property afu_cfg_slave associatedClock clock
set_interface_property afu_cfg_slave associatedReset reset
set_interface_property afu_cfg_slave bitsPerSymbol 8
set_interface_property afu_cfg_slave bridgedAddressOffset ""
set_interface_property afu_cfg_slave bridgesToMaster ""
set_interface_property afu_cfg_slave burstOnBurstBoundariesOnly false
set_interface_property afu_cfg_slave burstcountUnits WORDS
set_interface_property afu_cfg_slave explicitAddressSpan 0
set_interface_property afu_cfg_slave holdTime 0
set_interface_property afu_cfg_slave linewrapBursts false
set_interface_property afu_cfg_slave maximumPendingReadTransactions 0
set_interface_property afu_cfg_slave maximumPendingWriteTransactions 0
set_interface_property afu_cfg_slave minimumResponseLatency 1
set_interface_property afu_cfg_slave readLatency 0
set_interface_property afu_cfg_slave readWaitTime 1
set_interface_property afu_cfg_slave setupTime 0
set_interface_property afu_cfg_slave timingUnits Cycles
set_interface_property afu_cfg_slave transparentBridge false
set_interface_property afu_cfg_slave waitrequestAllowance 0
set_interface_property afu_cfg_slave writeWaitTime 0
set_interface_property afu_cfg_slave ENABLED true
set_interface_property afu_cfg_slave EXPORT_OF ""
set_interface_property afu_cfg_slave PORT_NAME_MAP ""
set_interface_property afu_cfg_slave CMSIS_SVD_VARIABLES ""
set_interface_property afu_cfg_slave SVD_ADDRESS_GROUP ""

add_interface_port afu_cfg_slave avmm_readdata readdata Output "((64 - 1)) - (0) + 1"
add_interface_port afu_cfg_slave avmm_writedata writedata Input "((64 - 1)) - (0) + 1"
add_interface_port afu_cfg_slave avmm_address address Input "((3 - 1)) - (0) + 1"
add_interface_port afu_cfg_slave avmm_write write Input 1
add_interface_port afu_cfg_slave avmm_read read Input 1
set_interface_assignment afu_cfg_slave embeddedsw.configuration.isFlash 0
set_interface_assignment afu_cfg_slave embeddedsw.configuration.isMemoryDevice 0
set_interface_assignment afu_cfg_slave embeddedsw.configuration.isNonVolatileStorage 0
set_interface_assignment afu_cfg_slave embeddedsw.configuration.isPrintableDevice 0

