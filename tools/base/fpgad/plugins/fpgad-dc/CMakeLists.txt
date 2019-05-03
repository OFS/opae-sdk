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

project(libfpgad-dc)

set(SRC
  fpgad-dc.c
)

add_library(fpgad-dc MODULE ${SRC})

target_include_directories(fpgad-dc PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae
  PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
  PRIVATE ${OPAE_SDK_SOURCE}/tools/base
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${libjson-c_INCLUDE_DIRS})

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_property(SOURCE ${SRC} APPEND PROPERTY COMPILE_FLAGS ${GCOV_COMPILE_FLAGS})
  target_link_libraries(fpgad-dc ${GCOV_LINK_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Target properties
set_property(TARGET fpgad-dc PROPERTY C_STANDARD 99)

target_link_libraries(fpgad-dc
  safestr
  opae-c
  fpgad-api
  ${CMAKE_THREAD_LIBS_INIT}
  ${libjson-c_LIBRARIES})

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

install(TARGETS fpgad-dc
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT toolfpgad_dc)
