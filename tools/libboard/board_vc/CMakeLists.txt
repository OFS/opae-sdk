## Copyright(c) 2019, Intel Corporation
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

project(libboard_vc)

set(SRC
    board_vc.c
    )

add_library(board_vc MODULE ${SRC})

target_include_directories(board_rc PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_property(SOURCE ${SRC} APPEND PROPERTY COMPILE_FLAGS ${GCOV_COMPILE_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Target properties
set_property(TARGET board_vc PROPERTY C_STANDARD 99)
set_target_properties(board_vc PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  target_link_libraries(board_vc ${GCOV_LINK_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

target_link_libraries(board_vc
  safestr
  opae-c
  ${CMAKE_THREAD_LIBS_INIT})

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

install(TARGETS board_vc
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT opaeclib)

