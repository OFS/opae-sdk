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
## POSSIBILITY OF SUCH DAMAGE

# - Try to find uuid
# Once done, this will define
#
#  libuuid_FOUND - system has libuuid
#  libuuid_INCLUDE_DIRS - the libuuid include directories
#  libuuid_LIBRARIES - link these to use libuuid

find_package(PkgConfig)
pkg_check_modules(PC_UUID QUIET uuid)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags uuid --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE UUID_PKG_CONFIG_INCLUDE_DIRS)
set(UUID_PKG_CONFIG_INCLUDE_DIRS "${UUID_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for UUID library")

# Include dir
find_path(libuuid_INCLUDE_DIRS
  NAMES uuid/uuid.h
  PATHS ${LIBUUID_ROOT}/include
  ${UUID_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libuuid_LIBRARIES
  NAMES uuid
  PATHS ${LIBUUID_ROOT}/lib
  ${PC_UUID_LIBDIR}
  ${PC_UUID_LIBRARY_DIRS}
  /usr/local/lib
  /usr/lib
  /lib
  ${CMAKE_EXTRA_LIBS})

if(libuuid_LIBRARIES AND libuuid_INCLUDE_DIRS)
  set(libuuid_FOUND true)
endif(libuuid_LIBRARIES AND libuuid_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libuuid REQUIRED_VARS libuuid_INCLUDE_DIRS libuuid_LIBRARIES)
