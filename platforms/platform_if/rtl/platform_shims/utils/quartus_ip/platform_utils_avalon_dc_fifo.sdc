# $File: //acds/rel/18.1/ip/sopc/components/altera_avalon_dc_fifo/altera_avalon_dc_fifo.sdc $
# $Revision: #2 $
# $Date: 2018/08/03 $
# $Author: kknagar $

#-------------------------------------------------------------------------------
# TimeQuest constraints to constrain the timing across asynchronous clock domain crossings.
# The idea is to minimize skew to less than one launch clock period to keep the gray encoding, 
# and to minimize latency on the pointer crossings.
#
# The paths are from the Gray Code read and write pointers to their respective synchronizer banks.
#
# *** Important note *** 
#
# Do not declare the FIFO clocks as asynchronous at the top level, or false path these crossings,
# because that will override these constraints.
#-------------------------------------------------------------------------------
set all_dc_fifo [get_entity_instances platform_utils_avalon_dc_fifo]

set_max_delay -from [get_registers {*|in_wr_ptr_gray[*]}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:write_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}] 200
set_min_delay -from [get_registers {*|in_wr_ptr_gray[*]}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:write_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}] -200

set_max_delay -from [get_registers {*|out_rd_ptr_gray[*]}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:read_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}] 200
set_min_delay -from [get_registers {*|out_rd_ptr_gray[*]}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:read_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}] -200

set_net_delay -max -get_value_from_clock_period dst_clock_period -value_multiplier 0.8 -from [get_pins -compatibility_mode {*|in_wr_ptr_gray[*]*}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:write_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}] 
set_net_delay -max -get_value_from_clock_period dst_clock_period -value_multiplier 0.8 -from [get_pins -compatibility_mode {*|out_rd_ptr_gray[*]*}] -to [get_registers {*|platform_utils_dcfifo_synchronizer_bundle:read_crosser|platform_utils_std_synchronizer_nocut:sync[*].u|din_s1}]


foreach dc_fifo_inst $all_dc_fifo {
   if { [ llength [query_collection -report -all [get_registers $dc_fifo_inst|in_wr_ptr_gray[*]]]] > 0  } {
      set_max_skew -get_skew_value_from_clock_period src_clock_period -skew_value_multiplier 0.8  -from [get_registers $dc_fifo_inst|in_wr_ptr_gray[*]] -to [get_registers $dc_fifo_inst|write_crosser|sync[*].u|din_s1] 
   }

   if { [ llength [query_collection -report -all [get_registers $dc_fifo_inst|out_rd_ptr_gray[*]]]] > 0 } {
      set_max_skew -get_skew_value_from_clock_period src_clock_period -skew_value_multiplier 0.8  -from [get_registers $dc_fifo_inst|out_rd_ptr_gray[*]] -to [get_registers $dc_fifo_inst|read_crosser|sync[*].u|din_s1] 
   }
}


# add in timing constraints across asynchronous clock domain crossings for simple dual clock memory inference

set mem_regs [get_registers -nowarn *|platform_utils_avalon_dc_fifo:*|mem*];
if {![llength [query_collection -report -all $mem_regs]] > 0} {
    set mem_regs [get_registers -nowarn platform_utils_avalon_dc_fifo:*|mem*];
}

set internal_out_payload_regs [get_registers -nowarn *|platform_utils_avalon_dc_fifo:*|internal_out_payload*];
if {![llength [query_collection -report -all $internal_out_payload_regs]] > 0} {
    set internal_out_payload_regs [get_registers -nowarn platform_utils_avalon_dc_fifo:*|internal_out_payload*];
}

if {[llength [query_collection -report -all $internal_out_payload_regs]] > 0 && [llength [query_collection -report -all $mem_regs]] > 0} {
    set_max_delay -from $mem_regs -to $internal_out_payload_regs 200
    set_min_delay -from $mem_regs -to $internal_out_payload_regs -200

    set_net_delay -max -get_value_from_clock_period dst_clock_period -value_multiplier 0.8 -from $mem_regs -to $internal_out_payload_regs

    #set_max_skew 2 -from $mem_regs -to $internal_out_payload_regs
}

# -----------------------------------------------------------------------------
# This procedure constrains the skew between the pointer bits, and should
# be called from the top level SDC, once per instance of the FIFO.
#
# The hierarchy path to the FIFO instance is required as an 
# argument.
# -----------------------------------------------------------------------------
proc constrain_platform_utils_avalon_dc_fifo_ptr_skew { path } {

    set_max_skew -get_skew_value_from_clock_period src_clock_period -skew_value_multiplier 0.8 -from [ get_registers $path|in_wr_ptr_gray\[*\] ] -to [ get_registers $path|write_crosser|sync\[*\].u|din_s1 ]
    set_max_skew -get_skew_value_from_clock_period src_clock_period -skew_value_multiplier 0.8 -from [ get_registers $path|out_rd_ptr_gray\[*\] ] -to [ get_registers $path|read_crosser|sync\[*\].u|din_s1 ]

}
