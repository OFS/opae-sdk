#!/usr/bin/cmake -P
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

# - Try to find libjson-c
# Once done, this will define
#
#  libjson-c_FOUND - system has libjson-c
#  libjson-c_INCLUDE_DIRS - the libjson-c include directories
#  libjson-c_LIBRARIES - link these to use libjson-c

find_package(PkgConfig)
pkg_check_modules(PC_JSON_C QUIET json-c)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags json-c --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE JSON-C_PKG_CONFIG_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(JSON-C_PKG_CONFIG_INCLUDE_DIRS "${JSON-C_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for JSON-C library")

# Include dir
find_path(libjson-c_INCLUDE_DIRS
  NAMES json-c/json.h
  PATHS ${LIBJSON-C_ROOT}/include
  ${JSON-C_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libjson-c_LIBRARIES
  NAMES json-c
  PATHS ${LIBJSON-C_ROOT}/lib
  ${PC_JSON_C_LIBDIR}
  ${PC_JSON_C_LIBRARY_DIRS}
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(libjson-c_LIBRARIES AND libjson-c_INCLUDE_DIRS)
  set(libjson-c_FOUND true)
endif(libjson-c_LIBRARIES AND libjson-c_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libjson-c REQUIRED_VARS libjson-c_INCLUDE_DIRS libjson-c_LIBRARIES)
