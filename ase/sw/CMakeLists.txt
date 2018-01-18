## Copyright(c) 2017, Intel Corporation
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

project("opae-c-ase-server")
find_package(Threads REQUIRED)

# Directories
set(ASE_SERVER_SRC  ${PROJECT_SOURCE_DIR})

# ASE SW file setup
set(ASESW_FILE_LIST
  ${ASE_SERVER_SRC}/tstamp_ops.c
  ${ASE_SERVER_SRC}/ase_ops.c
  ${ASE_SERVER_SRC}/ipc_mgmt_ops.c
  ${ASE_SERVER_SRC}/mem_model.c
  ${ASE_SERVER_SRC}/protocol_backend.c
  ${ASE_SERVER_SRC}/mqueue_ops.c
  ${ASE_SERVER_SRC}/error_report.c
  ${ASE_SERVER_SRC}/linked_list_ops.c
  ${ASE_SERVER_SRC}/randomness_control.c)

add_library(opae-c-ase-server SHARED ${ASESW_FILE_LIST})
target_link_libraries(opae-c-ase-server rt safestr)

# Assure library is relocatable
set_property(TARGET opae-c-ase-server PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(opae-c-ase-server
  PRIVATE
  SIM_SIDE=1
  SIMULATOR=${ASE_SIMULATOR}
  ASE_PLATFORM=${ASE_PLATFORM})

if(ASE_SIMULATOR STREQUAL "QUESTA")
  find_package(Questa)

  target_include_directories(opae-c-ase-server PUBLIC
    $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${libsvdpi_INCLUDE_DIRS}>
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>)

  set_target_properties(opae-c-ase-server PROPERTIES
    VERSION ${INTEL_FPGA_API_VERSION}
    SOVERSION ${INTEL_FPGA_API_VER_MAJOR})
endif()
