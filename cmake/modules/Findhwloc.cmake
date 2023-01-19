#!/usr/bin/cmake -P
## Copyright(c) 2018-2022, Intel Corporation
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

# - Try to find hwloc
# Once done, this will define
#
#  hwloc_FOUND - system has hwloc
#  hwloc_ROOT - base dir for hwloc
#  hwloc_INCLUDE_DIRS - the hwloc include directories
#  hwloc_DEFINITIONS - preprocessor defines for hwloc
#  hwloc_LIBRARIES - link these to use hwloc

find_package(PkgConfig)
pkg_check_modules(PC_HWLOC QUIET hwloc)

execute_process(COMMAND pkg-config --cflags-only-I hwloc --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE HWLOC_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other hwloc --silence-errors
    OUTPUT_VARIABLE HWLOC_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (HWLOC_PKG_CONFIG_DEFINITIONS)
    set(hwloc_DEFINITIONS ${HWLOC_PKG_CONFIG_DEFINITIONS})
else(HWLOC_PKG_CONFIG_DEFINITIONS)
    set(hwloc_DEFINITIONS "")
endif(HWLOC_PKG_CONFIG_DEFINITIONS)

find_path(hwloc_ROOT NAMES include/hwloc.h)

find_path(hwloc_INCLUDE_DIRS
    NAMES hwloc/helper.h
    HINTS ${hwloc_ROOT}/include
    PATHS ${hwloc_ROOT}/include
    ${HWLOC_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_HWLOC_hwloc)
    set(hwloc_LIBRARIES ${pkgcfg_lib_PC_HWLOC_hwloc})
else (pkgcfg_lib_PC_HWLOC_hwloc)
    find_library(hwloc_LIBRARIES
        NAMES hwloc
        PATHS ${hwloc_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_HWLOC_hwloc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hwloc
    FOUND_VAR
        hwloc_FOUND
    REQUIRED_VARS
        hwloc_ROOT hwloc_INCLUDE_DIRS hwloc_LIBRARIES
)

set(hwloc_FOUND ${hwloc_FOUND} CACHE BOOL "hwloc status" FORCE)
set(hwloc_ROOT ${hwloc_ROOT} CACHE PATH "base dir for hwloc" FORCE)
set(hwloc_INCLUDE_DIRS ${hwloc_INCLUDE_DIRS} CACHE STRING "hwloc include dirs" FORCE)
set(hwloc_LIBRARIES ${hwloc_LIBRARIES} CACHE STRING "hwloc libraries" FORCE)
set(hwloc_DEFINITIONS ${hwloc_DEFINITIONS} CACHE STRING "hwloc preprocessor defines" FORCE)
