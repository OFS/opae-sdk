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

project(libxfpga)

############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  include(coverage)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

############################################################################
## Add 'libxfpga' library ###################################################
############################################################################
set(SRC
  common.c
  enum.c
  error.c
  umsg.c
  reconf.c
  open.c
  close.c
  reset.c
  mmio.c
  buffer.c
  bitstream.c
  hostif.c
  event.c
  properties.c
  init.c
  opae_ioctl.c
  sysfs.c
  wsid_list.c
  token_list.c
  mmap.c
  version.c
  userclk.c
  usrclk/user_clk_pgm_uclock.c
  plugin.c
  sysobject.c
  manage.c
  metrics/metrics.c
  metrics/metrics_utils.c
  metrics/afu_metrics.c
  metrics/vector.c)

# Define target
add_library(xfpga MODULE ${SRC})
#add_dependencies(xfpga copy-common-opae-header-files)
target_link_libraries(xfpga
  m
  safestr
  dl
  ${libjson-c_LIBRARIES}
  ${libuuid_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  opae-c)

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths. Keep current directory private.
target_include_directories(xfpga PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${OPAE_SDK_SOURCE}/libopae)

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_property(SOURCE ${SRC} APPEND PROPERTY COMPILE_FLAGS ${GCOV_COMPILE_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Target properties
set_property(TARGET xfpga PROPERTY C_STANDARD 99)
set_target_properties(xfpga PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

# Add coverage flags
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  target_link_libraries(xfpga ${GCOV_LINK_FLAGS})
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

# Set debug flags, if required
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DLIBOPAE_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

# Hide non-public symbols when building release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message("!! Building for release, may break internal tests")
  set_property(TARGET xfpga PROPERTY C_VISIBILITY_PRESET hidden)
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

install(TARGETS xfpga
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT opaeclib)

#get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS) 
#if ("${LIB64}" STREQUAL "TRUE")
#    install(TARGETS xfpga
#      LIBRARY DESTINATION lib64
#      COMPONENT opaeclib)
#else()
#    install(TARGETS xfpga
#      LIBRARY DESTINATION lib
#      COMPONENT opaeclib)
#endif() 


############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_TESTS AND GTEST_FOUND)
    set_target_for_coverage(xfpga bin/gtapi -p)
    add_dependencies(coverage_xfpga gtapi)
  endif(BUILD_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
