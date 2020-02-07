## Copyright(c) 2014-2019, Intel Corporation
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

#find_package(UUID REQUIRED)
#include(opae_devpkgs_cmake_install)
#include(ase_add_module)

include_directories(${OPAE_INCLUDE_DIR}
  ${OPAE_SDK_SOURCE}/libopae/src )
include_directories(base/sw)

if(BUILD_ASE_SAMPLES)
  find_package(Quartus)
  find_package(Questa)
endif()

# Prefer GNU99
if ((CMAKE_C_COMPILER_ID STREQUAL "GNU") OR (CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H -std=gnu99")
elseif (NOT CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_C_STANDARD_REQUIRED TRUE)
    set (CMAKE_C_STANDARD "99")
endif ()

######################################################################
# Add the samples ####################################################
######################################################################

set(OPAE_SAMPLES src/opae/samples)
if(BUILD_ASE_SAMPLES)
  # BBB libraries (MPF, DMA, etc)
  include(intel_fpga_bbb)
  Build_Intel_FPGA_BBB()

  add_subdirectory(intg_xeon_nlb)
  add_subdirectory(hello_afu)
  add_subdirectory(hello_mpf_afu)
  if(BUILD_ASE_INTR)
    add_subdirectory(hello_intr_afu)
  endif()
  # add_subdirectory(hello_error_afu)
  # add_subdirectory(hello_dma_afu)
endif()

######################################################################
# Client application #################################################
######################################################################
if(BUILD_ASE)
  add_definitions(-DTEST_TIMEOUT=130000000)
endif()
add_executable(hello_fpga hello_fpga.c)

target_link_libraries(hello_fpga ${libjson-c_LIBRARIES} uuid ${CMAKE_THREAD_LIBS_INIT} opae-c)

if(BUILD_LIBOPAE_C)
  add_executable(hello_events hello_events.c)
  target_link_libraries(hello_events ${libjson-c_LIBRARIES} uuid ${CMAKE_THREAD_LIBS_INIT} opae-c)

  add_executable(object_api object_api.c)
  target_link_libraries(object_api ${libjson-c_LIBRARIES} uuid ${CMAKE_THREAD_LIBS_INIT} opae-c)
endif()

set(SAMPLES_SRC hello_fpga.c hello_events.c object_api.c)

install(FILES ${SAMPLES_SRC}
  DESTINATION ${OPAE_SAMPLES}
  COMPONENT samplesrc)

install(TARGETS hello_fpga
  RUNTIME DESTINATION bin
  COMPONENT samplebin)

if(BUILD_LIBOPAE_C)
  set(TESTS hello_fpga hello_events)
  install(TARGETS ${TESTS}
    DESTINATION bin
    COMPONENT testsopae)
endif()
