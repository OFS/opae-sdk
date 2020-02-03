## Copyright(c) 2018-2020, Intel Corporation
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

cmake_minimum_required(VERSION 2.8.12)
project(pyopae)

set(PYOPAE_PYBIND11_VERSION "2.2.4")

set(PYOPAE_SRC
    opae.cpp
    pycontext.h
    pycontext.cpp
    pyproperties.h
    pyproperties.cpp
    pyhandle.h
    pyhandle.cpp
    pytoken.h
    pytoken.cpp
    pyshared_buffer.h
    pyshared_buffer.cpp
    pyevents.h
    pyevents.cpp
    pyerrors.h
    pyerrors.cpp
    pysysobject.h
    pysysobject.cpp
)

set(PYBIND11_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pybind11/include)

opae_add_module_library(TARGET _opae
    SOURCE ${PYOPAE_SRC}
    LIBS
        safestr
        opae-c
        opae-cxx-core
)

target_include_directories(_opae
    PRIVATE ${PYBIND11_INCLUDE_DIR}
    PRIVATE ${PYTHON_INCLUDE_DIRS})

set_target_properties(_opae
    PROPERTIES PREFIX ""
    CXX_VISIBILITY_PRESET "hidden"
    LINK_FLAGS "-std=c++11"
    LIBRARY_OUTPUT_DIRECTORY
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/opae/fpga
)

add_custom_command(TARGET _opae
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/opae/__init__.py
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/opae
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/opae/fpga/__init__.py
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/opae/fpga
    COMMENT "Copying namespace package files")

add_custom_command(TARGET _opae
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pyopae.py
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}
    COMMENT "Copying Python test files")

if (BUILD_PYTHON_DIST)
    set(SETUP_INCLUDE_DIRS "${OPAE_INCLUDE_DIR}:${PYBIND_INCLUDE_DIR}")

    set(PYFILES
        setup.py
        opae/__init__.py
        opae/fpga/__init__.py
        test_pyopae.py
        )

    set (PYDIST_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)

    configure_file(README.md ${PYDIST_STAGE_DIR}/README.md @ONLY)
    foreach(pyfile ${PYFILES})
        configure_file(${pyfile} ${PYDIST_STAGE_DIR}/${pyfile} @ONLY)
    endforeach(pyfile ${PYFILES})

    foreach(cppfile ${PYOPAE_SRC})
        file(COPY ${cppfile} DESTINATION ${PYDIST_STAGE_DIR})
    endforeach(cppfile ${PYOPAE_SRC})
    file(COPY MANIFEST.in DESTINATION ${PYDIST_STAGE_DIR})
    add_custom_target(pyopae-dist
        COMMAND ${PYTHON_EXECUTABLE} setup.py sdist
        COMMAND ${PYTHON_EXECUTABLE} setup.py build_ext
        --include-dirs=${OPAE_INCLUDE_DIR}
        --library-dirs=${LIBRARY_OUTPUT_PATH}
        COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel
        DEPENDS ${PYFILES} ${PYOPAE_SRC}
        WORKING_DIRECTORY ${PYDIST_STAGE_DIR}
        COMMENT "Building Python distrubutions")
endif (BUILD_PYTHON_DIST)
