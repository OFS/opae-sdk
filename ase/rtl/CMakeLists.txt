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
set(PLATFORM_IF_DIR    ${CMAKE_SOURCE_DIR}/platforms/platform_if)
set(PLATFORM_IF_RTL    ${PLATFORM_IF_DIR}/rtl/device_if)
set(PLATFORM_SHARE_DIR ${CMAKE_SOURCE_DIR}/platforms)

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
  configure_file(${PROJECT_SOURCE_DIR}/../scripts/in/afu_platform_config.py.in
    ${CMAKE_BINARY_DIR}/scripts/in/afu_platform_config.py @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/../scripts/in/afu_synth_setup_std.py.in
    ${CMAKE_BINARY_DIR}/scripts/afu_synth_setup.py @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/../scripts/in/rtl_src_config.py.in
    ${CMAKE_BINARY_DIR}/scripts/rtl_src_config.py @ONLY)

  configure_file(${ASE_SERVER_RTL}/platform.txt.in
    ${PROJECT_BINARY_DIR}/platform.txt)
  configure_file(${ASE_SERVER_RTL}/platform_if_addenda.txt.in
    ${PROJECT_BINARY_DIR}/intg_xeon/sim/platform_if_addenda.txt)
  configure_file(${ASE_SERVER_RTL}/platform_if_includes.txt.in
    ${PROJECT_BINARY_DIR}/intg_xeon/sim/platform_if_includes.txt)

  configure_file(${PLATFORM_IF_DIR}/rtl/platform_if.vh
    ${PROJECT_BINARY_DIR}/platform_if.vh)

  add_custom_command(OUTPUT "platform_afu_top_config.vh"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${CMAKE_BINARY_DIR}/scripts/in/afu_platform_config.py --platform_if intg_xeon --ifc=ccip_std_afu --sim -v intg_xeon)

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
  add_custom_target (tb_mag_top ALL
    # COMMAND vsim -c tb_mag_top  +access+rw -l simv.log -work worklib -do "run -all; quit"
    DEPENDS vlog)

endif()
# questa_build: sw_build
#   @echo "############################################################"
#   @echo "#                                                          #"
#   @echo "#              QuestaSim-GCC build initiated               #"
#   @echo "#                                                          #"
#   @echo "############################################################"
#   cd $(WORK) ; vlib $(WORK) ; vmap work $(WORK) ; cd -
# ifeq ($(ASE_DISCRETE_EMIF_MODEL), EMIF_MODEL_ADVANCED)
#   cp -f $(ASE_MEM_SRC)/*.hex $(ASE_WORKDIR)
# endif
#   cd $(WORK) ; vlog $(MENT_VLOG_OPT) $(QSIM_MODEL_VLOG_OPT) $(ASEHW_FILE_LIST) $(ASE_PLATFORM_FILE_LIST) -l vlog-ase.log ; cd -
# ifdef DUT_VHD_SRC_LIST
#   cd $(WORK) ; vcom $(MENT_VCOM_OPT) -f $(DUT_VHD_SRC_LIST) -l vcom-afu.log ; cd -
# endif
# ifdef DUT_VLOG_SRC_LIST
#   cd $(WORK) ; vlog $(MENT_VLOG_OPT) $(ASE_PLATFORM_INC) -f $(DUT_VLOG_SRC_LIST) -l vlog-afu.log ; cd -
# endif

# ## Build Simulator objects ##
# build:
# ifeq ($(SIMULATOR), VCS)
#   make vcs_build
# else
#   ifeq ($(SIMULATOR), QUESTA)
#   make questa_build
#   else
#   @echo "############################################################"
#   @echo "#         SIMULATOR=$(SIMULATOR) not supported             #"
#   @echo "############################################################"
#   endif
# endif

# ## Run ASE Simulator ##
# sim: check
# ifeq ($(SIMULATOR), VCS)
#   cd $(ASE_WORKDIR) ; ./ase_simv $(SNPS_SIM_OPT) +CONFIG=$(ASE_CONFIG) +SCRIPT=$(ASE_SCRIPT) ; cd -
# else
#   ifeq ($(SIMULATOR), QUESTA)
#         ifeq ($(ASE_DISCRETE_EMIF_MODEL), EMIF_MODEL_BASIC)
#                 # recompile memory model to keep Questa happy
#       cd $(WORK) ; vlog $(MENT_VLOG_OPT) $(QUARTUS_HOME)/../ip/altera/sopc_builder_ip/verification/altera_avalon_mm_slave_bfm/altera_avalon_mm_slave_bfm.sv -l vlog-ase.log ; cd -
#         endif
#   cd $(ASE_WORKDIR) ; vsim $(MENT_VSIM_OPT) +CONFIG=$(ASE_CONFIG) +SCRIPT=$(ASE_SCRIPT) $(ASE_TOP) ; cd -
#   else
#   @echo "############################################################"
#   @echo "#         SIMULATOR=$(SIMULATOR) not supported             #"
#   @echo "############################################################"
#   endif
# endif

# # Open Wave file
# wave: check
# ifeq ($(SIMULATOR), VCS)
#   dve -vpd `find . -name inter.vpd` || dve -vpd `find . -name inter.vpd` -full64
# else
#   ifeq ($(SIMULATOR), QUESTA)
#   vsim -view `find . -name vsim.wlf`
#   endif
# endif
