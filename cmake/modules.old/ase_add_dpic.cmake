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

cmake_minimum_required(VERSION 2.8.11)

find_package(UUID REQUIRED)
find_package(Threads REQUIRED)
include(coverage)
include(ase_variables_config)

############################################################################
## Fetch tool and script locations #########################################
############################################################################

# Locate sv DPI header files
if(ASE_SIMULATOR STREQUAL "QUESTA")
  find_package(Quartus)
  find_package(Questa)
endif()

# Directories
set(ASE_SERVER_SRC ${ASE_SHARE_DIR}/sw)

# ASE SW file setup
set(ASESW_FILE_LIST
  ${ASE_SERVER_SRC}/tstamp_ops.c
  ${ASE_SERVER_SRC}/ase_ops.c
  ${ASE_SERVER_SRC}/ase_strings.c
  ${ASE_SERVER_SRC}/ipc_mgmt_ops.c
  ${ASE_SERVER_SRC}/ase_shbuf.c
  ${ASE_SERVER_SRC}/protocol_backend.c
  ${ASE_SERVER_SRC}/mqueue_ops.c
  ${ASE_SERVER_SRC}/error_report.c
  ${ASE_SERVER_SRC}/linked_list_ops.c
  ${ASE_SERVER_SRC}/randomness_control.c)

############################################################################
## Define DPI C code #######################################################
############################################################################

function(ase_module_add_dpic name)

  # Get object properties
  get_property(platform_name TARGET ${m} PROPERTY ASE_MODULE_PLATFORM_NAME)
  if(platform_name STREQUAL "intg_xeon")
    set(used_platform "FPGA_PLATFORM_INTG_XEON")
  else()
    set(used_platform "FPGA_PLATFORM_DISCRETE")
  endif()

  add_library(opae-c-ase-server-${name} SHARED
    ${ASESW_FILE_LIST}
    ${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.h)
  target_link_libraries(opae-c-ase-server-${name} rt)

  # Assure position indenpendent code
  set_property(TARGET opae-c-ase-server-${name} PROPERTY POSITION_INDEPENDENT_CODE ON)

if(BUILD_ASE_TESTS)
  target_compile_definitions(opae-c-ase-server-${name} 
	PRIVATE
	_GLIBCXX_USE_CXX11_ABI=0
    SIM_SIDE=1
    SIMULATOR=${ASE_SIMULATOR}
    ${used_platform}
    ASE_LL_VIEW=1
    ASE_MSG_VIEW=1
    ASE_UNIT=1)
else()
  # Add required compilation flags
  target_compile_definitions(opae-c-ase-server-${name}
    PRIVATE
    _GLIBCXX_USE_CXX11_ABI=0
    SIM_SIDE=1
    SIMULATOR=${ASE_SIMULATOR}
    ${used_platform})
endif()

  # Define include directories
  target_include_directories(opae-c-ase-server-${name} PUBLIC
    $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${libsvdpi_INCLUDE_DIRS}>
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

  # Match OPAE ASE client library version
  set_target_properties(opae-c-ase-server-${name} PROPERTIES
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR})

  # Add coverage flags
  if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set_property(SOURCE ${ASESW_FILE_LIST} APPEND PROPERTY COMPILE_FLAGS ${GCOV_COMPILE_FLAGS})
  endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

  # Add coverage flags
  if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_link_libraries(opae-c-ase-server-${name} ${GCOV_LINK_FLAGS})
  endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

endfunction(ase_module_add_dpic name)
