## Copyright(c) 2017-2019, Intel Corporation
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

include(packaging)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_CONFIG_H -std=c++11")
include_directories(
    ${OPAE_INCLUDE_DIR}
    ${libjson-c_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OPAE_SDK_SOURCE}/tools/extra/c++utils
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
                       perf_counters.h
                       perf_counters.cpp
                       diag_utils.cpp)

set_install_rpath(opae-c++-nlb)

target_link_libraries(opae-c++-nlb opae-c opae-cxx-core pthread)

set_target_properties(opae-c++-nlb PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})

macro(add_fpgadiag_app target_name sources)
    add_executable(${target_name} ${sources})
    target_link_libraries(${target_name} opae-c++-utils uuid pthread opae-c
        opae-cxx-core opae-c++-nlb)
    set_install_rpath(${target_name})
endmacro(add_fpgadiag_app target_name sources)

add_fpgadiag_app( nlb0    nlb0_main.cpp   )
add_fpgadiag_app( nlb3    nlb3_main.cpp   )
add_fpgadiag_app( nlb7    nlb7_main.cpp   )

set(binaries nlb0
             nlb3
             nlb7)

set(fpgalpbksrc common.py
                fpgalpbk.py)

set(mactestsrc common.py
               mactest.py)

set(fpgastatssrc common.py
               fpgastats.py)

CREATE_PYTHON_EXE(fpgalpbk fpgalpbk ${fpgalpbksrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

CREATE_PYTHON_EXE(mactest mactest ${mactestsrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

CREATE_PYTHON_EXE(fpgastats fpgastats ${fpgastatssrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

install(PROGRAMS fpgadiag
    DESTINATION bin
    COMPONENT toolfpgadiag)

install(TARGETS opae-c++-nlb
   LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
   COMPONENT opaecxxnlb)

install(TARGETS ${binaries}
    RUNTIME DESTINATION bin
    COMPONENT toolfpgadiagapps)
