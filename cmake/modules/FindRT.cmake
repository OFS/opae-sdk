#!/usr/bin/cmake -P
## Copyright(c) 2014-2020, Intel Corporation
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

# - Try to find librt
# Once done, this will define
#
#  librt_FOUND - system has librt
#  librt_INCLUDE_DIRS - the librt include directories
#  librt_LIBRARIES - link these to use librt

find_package(PkgConfig)
pkg_check_modules(PC_RT QUIET rt)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags rt --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE RT_PKG_CONFIG_INCLUDE_DIRS)
set(RT_PKG_CONFIG_INCLUDE_DIRS "${RT_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for RT library")

# Include dir
find_path(librt_INCLUDE_DIRS
  NAMES time.h
  PATHS ${LIBRT_ROOT}/include
  ${RT_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(librt_LIBRARIES
  NAMES rt
  PATHS ${LIBRT_ROOT}/lib
  ${PC_RT_LIBDIR}
  ${PC_RT_LIBRARY_DIRS}
  /usr/local/lib
  /usr/lib
  /lib
  ${CMAKE_EXTRA_LIBS})

if(librt_LIBRARIES AND librt_INCLUDE_DIRS)
  set(librt_FOUND true)
endif(librt_LIBRARIES AND librt_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(librt REQUIRED_VARS librt_INCLUDE_DIRS librt_LIBRARIES)
