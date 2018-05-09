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

include_directories(${OPAE_INCLUDE_DIR}
                    ${OPAE_SDK_SOURCE}/libopae/src )

set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")

find_package(UUID REQUIRED)

add_executable(hello_fpga hello_fpga.c)
if(BUILD_LIBOPAE_C)
  target_link_libraries(hello_fpga json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c)
else()
  target_link_libraries(hello_fpga json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c-ase)
endif()

if(BUILD_LIBOPAE_C)
  add_executable(hello_events hello_events.c)
  target_link_libraries(hello_events json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c)
endif()

set(SAMPLES_SRC hello_fpga.c hello_events.c)

install(FILES ${SAMPLES_SRC}
        DESTINATION src/opae
        COMPONENT samplesrc)
