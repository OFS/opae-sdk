##
## Platform interface timing constraints.
##

##
## These signals in ccip_async_shim are in the pClk domain as the last stage
## before clock crossing.
##
set_false_path -from [get_keepers *|platform_shim_ccip|c.ccip_async_shim|reset[0]]
set_false_path -from [get_keepers *|platform_shim_ccip|c.ccip_async_shim|error[0]]
set_false_path -from [get_keepers *|platform_shim_ccip|c.ccip_async_shim|pwrState[0]*]
set_false_path -from [get_keepers *|platform_shim_ccip|c.ccip_async_shim|async_shim_error_bb*]

##
## Reset path to local memory clock after clock crossing.
##
set_false_path -to [get_keepers *|platform_shim_avalon_mem_if|c.mm_async*.local_mem_reset_pipe[0]]
