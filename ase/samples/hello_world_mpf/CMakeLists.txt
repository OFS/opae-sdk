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

project("opae-c-ase-samples-hello_world")
include(ase_add_modelsim)

######################################################################
# ASE module definition ##############################################
######################################################################

set(hello_world_SOURCES
  hw/sources.txt
  hw/base_addenda.txt
  hw/cci_mpf_hello.json)
ase_add_modelsim_module(hello_world_mpf
  ${hello_world_SOURCES})
file(COPY ${PROJECT_SOURCE_DIR}/hw/rtl
  DESTINATION ${PROJECT_BINARY_DIR}/hw)

# Set platform
ase_module_set_platform_name(hello_world_mpf intg_xeon)

# Add definitions
ase_module_add_definitions(hello_world_mpf NUM_AFUS=1)
ase_module_add_definitions(hello_world_mpf FPGA_PLATFORM_INTG_XEON)

# Configure files and finalize target rules
ase_finalize_modelsim_module_linking(hello_world_mpf)

######################################################################
# Installation files #################################################
######################################################################

if(NOT ASE_POST_INSTALL)
  install(FILES ${PROJECT_BINARY_DIR}/sources.txt
    DESTINATION ${ASE_SAMPLES}/hello_world_mpf/hw
    COMPONENT asesamples)

  install(FILES ${PROJECT_BINARY_DIR}/hw/cci_mpf_hello.json
    DESTINATION ${ASE_SAMPLES}/hello_world_mpf/hw
    COMPONENT asesamples)

  install(DIRECTORY ${PROJECT_BINARY_DIR}/hw/rtl
    DESTINATION ${ASE_SAMPLES}/hello_world_mpf/hw
    COMPONENT asesamples)
endif()

######################################################################
# Testing application ################################################
######################################################################

include_directories(${OPAE_INCLUDE_DIR}
  ${OPAE_SDK_SOURCE}/libopae/src
  ${MPF_INCLUDE_DIRS})

add_executable(cci_hello_world_mpf
  ${ASE_SHARE_DIR}/samples/base/sw/opae_svc_wrapper.cpp
  sw/cci_mpf_hello.cpp)
if(BUILD_LIBOPAE_C)
  target_link_libraries(cci_hello_world_mpf json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c)
else()
  target_link_libraries(cci_hello_world_mpf json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c-ase)
endif()

######################################################################
# Test coverage ######################################################
######################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set(COVERAGE_EXTRA 1)
  set(COVERAGE_EXTRA_COMPONENTS " cci_hello_world_mpf intel-fpga-bbb")
  set_target_for_coverage_local(opae-c-ase-server-hello_world_mpf ${PROJECT_BINARY_DIR}/ase_server.sh)
endif()
