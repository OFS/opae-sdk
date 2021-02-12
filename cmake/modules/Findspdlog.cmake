#!/usr/bin/cmake -P
## Copyright(c) 2021, Intel Corporation
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
#  spdlog_INCLUDE_DIRS - the spdlog include directories
#  spdlog_LIBRARIES - link these to use spdlog

find_package(PkgConfig)
pkg_check_modules(PC_SPDLOG QUIET spdlog)

execute_process(COMMAND pkg-config --cflags spdlog --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE SPDLOG_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
set(SPDLOG_PKG_CONFIG_INCLUDE_DIRS "${EDIT_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for spdlog")

find_path(spdlog_INCLUDE_DIRS
    NAMES spdlog/spdlog.h
    PATHS ${SPDLOG_ROOT}/include
    ${SPDLOG_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES})

if (pkgcfg_lib_PC_SPDLOG_spdlog)
    set(spdlog_LIBRARIES ${pkgcfg_lib_PC_SPDLOG_spdlog}
        CACHE STRING "Path to spdlog libraries")
else (pkgcfg_lib_PC_SPDLOG_spdlog)
    find_library(spdlog_LIBRARIES
        NAMES spdlog
        PATHS ${SPDLOG_ROOT}/lib
        ${PC_SPDLOG_LIBDIR}
        ${PC_SPDLOG_LIBRARY_DIRS}
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS})
endif (pkgcfg_lib_PC_SPDLOG_spdlog)

if(spdlog_LIBRARIES AND spdlog_INCLUDE_DIRS)
    set(spdlog_FOUND true)
    set(spdlog_DEFINITIONS
        "SPDLOG_COMPILED_LIB" CACHE STRING "Compile definitions of spdlog")
endif(spdlog_LIBRARIES AND spdlog_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(spdlog
    REQUIRED_VARS spdlog_INCLUDE_DIRS spdlog_LIBRARIES spdlog_DEFINITIONS)
