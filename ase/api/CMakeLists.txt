## Copyright(c) 2018, Intel Corporation
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

project("opae-c-ase")
find_package(Threads REQUIRED)

add_definitions(-DHAVE_CONFIG_H=1)
set(API_DIR ${PROJECT_SOURCE_DIR})
set(ASEAPI_SRC
  ${API_DIR}/../sw/tstamp_ops.c
  ${API_DIR}/../sw/ase_ops.c
  ${API_DIR}/../sw/ase_strings.c
  ${API_DIR}/../sw/ase_host_memory.c
  ${API_DIR}/../sw/app_backend.c
  ${API_DIR}/../sw/mqueue_ops.c
  ${API_DIR}/../sw/error_report.c
  ${API_DIR}/src/common.c
  ${API_DIR}/src/buffer.c
  ${API_DIR}/src/close.c
  ${API_DIR}/src/enum.c
  ${API_DIR}/src/event.c
  ${API_DIR}/src/manage.c
  ${API_DIR}/src/reconf.c
  ${API_DIR}/src/mmio.c
  ${API_DIR}/src/open.c
  ${API_DIR}/src/umsg.c
  ${API_DIR}/src/version.c
  ${API_DIR}/src/wsid_list.c
  ${API_DIR}/src/error.c)

add_library(opae-c-ase SHARED ${ASEAPI_SRC})
target_link_libraries(opae-c-ase
  ${libjson-c_LIBRARIES}
  ${libuuid_LIBRARIES}
  ${librt_LIBRARIES})

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths. Keep current directory private.
target_include_directories(opae-c-ase PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE src
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/../include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../sw>)

set_target_properties(opae-c-ase PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

install(TARGETS opae-c-ase
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT opaecase)
