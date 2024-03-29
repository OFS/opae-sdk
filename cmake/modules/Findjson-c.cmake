#!/usr/bin/cmake -P
## Copyright(c) 2017-2022, Intel Corporation
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

# - Try to find json-c
# Once done, this will define
#
#  json-c_ROOT - root of libjson-c files
#  json-c_FOUND - system has libjson-c
#  json-c_INCLUDE_DIRS - the libjson-c include directories
#  json-c_LIBRARIES - link these to use libjson-c
#  json-c_DEFINITIONS - preprocessor definitions for libjson-c

find_package(PkgConfig)
pkg_check_modules(PC_JSON_C QUIET json-c)

execute_process(COMMAND pkg-config --cflags-only-I json-c --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE JSON_C_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other json-c --silence-errors
    OUTPUT_VARIABLE JSON_C_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (JSON_C_PKG_CONFIG_DEFINITIONS)
    set(json-c_DEFINITIONS ${JSON_C_PKG_CONFIG_DEFINITIONS})
else(JSON_C_PKG_CONFIG_DEFINITIONS)
    set(json-c_DEFINITIONS "")
endif(JSON_C_PKG_CONFIG_DEFINITIONS)

find_path(json-c_ROOT NAMES include/json-c/json.h)

find_path(json-c_INCLUDE_DIRS
    NAMES json-c/json.h
    HINTS ${json-c_ROOT}/include
    PATHS ${json-c_ROOT}/include
    ${JSON_C_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_JSON_C_json-c)
    set(json-c_LIBRARIES ${pkgcfg_lib_PC_JSON_C_json-c})
else (pkgcfg_lib_PC_JSON_C_json-c)
    find_library(json-c_LIBRARIES
        NAMES json-c
        PATHS ${json-c_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_JSON_C_json-c)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(json-c
    FOUND_VAR
        json-c_FOUND
    REQUIRED_VARS
        json-c_ROOT json-c_INCLUDE_DIRS json-c_LIBRARIES
)

set(json-c_FOUND ${json-c_FOUND} CACHE BOOL "json-c status" FORCE)
set(json-c_ROOT ${json-c_ROOT} CACHE PATH "base dir for json-c" FORCE)
set(json-c_INCLUDE_DIRS ${json-c_INCLUDE_DIRS} CACHE STRING "json-c include dirs" FORCE)
set(json-c_LIBRARIES ${json-c_LIBRARIES} CACHE STRING "json-c libraries" FORCE)
set(json-c_DEFINITIONS ${json-c_DEFINITIONS} CACHE STRING "json-c preprocessor defines" FORCE)
