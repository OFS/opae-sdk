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
if(BUILD_ASE)
  add_definitions(-DTARGET_ASE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(
    ${OPAE_INCLUDE_DIR}
    ${OPAE_SDK_SOURCE}/tools/extra/c++utils
    ${OPAE_SDK_SOURCE}/tools/extra/libopae++
    )

add_library(opae-c++-nlb SHARED nlb.h
                       nlb_cache_prime.h
                       nlb_cache_prime.cpp
                       nlb_stats.h
                       nlb_stats.cpp
                       nlb0.h
                       nlb0.cpp
                       nlb3.h
                       nlb3.cpp
                       nlb7.h
                       nlb7.cpp
                       cmdq0.h
                       cmdq0.cpp
                       cmdq3.h
                       cmdq3.cpp
                       cmdq7.h
                       cmdq7.cpp
                       mb1.h
                       mb1.cpp
                       mtnlb.h
                       mtnlb.cpp
                       mtnlb7.h
                       mtnlb7.cpp
                       mtnlb8.h
                       mtnlb8.cpp)

set_install_rpath(opae-c++-nlb)

target_link_libraries(opae-c++-nlb opae-c opae-c++ pthread)

set_target_properties(opae-c++-nlb PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

option(CMDQ3_USE_NUMA "Build cmdq3 with numa support" OFF)
mark_as_advanced(CMDQ3_USE_NUMA)
if (CMDQ3_USE_NUMA)
    find_library(NUMA_LIB numa)
    if (NUMA_LIB)
        add_definitions(-DENABLE_NUMA)
    endif(NUMA_LIB)
endif (CMDQ3_USE_NUMA)

macro(add_fpgadiag_app target_name sources)
    add_executable(${target_name} ${sources})
    target_link_libraries(${target_name} opae-c++-utils uuid pthread opae-c opae-c++ opae-c++-nlb)
    set_install_rpath(${target_name})
endmacro(add_fpgadiag_app target_name sources)

add_fpgadiag_app( mb1     mb1_main.cpp    )
add_fpgadiag_app( nlb0    nlb0_main.cpp   )
add_fpgadiag_app( nlb3    nlb3_main.cpp   )
add_fpgadiag_app( nlb7    nlb7_main.cpp   )
add_fpgadiag_app( mtnlb7  mtnlb7_main.cpp )
add_fpgadiag_app( mtnlb8  mtnlb8_main.cpp )
add_fpgadiag_app( fpgamux mux.cpp         )
add_fpgadiag_app( cmdq0   cmdq0_main.cpp  )
add_fpgadiag_app( cmdq3   cmdq3_main.cpp  )
add_fpgadiag_app( cmdq7   cmdq7_main.cpp  )

set(binaries nlb0
             nlb3
             nlb7
             mtnlb7
             mtnlb8
             fpgamux)

if (NUMA_LIB)
    target_link_libraries(cmdq3 ${NUMA_LIB})
endif(NUMA_LIB)

install(PROGRAMS fpgadiag
    DESTINATION bin
    COMPONENT toolfpgadiag)
install(TARGETS opae-c++-nlb
    LIBRARY DESTINATION lib
    COMPONENT opaecxxnlb)
install(TARGETS ${binaries}
    RUNTIME DESTINATION bin
    COMPONENT toolfpgadiagapps)

