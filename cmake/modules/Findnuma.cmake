#!/usr/bin/cmake -P
## Copyright(c) 2023, Intel Corporation
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

# - Try to find numa
# Once done, this will define
#
#  numa_FOUND - system has numa
#  numa_ROOT - base dir for numa
#  numa_INCLUDE_DIRS - the numa include directories
#  numa_DEFINITIONS - preprocessor defines for numa
#  numa_LIBRARIES - link these to use numa

find_package(PkgConfig)
pkg_check_modules(PC_NUMA QUIET numa)

execute_process(COMMAND pkg-config --cflags-only-I numa --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE NUMA_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other numa --silence-errors
    OUTPUT_VARIABLE NUMA_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NUMA_PKG_CONFIG_DEFINITIONS)
    set(numa_DEFINITIONS ${NUMA_PKG_CONFIG_DEFINITIONS})
else(NUMA_PKG_CONFIG_DEFINITIONS)
    set(numa_DEFINITIONS "")
endif(NUMA_PKG_CONFIG_DEFINITIONS)

find_path(numa_ROOT NAMES include/numa.h)

find_path(numa_INCLUDE_DIRS
    NAMES numa.h
    HINTS ${numa_ROOT}/include
    PATHS ${numa_ROOT}/include
    ${NUMA_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_NUMA_numa)
    set(numa_LIBRARIES ${pkgcfg_lib_PC_NUMA_numa})
else (pkgcfg_lib_PC_NUMA_numa)
    find_library(numa_LIBRARIES
        NAMES numa
        PATHS ${numa_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_NUMA_numa)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(numa
    FOUND_VAR
        numa_FOUND
    REQUIRED_VARS
        numa_ROOT numa_INCLUDE_DIRS numa_LIBRARIES
)

set(numa_FOUND ${numa_FOUND} CACHE BOOL "numa status" FORCE)
set(numa_ROOT ${numa_ROOT} CACHE PATH "base dir for numa" FORCE)
set(numa_INCLUDE_DIRS ${numa_INCLUDE_DIRS} CACHE STRING "numa include dirs" FORCE)
set(numa_LIBRARIES ${numa_LIBRARIES} CACHE STRING "numa libraries" FORCE)
set(numa_DEFINITIONS ${numa_DEFINITIONS} CACHE STRING "numa preprocessor defines" FORCE)
