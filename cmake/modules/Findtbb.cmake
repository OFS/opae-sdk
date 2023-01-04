#!/usr/bin/cmake -P
## Copyright(c) 2019-2022, Intel Corporation
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

# - Try to find tbb
# Once done, this will define
#
#  tbb_FOUND - system has tbb
#  tbb_ROOT - base dir for tbb
#  tbb_INCLUDE_DIRS - the tbb include directories
#  tbb_DEFINITIONS - preprocessor defines for tbb
#  tbb_LIBRARIES - link these to use tbb

find_package(PkgConfig)
pkg_check_modules(PC_TBB QUIET tbb)

execute_process(COMMAND pkg-config --cflags-only-I tbb --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE TBB_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other tbb --silence-errors
    OUTPUT_VARIABLE TBB_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (TBB_PKG_CONFIG_DEFINITIONS)
    set(tbb_DEFINITIONS ${TBB_PKG_CONFIG_DEFINITIONS})
else(TBB_PKG_CONFIG_DEFINITIONS)
    set(tbb_DEFINITIONS "")
endif(TBB_PKG_CONFIG_DEFINITIONS)

find_path(tbb_ROOT NAMES include/tbb/tbb.h)

find_path(tbb_INCLUDE_DIRS
    NAMES tbb/tbb.h
    HINTS ${tbb_ROOT}/include
    PATHS ${tbb_ROOT}/include
    ${TBB_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_TBB_tbb)
    set(tbb_LIBRARIES ${pkgcfg_lib_PC_TBB_tbb})
else (pkgcfg_lib_PC_TBB_tbb)
    find_library(tbb_LIBRARIES
        NAMES tbb
        PATHS ${tbb_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_TBB_tbb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(tbb
    FOUND_VAR
        tbb_FOUND
    REQUIRED_VARS
        tbb_ROOT tbb_INCLUDE_DIRS tbb_LIBRARIES
)

set(tbb_FOUND ${tbb_FOUND} CACHE BOOL "tbb status" FORCE)
set(tbb_ROOT ${tbb_ROOT} CACHE PATH "base dir for tbb" FORCE)
set(tbb_INCLUDE_DIRS ${tbb_INCLUDE_DIRS} CACHE STRING "tbb include dirs" FORCE)
set(tbb_LIBRARIES ${tbb_LIBRARIES} CACHE STRING "tbb libraries" FORCE)
set(tbb_DEFINITIONS ${tbb_DEFINITIONS} CACHE STRING "tbb preprocessor defines" FORCE)
