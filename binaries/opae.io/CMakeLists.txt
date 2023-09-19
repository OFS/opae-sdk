## Copyright(c) 2020-2023, Intel Corporation
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

if (PLATFORM_SUPPORTS_VFIO AND OPAE_WITH_PYBIND11 AND OPAE_WITH_LIBEDIT)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    set(PYBIND11_CPP_STANDARD -std=c++11)

    add_executable(opae.io main.cpp)

    if (NOT (CMAKE_VERSION VERSION_LESS 3.0))
        target_link_libraries(opae.io PRIVATE pybind11::embed dl util ${libedit_LIBRARIES} opaevfio)
    else()
        target_link_libraries(opae.io PRIVATE ${Python3_LIBRARIES} dl util ${libedit_LIBRARIES} opaevfio)
    endif()

    if (libedit_IMPORTED)
        add_dependencies(opae.io libedit_IMPORT)
    endif(libedit_IMPORTED)

    target_include_directories(opae.io
        PUBLIC ${OPAE_INCLUDE_DIR}
        PRIVATE ${Python3_INCLUDE_DIRS}
        PRIVATE ${pybind11_ROOT}/include
        PRIVATE ${libedit_INCLUDE_DIRS}
    )
    target_compile_definitions(opae.io
        PRIVATE LIBVFIO_EMBED
    )
    install(TARGETS opae.io
            RUNTIME DESTINATION bin
            COMPONENT opae.io
    )

    # Build and install opae.io Python package

    set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/timestamp)
    file(GLOB_RECURSE PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/opae/*)

    set(PYFILES
        pyproject.toml
        setup.py
        )

    set(PYDIST_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)

    foreach(pyfile ${PYFILES})
        configure_file(${pyfile} ${PYDIST_STAGE_DIR}/${pyfile} @ONLY)
    endforeach(pyfile ${PYFILES}) 

    file(COPY opae DESTINATION ${PYDIST_STAGE_DIR})
    file(COPY scripts DESTINATION ${PYDIST_STAGE_DIR})

    foreach(cppfile vfiobindings.cpp)
        file(COPY ${cppfile} DESTINATION ${PYDIST_STAGE_DIR})
    endforeach(cppfile ${PYOPAE_SRC})

    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${Python3_EXECUTABLE} -m pip install --root=${PYDIST_STAGE_DIR}/build .
        COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
        WORKING_DIRECTORY ${PYDIST_STAGE_DIR}
        DEPENDS ${PYFILES} ${PKG_FILES}
    )

    add_custom_target(opae.io-build ALL DEPENDS opaevfio opae.io ${OUTPUT})

    opae_python_install(
        COMPONENT opae.io
        RECORD_FILE opae.io-install.txt
	SOURCE_DIR ${PYDIST_STAGE_DIR}
        RPM_PACKAGE devel
    )

endif(PLATFORM_SUPPORTS_VFIO AND OPAE_WITH_PYBIND11 AND OPAE_WITH_LIBEDIT)
