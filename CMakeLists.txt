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

cmake_minimum_required(VERSION 2.8.12)
project(opae-libs)

set(OPAE_VERSION_MAJOR    1 CACHE STRING "OPAE major version" FORCE)
set(OPAE_VERSION_MINOR    4 CACHE STRING "OPAE minor version" FORCE)
set(OPAE_VERSION_REVISION 0 CACHE STRING "OPAE revision version" FORCE)
set(OPAE_VERSION ${OPAE_VERSION_MAJOR}.${OPAE_VERSION_MINOR}.${OPAE_VERSION_REVISION}
    CACHE STRING "OPAE version" FORCE)

set(OPAE_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-libs project" FORCE)
set(OPAE_INCLUDE_PATH ${OPAE_LIBS_ROOT}/include CACHE PATH "Include directory for opae-libs")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_LIBS_ROOT}/cmake/modules")

############################################################################
## Python Interpreter/Build Env  ###########################################
############################################################################
set(OPAE_PYTHON_VERSION 2.7 CACHE STRING "Python version to use for building/distributing pyopae")
set_property(CACHE OPAE_PYTHON_VERSION PROPERTY STRINGS 2.7 3.6 3.5 3.4 3.3)
find_package(PythonInterp ${OPAE_PYTHON_VERSION})
find_package(PythonLibs ${OPAE_PYTHON_VERSION})

option(OPAE_BUILD_TESTS "Enable building of OPAE unit tests" OFF)
mark_as_advanced(OPAE_BUILD_TESTS)

include(OPAE)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
    set(OPAE_LIB_INSTALL_DIR "lib64" CACHE INTERNAL "arch specific library")
else()
    set(OPAE_LIB_INSTALL_DIR "lib" CACHE INTERNAL "arch specific library")
endif()

############################################################################
## RPATH Handling ##########################################################
############################################################################
option(OPAE_INSTALL_RPATH "Enable to create RPATH during install" OFF)
mark_as_advanced(OPAE_INSTALL_RPATH)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
if(OPAE_INSTALL_RPATH)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${OPAE_LIB_INSTALL_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

############################################################################
## Place all executables and libraries under the same directories ##########
############################################################################
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Exe Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH "Lib Build directory" FORCE)

############################################################################
## Conditional Build Steps #################################################
############################################################################
option(OPAE_BUILD_LIBOPAE_CXX "Enable building of C++ bindings around libopae-c." ON)
mark_as_advanced(OPAE_BUILD_LIBOPAE_CXX)

option(OPAE_BUILD_LIBOPAE_PY "Enable building of Python bindings around libopae-cxx-core." ON)
mark_as_advanced(OPAE_BUILD_LIBOPAE_PY)

option(OPAE_BUILD_PYTHON_DIST "Enable building of Python source and binary distribution" OFF)
mark_as_advanced(OPAE_BUILD_PYTHON_DIST)

option(OPAE_ENABLE_MOCK "Enable building of test infrastructure with mock" OFF)
mark_as_advanced(OPAE_ENABLE_MOCK)

option(OPAE_BUILD_SIM "Enable building of the AFU simulation environment" OFF)
mark_as_advanced(OPAE_BUILD_SIM)

option(OPAE_PRESERVE_REPOS "Disable refresh of external repos" OFF)
mark_as_advanced(OPAE_PRESERVE_REPOS)

if(OPAE_BUILD_TESTS)
    option(OPAE_TEST_TAG "Desired branch for opae-test" master)
    mark_as_advanced(OPAE_TEST_TAG)
endif(OPAE_BUILD_TESTS)

if(OPAE_BUILD_SIM)
    option(OPAE_SIM_TAG "Desired branch for opae-sim" master)
    mark_as_advanced(OPAE_SIM_TAG)
endif(OPAE_BUILD_SIM)

############################################################################
## Add Subdirectories ######################################################
############################################################################

opae_add_subdirectory(external)
opae_add_subdirectory(libopae-c)
opae_add_subdirectory(libbitstream)
opae_add_subdirectory(plugins)

if(OPAE_BUILD_LIBOPAE_CXX)
    opae_add_subdirectory(libopaecxx)
endif()

if(OPAE_BUILD_LIBOPAE_PY AND OPAE_BUILD_LIBOPAE_CXX)
    opae_add_subdirectory(pyopae)
endif()

if(OPAE_BUILD_TESTS)
    enable_testing()
    opae_add_subdirectory(tests)
endif(OPAE_BUILD_TESTS)

install(DIRECTORY include/opae
    DESTINATION include
    COMPONENT libopaeheaders)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/config.h.in"
               "${CMAKE_BINARY_DIR}/include/config.h")
