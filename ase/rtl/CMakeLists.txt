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

# CCIP base RTL code
configure_file(${PROJECT_SOURCE_DIR}/sources_abs.txt.in
  ${PROJECT_BINARY_DIR}/sources_abs.txt)
install(FILES ${PROJECT_BINARY_DIR}/sources_abs.txt
  DESTINATION ${ASE_SHARE_DIR}/rtl
  COMPONENT aseextra)

# find afu_sim_setup
find_program(AFU_PLATFORM_CONFIG
  NAMES
  afu_platform_config
  PATHS
  ${CMAKE_SOURCE_DIR}/platforms/scripts
  /usr/bin
  /usr/local/bin
  /opt/local/bin
  DOC "AFU platform configuration utility")

# Directories
set(ASE_SERVER_RTL ${PROJECT_SOURCE_DIR})

## Verilog compilation
set(VLOG_SRC_LIST
  vlog_files.list
  CACHE STRING "Project file listing all SystemVerilog source code for the AFU")

set(ASE_PKG_FILE ${ASE_SERVER_RTL}/ase_pkg.sv)
set(PLATFORM_PKG_FILE ${PLATFORM_IF_RTL}/ccip_if_pkg.sv)

if(ASE_SIMULATOR STREQUAL "QUESTA")

endif()
