## Copyright(c) 2018-2023, Intel Corporation
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

opae_add_module_library(TARGET _opae
    SOURCE ${PYOPAE_SRC}
    LIBS
        opae-c
        opae-cxx-core
)

target_include_directories(_opae
    PRIVATE ${pybind11_INCLUDE_DIRS}
    PRIVATE ${Python3_INCLUDE_DIRS})

set_target_properties(_opae
    PROPERTIES PREFIX ""
    CXX_VISIBILITY_PRESET "hidden"
    LINK_FLAGS "-std=c++11"
    LIBRARY_OUTPUT_DIRECTORY
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/opae/fpga
)

set(PYPKGFILES
  opae/fpga/__init__.py
  opae/fpga/dfh.py
  opae/fpga/feature.py
  opae/fpga/mailbox.py
  opae/fpga/hexview.py
  opae/fpga/pcie/__init__.py
  opae/fpga/pcie/address.py
  opae/fpga/tools/__init__.py
  opae/fpga/tools/opae_mem.py
)

foreach(pypkgfile ${PYPKGFILES})
    get_filename_component(pypkgdir
        ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/${pypkgfile} DIRECTORY)
    add_custom_command(TARGET _opae
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${pypkgdir}
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/${pypkgfile}
        ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}/opae/fpga
        COMMENT "Copying namespace package files")
endforeach(pypkgfile ${PYPKGFILES})

add_custom_command(TARGET _opae
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pyopae.py
    ${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}
    COMMENT "Copying Python test files")

if (OPAE_BUILD_PYTHON_DIST)

    set(PYFILES
        pyproject.toml
        setup.py
        ${PYPKGFILES}
        test_pyopae.py
        )

    set(PYDIST_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)

    configure_file(README.md ${PYDIST_STAGE_DIR}/README.md @ONLY)
    foreach(pyfile ${PYFILES})
        configure_file(${pyfile} ${PYDIST_STAGE_DIR}/${pyfile} @ONLY)
    endforeach(pyfile ${PYFILES})

    foreach(cppfile ${PYOPAE_SRC})
        file(COPY ${cppfile} DESTINATION ${PYDIST_STAGE_DIR})
    endforeach(cppfile ${PYOPAE_SRC})

    add_custom_target(pyopae-dist
        COMMAND ${Python3_EXECUTABLE} -m pip install --root=${PYDIST_STAGE_DIR}/build .
        DEPENDS ${PYFILES} ${PYOPAE_SRC}
        WORKING_DIRECTORY ${PYDIST_STAGE_DIR}
        COMMENT "Building Python distrubutions")

    opae_python_install(
        COMPONONENT opae.fpga
        RECORD_FILE opae.fpga-install.txt
        SOURCE_DIR ${PYDIST_STAGE_DIR}
    )
endif (OPAE_BUILD_PYTHON_DIST)
