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

project(libopae-c)

############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  include(coverage)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

############################################################################
## Add 'libopae-c' library ###################################################
############################################################################
set(SRC
  src/common.c
  src/enum.c
  src/umsg.c
  src/reconf.c
  src/open.c
  src/close.c
  src/reset.c
  src/mmio.c
  src/buffer.c
  src/bitstream.c
  src/hostif.c
  src/event.c
  src/properties.c
  src/log.c
  src/sysfs.c
  src/wsid_list.c
  src/token_list.c
  src/mmap.c
  src/usrclk/user_clk_pgm_uclock.c)

# Define target
add_library(opae-c SHARED ${SRC})
add_dependencies(opae-c copy-common-opae-header-files)
target_link_libraries(opae-c
  m
  safestr
  ${libjson-c_LIBRARIES}
  ${libuuid_LIBRARIES})

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths. Keep current directory private.
target_include_directories(opae-c PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE src)

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_property(SOURCE ${SRC} APPEND PROPERTY COMPILE_FLAGS ${GCOV_COMPILE_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Target properties
set_property(TARGET opae-c PROPERTY C_STANDARD 99)
set_target_properties(opae-c PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  target_link_libraries(opae-c ${GCOV_LINK_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Set debug flags, if required
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DLIBOPAE_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Hide non-public symbols when building release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message("!! Building for release, may break internal tests")
  set_property(TARGET opae-c PROPERTY C_VISIBILITY_PRESET hidden)
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

# Binary install locations
install(TARGETS opae-c
  LIBRARY DESTINATION lib
  COMPONENT opaeclib)


############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_TESTS AND GTEST_FOUND)
    set_target_for_coverage(opae-c bin/gtapi -p)
    add_dependencies(coverage_opae-c gtapi)
  endif(BUILD_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
