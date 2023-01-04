#!/usr/bin/cmake -P
## Copyright(c) 2021-2022, Intel Corporation
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

# - Try to find spdlog
# Once done, this will define
#
#  spdlog_FOUND - system has spdlog
#  spdlog_ROOT - base dir for spdlog
#  spdlog_INCLUDE_DIRS - the spdlog include directories
#  spdlog_DEFINITIONS - preprocessor defines for spdlog
#  spdlog_LIBRARIES - link these to use spdlog

find_package(PkgConfig)
pkg_check_modules(PC_SPDLOG QUIET spdlog)

execute_process(COMMAND pkg-config --cflags-only-I spdlog --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE SPDLOG_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other spdlog --silence-errors
    OUTPUT_VARIABLE SPDLOG_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (SPDLOG_PKG_CONFIG_DEFINITIONS)
    set(spdlog_DEFINITIONS ${SPDLOG_PKG_CONFIG_DEFINITIONS})
else(SPDLOG_PKG_CONFIG_DEFINITIONS)
    set(spdlog_DEFINITIONS "")
endif(SPDLOG_PKG_CONFIG_DEFINITIONS)

find_path(spdlog_ROOT NAMES include/spdlog/spdlog.h)

find_path(spdlog_INCLUDE_DIRS
    NAMES spdlog/spdlog.h
    HINTS ${spdlog_ROOT}/include
    PATHS ${spdlog_ROOT}/include
    ${SPDLOG_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_SPDLOG_spdlog)
    set(spdlog_LIBRARIES ${pkgcfg_lib_PC_SPDLOG_spdlog})
else (pkgcfg_lib_PC_SPDLOG_spdlog)
    find_library(spdlog_LIBRARIES
        NAMES spdlog
        PATHS ${spdlog_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_SPDLOG_spdlog)

# some distros require linking to libfmt.so, pkg-config can tell us if so
if(pkgcfg_lib_PC_SPDLOG_fmt)
    set(spdlog_LIBRARIES ${spdlog_LIBRARIES} ${pkgcfg_lib_PC_SPDLOG_fmt})
endif(pkgcfg_lib_PC_SPDLOG_fmt)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
    FOUND_VAR
        spdlog_FOUND
    REQUIRED_VARS
        spdlog_ROOT spdlog_INCLUDE_DIRS
)

set(spdlog_FOUND ${spdlog_FOUND} CACHE BOOL "spdlog status" FORCE)
set(spdlog_ROOT ${spdlog_ROOT} CACHE PATH "base dir for spdlog" FORCE)
set(spdlog_INCLUDE_DIRS ${spdlog_INCLUDE_DIRS} CACHE STRING "spdlog include dirs" FORCE)
set(spdlog_LIBRARIES ${spdlog_LIBRARIES} CACHE STRING "spdlog libraries" FORCE)
set(spdlog_DEFINITIONS ${spdlog_DEFINITIONS} CACHE STRING "spdlog preprocessor defines" FORCE)
