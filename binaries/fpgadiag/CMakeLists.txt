## Copyright(c) 2017-2023, Intel Corporation
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
        src/nlb.h
        src/nlb_cache_prime.h
        src/nlb_cache_prime.cpp
        src/nlb_stats.h
        src/nlb_stats.cpp
        src/nlb0.h
        src/nlb0.cpp
        src/nlb3.h
        src/nlb3.cpp
        src/nlb7.h
        src/nlb7.cpp
        src/perf_counters.h
        src/perf_counters.cpp
        src/diag_utils.cpp
    LIBS
        opae-c
        opae-cxx-core
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR}
    COMPONENT opaecxxnlb
)

target_include_directories(opae-c++-nlb
    PRIVATE
        ${OPAE_LIB_SOURCE}/c++utils
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
            ${json-c_LIBRARIES}
            ${uuid_LIBRARIES}
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
            ${OPAE_LIB_SOURCE}/c++utils
    )
endfunction()

add_fpgadiag_app(nlb0 src/nlb0_main.cpp toolfpgadiagapps)
add_fpgadiag_app(nlb3 src/nlb3_main.cpp toolfpgadiagapps)
add_fpgadiag_app(nlb7 src/nlb7_main.cpp toolfpgadiagapps)

if (OPAE_WITH_PYBIND11)
    set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/timestamp)
    file(GLOB_RECURSE PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/opae/*)

    set(PYFILES
        pyproject.toml
        setup.py
        mux.json
        nlb0.json
        nlb3.json
        nlb7.json
        )

    set(PYDIST_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)

    foreach(pyfile ${PYFILES})
        configure_file(${pyfile} ${PYDIST_STAGE_DIR}/${pyfile} @ONLY)
    endforeach(pyfile ${PYFILES})

    file(COPY opae DESTINATION ${PYDIST_STAGE_DIR})
    file(COPY src DESTINATION ${PYDIST_STAGE_DIR})

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${Python3_EXECUTABLE} -m pip install --root=${PYDIST_STAGE_DIR}/build .
        COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
        WORKING_DIRECTORY ${PYDIST_STAGE_DIR}
        DEPENDS pyproject.toml setup.py ${PKG_FILES}
    )

    pybind11_add_module(eth_group THIN_LTO ${PYDIST_STAGE_DIR}/src/eth_group.cpp)
    target_link_libraries(eth_group PRIVATE opaeuio)

    add_custom_target(opae.diag-build ALL DEPENDS nlb0 nlb3 nlb7 ${OUTPUT})
    opae_python_install(
        COMPONENT toolfpgadiagapps
        RECORD_FILE fpgadiag-install.txt
	SOURCE_DIR ${PYDIST_STAGE_DIR}
        RPM_PACKAGE tools-extra
    )
endif(OPAE_WITH_PYBIND11)
