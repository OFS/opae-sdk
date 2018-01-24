## Copyright(c) 2017, 2018, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 2.8.12)
project("opae-c-ase-platform")

# Directories
set(ASE_SERVER_RTL     ${PROJECT_SOURCE_DIR})
set(PLATFORM_SHARE_DIR ${CMAKE_SOURCE_DIR}/../platform)
set(PLATFORM_IF_DIR    ${PLATFORM_SHARE_DIR}/platform_if)
set(PLATFORM_IF_RTL    ${PLATFORM_IF_DIR}/rtl/device_if)
set(ASE_SCRIPTS_IN     ${PROJECT_SOURCE_DIR}/../in)

## Verilog compilation
set(VLOG_SRC_LIST
  vlog_files.list
  CACHE STRING "Project file listing all SystemVerilog source code for the AFU")

set(ASE_PKG_FILE ${ASE_SERVER_RTL}/ase_pkg.sv)
set(PLATFORM_PKG_FILE ${PLATFORM_IF_RTL}/ccip_if_pkg.sv)

if(ASE_SIMULATOR STREQUAL "QUESTA")
  find_package(Questa)

  set(VLOG_DEFINES_MENTOR "+define+VENDOR_ALTERA +define+TOOL_QUARTUS # +define+NUM_AFUS=1 +define+NLB400_MODE_0"
    CACHE STRING "Modelsim/Questa compiler define flags")
  set(VLOG_INCLUDES_MENTOR "+incdir+.+work+${ASE_SERVER_RTL}+${PLATFORM_IF_RTL}"
    CACHE STRING "Modelsim/Questa compiler include flags")

  set(VLOG_OPTS_MENTOR "-nologo -sv +librescan -timescale ${ASE_TIMESCALE} -work work +define+${SIMULATOR} +define+${ASE_PLATFORM} +define+VENDOR_ALTERA +define+TOOL_QUARTUS -novopt")

  ## Questasim template
  configure_file(${ASE_SCRIPTS_IN}/afu_platform_config.py.in
    ${PROJECT_BINARY_DIR}/scripts/in/afu_platform_config.py @ONLY)
  configure_file(${ASE_SCRIPTS_IN}/afu_synth_setup_std.py.in
    ${PROJECT_BINARY_DIR}/scripts/in/afu_synth_setup.py @ONLY)
  configure_file(${ASE_SCRIPTS_IN}/rtl_src_config.py.in
    ${PROJECT_BINARY_DIR}/scripts/in/rtl_src_config.py @ONLY)

  # now copy the temporary into the final destination, setting the permissions
  file(COPY ${PROJECT_BINARY_DIR}/scripts/in/afu_platform_config.py
    DESTINATION ${PROJECT_BINARY_DIR}/scripts
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  file(COPY ${PROJECT_BINARY_DIR}/scripts/in/afu_synth_setup.py
    DESTINATION ${PROJECT_BINARY_DIR}/scripts
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  file(COPY ${PROJECT_BINARY_DIR}/scripts/in/rtl_src_config.py
    DESTINATION ${PROJECT_BINARY_DIR}/scripts
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  # Add extra project files
  configure_file(${ASE_SCRIPTS_IN}/platform.txt.in
    ${PROJECT_BINARY_DIR}/platform.txt)
  configure_file(${ASE_SCRIPTS_IN}/platform_if_addenda.txt.in
    ${PROJECT_BINARY_DIR}/${ASE_PLATFORM_ABBREV}/sim/platform_if_addenda.txt)
  configure_file(${ASE_SCRIPTS_IN}/platform_if_includes.txt.in
    ${PROJECT_BINARY_DIR}/${ASE_PLATFORM_ABBREV}/sim/platform_if_includes.txt)

  # Configure generic header file
  configure_file(${PLATFORM_IF_DIR}/rtl/platform_if.vh
    ${PROJECT_BINARY_DIR}/platform_if.vh)

  # Define rule to generate platform_afu_top_config.vh
  add_custom_command(OUTPUT "platform_afu_top_config.vh"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${PROJECT_BINARY_DIR}/scripts/afu_platform_config.py --platform_if ${ASE_PLATFORM_ABBREV} --ifc=${ASE_PLATFORM_IF} --sim -v ${ASE_PLATFORM_ABBREV})

  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)
  add_custom_command(OUTPUT "include/platform_dpi.h"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${VLIB_EXECUTABLE} work
    COMMAND ${VLOG_EXECUTABLE} -dpiheader include/platform_dpi.h -f platform.txt -nologo -sv +librescan  -timescale ${ASE_TIMESCALE} -work work +define+${ASE_SIMULATOR} +define+${ASE_PLATFORM} +define+VENDOR_ALTERA +define+TOOL_QUARTUS -novopt ${VLOG_INCLUDES_MENTOR} -l vlog.log
    DEPENDS "platform_afu_top_config.vh")

  add_custom_command (OUTPUT vlog
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${VLOG_EXECUTABLE} -f platform.txt -nologo -sv +librescan  -timescale ${ASE_TIMESCALE} -work work +define+${ASE_SIMULATOR} +define+${ASE_PLATFORM} +define+VENDOR_ALTERA +define+TOOL_QUARTUS -novopt ${VLOG_INCLUDES_MENTOR} -l vlog.log
    DEPENDS "include/platform_dpi.h")

  # Simulation step
  add_custom_target (vlog_setup ALL
    DEPENDS vlog)

endif()
