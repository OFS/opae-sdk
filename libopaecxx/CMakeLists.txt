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

set(OPAECXXCORE_SRC src/properties.cpp
                    src/token.cpp
                    src/handle.cpp
                    src/shared_buffer.cpp
                    src/events.cpp
                    src/except.cpp
                    src/errors.cpp
                    src/sysobject.cpp
                    src/version.cpp)

add_library(opae-cxx-core SHARED ${OPAECXXCORE_SRC})
target_link_libraries(opae-cxx-core uuid)

add_executable(hello_cxxcore samples/hello_fpga-1.cpp)
target_link_libraries(hello_cxxcore opae-c ${CMAKE_THREAD_LIBS_INIT} opae-cxx-core )


# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths. Keep current directory private.
target_include_directories(opae-cxx-core PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

############################################################################
## Create config_int.h #########################################################
############################################################################

configure_file ("${CMAKE_SOURCE_DIR}/cmake/config/config.h.in"
                "${PROJECT_BINARY_DIR}/include/config_int.h" )

set_install_rpath(opae-cxx-core)
install(TARGETS opae-cxx-core
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT opaecxxcorelib)
