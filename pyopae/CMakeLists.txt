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

set(PYOPAE_SRC opae.cpp
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
	       pyerrors.cpp)

set(PYBIND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pybind11/include)
include_directories(${PYBIND_INCLUDE_DIR}
	            ${PYTHON_INCLUDE_DIRS})
add_library(_opae MODULE ${PYOPAE_SRC})
target_include_directories(_opae
	PRIVATE ${PYBIND11_INCLUDE_DIR}
	PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(_opae PUBLIC opae-c opae-cxx-core)
set_target_properties(_opae PROPERTIES PREFIX ""
				       CXX_VISIBILITY_PRESET "hidden"
		                       COMPILE_FLAGS "-std=c++11"
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

####################################################
## TODO: Re-enable building python wheel package
## Comment out for now
####################################################

#execute_process(COMMAND
#		${PYTHON_EXECUTABLE} -c
#		"import pybind11; print pybind11.get_include().strip()"
#		OUTPUT_VARIABLE PYBIND_INCLUDE_DIR
#		OUTPUT_STRIP_TRAILING_WHITESPACE)
#include_directories(${PYBIND_INCLUDE_DIR}
#	            ${PYTHON_INCLUDE_DIRS})

#include(FindPythonPackage)
#option(USE_VIRTUALENV "Install Python virtualenv in build directory" ON)
#if (USE_VIRTUALENV)
#    set(VIRTUAL_ENV_PACKAGES "nose2;pybind11" CACHE STRING "List of packages to install in virtualenv")
#    FIND_PYTHON_PKG(virtualenv WARNING)
#    if (virtualenv_FOUND AND NOT EXISTS ${CMAKE_BINARY_DIR}/bin/python)
#        message(STATUS "Installing Python virtualenv in ${CMAKE_BINARY_DIR}")
#        execute_process(
#            COMMAND ${PYTHON_EXECUTABLE} -m virtualenv ${CMAKE_BINARY_DIR}
#            OUTPUT_QUIET RESULT_VARIABLE EXIT_CODE)
#    endif (virtualenv_FOUND AND NOT EXISTS ${CMAKE_BINARY_DIR}/bin/python)
#    if(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
#        set(PYTHON_VIRTUALENV ${CMAKE_BINARY_DIR}/bin/python CACHE FILEPATH "Virtual Python Executable")
#        foreach(pkg ${VIRTUAL_ENV_PACKAGES})
#            execute_process(
#                COMMAND ${PYTHON_VIRTUALENV} -m pip install ${pkg}
#                OUTPUT_QUIET)
#        endforeach(pkg ${VIRTUAL_ENV_PACKAGES})
#    else(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
#	set(PYTHON_VIRTUALENV ${PYTHON_EXECUTABLE} CACHE FILEPATH "Python Executable")
#    endif(EXISTS ${CMAKE_BINARY_DIR}/bin/python)
#endif (USE_VIRTUALENV)


#add_library(test_pybind_compile MODULE test_pybind_compile.cpp)
#set_property(TARGET test_pybind_compile PROPERTY CXX_STANDARD 11)
#set(SETUP_INCLUDE_DIRS "${OPAE_INCLUDE_DIR}:${PYBIND_INCLUDE_DIR}")
#
#
#include(create_python_dist)
#create_python_dist(
#    TARGET pyopae-dist
#    BUILD_WHEEL
#    PACKAGE opae
#    COPY_FILES ${PYOPAE_SRC}
#	       test_pyopae.py
#    BUILD_COMMAND build_ext
#    BUILD_INCLUDE_DIRS "${SETUP_INCLUDE_DIRS}"
#    BUILD_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH}
#    DEPENDS opae-c opae-cxx-core test_pybind_compile
#)




