## Copyright(c) 2018, Intel Corporation
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

# Enable ExternalProject CMake module
include(ExternalProject)

ExternalProject_Add(
  pybind11
  GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
  GIT_TAG "v2.2.2"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  PREFIX ${CMAKE_BINARY_DIR}/ext/pybind11
  INSTALL_DIR ${CMAKE_BINARY_DIR}/ext
)

ExternalProject_Get_Property(pybind11 source_dir binary_dir)

# test if we can compile with pybind11
find_package(PythonLibs 2.7)
include_directories(${source_dir}/include
	            ${PYTHON_INCLUDE_DIRS})
file(WRITE test_pybind_compile.cpp "#include <pybind11/pybind11.h>")
add_library(test_pybind_compile MODULE test_pybind_compile.cpp)
set_property(TARGET test_pybind_compile PROPERTY CXX_STANDARD 11)

include(FindPythonPackage)
option(USE_VIRTUALENV "Install Python virtualenv in build directory" ON)
if (USE_VIRTUALENV)
    set(VIRTUAL_ENV_PACKAGES "nose2" CACHE STRING "List of packages to install in virtualenv")
    FIND_PYTHON_PKG(virtualenv WARNING)
    if (virtualenv_FOUND AND NOT EXISTS ${CMAKE_BINARY_DIR}/bin/python)
        message(STATUS "Installing Python virtualenv in ${CMAKE_BINARY_DIR}")
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} -m virtualenv ${CMAKE_BINARY_DIR}
            OUTPUT_QUIET RESULT_VARIABLE EXIT_CODE)
    endif (virtualenv_FOUND AND NOT EXISTS ${CMAKE_BINARY_DIR}/bin/python)
    if(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
        set(PYTHON_VIRTUALENV ${CMAKE_BINARY_DIR}/bin/python CACHE FILEPATH "Virtual Python Executable")
        foreach(pkg ${VIRTUAL_ENV_PACKAGES})
            execute_process(
                COMMAND ${PYTHON_VIRTUALENV} -m pip install ${pkg}
                OUTPUT_QUIET)
        endforeach(pkg ${VIRTUAL_ENV_PACKAGES})
    else(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
	set(PYTHON_VIRTUALENV ${PYTHON_EXECUTABLE} CACHE FILEPATH "Python Executable")
    endif(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
endif (USE_VIRTUALENV)
include(create_python_dist)
create_python_dist(
    TARGET pyopae
    BUILD_WHEEL
    MODULE accel.py
    COPY_FILES opae.cpp test_pyopae.py diag.py
    BUILD_COMMAND build_ext
    BUILD_INCLUDE_DIRS "${source_dir}/include:${OPAE_INCLUDE_DIR}"
    BUILD_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH}
    DEPENDS pybind11 opae-c opae-cxx-core
)





