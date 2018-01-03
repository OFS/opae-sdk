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

project(libopae-cxx)


#############################################################################
### Add 'libopae-cxx' library ###################################################
#############################################################################
#set(SRC)

## Define target
#add_library(opae-c SHARED ${SRC})
#add_dependencies(opae-c copy-common-opae-header-files)
#target_link_libraries(opae-c
#  m
#  safestr
#  ${libjson-c_LIBRARIES}
#  ${libuuid_LIBRARIES})

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
include_directories(
    ${OPAE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

set(OPAECXX_SRC src/properties.cpp
                src/token.cpp
                src/handle.cpp
                src/dma_buffer.cpp
                src/events.cpp
                src/except.cpp
                src/log.cpp)

add_library(opae-cxx-core SHARED ${OPAECXX_SRC})
target_link_libraries(opae-cxx-core opae-c)
add_executable(simple_example samples/simple_example.cpp)
target_link_libraries(simple_example opae-c ${CMAKE_THREAD_LIBS_INIT} opae-cxx-core )

