#!/usr/bin/cmake -P
## Copyright(c) 2022, Intel Corporation
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

# - Try to find pybind11
# Once done, this will define
#
#  pybind11_FOUND - system has pybind11
#  pybind11_ROOT - base dir for pybind11
#  pybind11_INCLUDE_DIRS - the pybind11 include directories
#  pybind11_DEFINITIONS - preprocessor defines for pybind11

find_package(PkgConfig)
pkg_check_modules(PC_PYBIND11 QUIET pybind11)

execute_process(COMMAND pkg-config --cflags-only-I pybind11 --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE PYBIND11_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other pybind11 --silence-errors
    OUTPUT_VARIABLE PYBIND11_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (PYBIND11_PKG_CONFIG_DEFINITIONS)
    set(pybind11_DEFINITIONS ${PYBIND11_PKG_CONFIG_DEFINITIONS})
else(PYBIND11_PKG_CONFIG_DEFINITIONS)
    set(pybind11_DEFINITIONS "")
endif(PYBIND11_PKG_CONFIG_DEFINITIONS)

find_path(pybind11_ROOT NAMES include/pybind11/pybind11.h)

find_path(pybind11_INCLUDE_DIRS
    NAMES pybind11/pybind11.h
    HINTS ${pybind11_ROOT}/include
    PATHS ${pybind11_ROOT}/include
    ${PYBIND11_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pybind11
    FOUND_VAR
        pybind11_FOUND
    REQUIRED_VARS
        pybind11_ROOT pybind11_INCLUDE_DIRS
)

set(pybind11_FOUND ${pybind11_FOUND} CACHE BOOL "pybind11 status" FORCE)
set(pybind11_ROOT ${pybind11_ROOT} CACHE PATH "base dir for pybind11" FORCE)
set(pybind11_INCLUDE_DIRS ${pybind11_INCLUDE_DIRS} CACHE STRING "pybind11 include dirs" FORCE)
set(pybind11_DEFINITIONS ${pybind11_DEFINITIONS} CACHE STRING "pybind11 preprocessor defines" FORCE)
