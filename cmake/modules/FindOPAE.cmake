#!/usr/bin/cmake -P
## Copyright(c) 2014-2018, Intel Corporation
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

# - Try to find libopae-c
# Once done, this will define
#
#  libopae-c_FOUND - system has libopae-c
#  libopae-c_INCLUDE_DIRS - the libopae-c include directories
#  libopae-c_LIBRARIES - link these to use libopae-c

find_package(PkgConfig)
pkg_check_modules(PC_OPAE QUIET opae-c)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags opae-c --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE OPAE-C_PKG_CONFIG_INCLUDE_DIRS)
set(OPAE-C_PKG_CONFIG_INCLUDE_DIRS "${OPAE-C_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for OPAE-C library")

# Include dir
find_path(libopae-c_INCLUDE_DIRS
  NAMES opae/fpga.h
  PATHS ${LIBOPAE-C_ROOT}/include
  ${OPAE-C_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libopae-c_LIBRARIES
  NAMES opae-c
  PATHS ${LIBOPAE-C_ROOT}/lib
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(libopae-c_LIBRARIES AND libopae-c_INCLUDE_DIRS)
  set(libopae-c_FOUND true)
endif(libopae-c_LIBRARIES AND libopae-c_INCLUDE_DIRS)
