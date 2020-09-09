## Copyright(c) 2017-2020, Intel Corporation
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

opae_add_shared_library(TARGET opae-c++-nlb
    SOURCE
        nlb.h
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
        diag_utils.cpp
    LIBS
        opae-c
        opae-cxx-core 
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR}
    COMPONENT opaecxxnlb
)

target_include_directories(opae-c++-nlb
    PRIVATE
        ${OPAE_SDK_SOURCE}/tools/extra/c++utils
)

set_target_properties(opae-c++-nlb
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

function(add_fpgadiag_app target source comp)
    opae_add_executable(TARGET ${target}
        SOURCE ${source}
        LIBS
            opae-c
            opae-cxx-core
            opae-c++-utils
            opae-c++-nlb
            ${libjson-c_LIBRARIES}
            ${libuuid_LIBRARIES}
        COMPONENT ${comp}
    )
    set_target_properties(${target}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )
    target_include_directories(${target}
        PRIVATE
            ${OPAE_SDK_SOURCE}/tools/extra/c++utils
    )
endfunction()

add_fpgadiag_app(nlb0 nlb0_main.cpp toolfpgadiagapps)
add_fpgadiag_app(nlb3 nlb3_main.cpp toolfpgadiagapps)
add_fpgadiag_app(nlb7 nlb7_main.cpp toolfpgadiagapps)

set(fpgalpbksrc common.py
                fpgalpbk.py)

set(mactestsrc common.py
               mactest.py)

set(fpgastatssrc common.py
               fpgastats.py)

set(fvlbypasssrc common.py
               fpgastats.py
               fpgalpbk.py
               fvlbypass.py)

set(fecmodesrc common.py
               fecmode.py)

set(fpgamacsrc common.py
               fpgamac.py)

find_package(PythonLibs)
include_directories(${PYTHON_INCLUDE_DIRS})

target_include_directories(_opae
    PRIVATE ${PYBIND11_INCLUDE_DIR}
    PRIVATE ${PYTHON_INCLUDE_DIRS})


try_compile(SUPPORTS_VFIO
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_vfio.c
    OUTPUT_VARIABLE TRY_COMPILE_OUTPUT
)

if (SUPPORTS_VFIO)
    pybind11_add_module(eth_group eth_group.cpp)
else(SUPPORTS_VFIO)
    message(WARNING
        "Could not compile VFIO. See errors in vfio_errors.txt")
    file(WRITE ${CMAKE_BINARY_DIR}/vfio_errors.txt ${TRY_COMPILE_OUTPUT})
endif(SUPPORTS_VFIO)


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

CREATE_PYTHON_EXE(fvlbypass fvlbypass ${fvlbypasssrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

CREATE_PYTHON_EXE(fecmode fecmode ${fecmodesrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

CREATE_PYTHON_EXE(fpgamac fpgamac ${fpgamacsrc})
install(PROGRAMS ${PACKAGER_BIN}
    DESTINATION bin
    COMPONENT toolfpgadiagapps)

install(PROGRAMS fpgadiag
    DESTINATION bin
    COMPONENT toolfpgadiag)
