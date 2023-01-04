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
## POSSIBILITY OF SUCH DAMAGE

# - Try to find uuid
# Once done, this will define
#
#  uuid_ROOT - root of libuuid
#  uuid_FOUND - system has libuuid
#  uuid_INCLUDE_DIRS - the libuuid include directories
#  uuid_LIBRARIES - link these to use libuuid

find_package(PkgConfig)
pkg_check_modules(PC_UUID QUIET uuid)

execute_process(COMMAND pkg-config --cflags-only-I uuid --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE UUID_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other uuid --silence-errors
    OUTPUT_VARIABLE UUID_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (UUID_PKG_CONFIG_DEFINITIONS)
    set(uuid_DEFINITIONS ${UUID_PKG_CONFIG_DEFINITIONS})
else(UUID_PKG_CONFIG_DEFINITIONS)
    set(uuid_DEFINITIONS "")
endif(UUID_PKG_CONFIG_DEFINITIONS)

find_path(uuid_ROOT NAMES include/uuid/uuid.h)

find_path(uuid_INCLUDE_DIRS
    NAMES uuid/uuid.h
    HINTS ${uuid_ROOT}/include
    PATHS ${uuid_ROOT}/include
    ${UUID_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_UUID_uuid)
    set(uuid_LIBRARIES ${pkgcfg_lib_PC_UUID_uuid})
else (pkgcfg_lib_PC_UUID_uuid)
    find_library(uuid_LIBRARIES
        NAMES uuid
        PATHS ${uuid_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_UUID_uuid)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(uuid
    FOUND_VAR
        uuid_FOUND
    REQUIRED_VARS
        uuid_ROOT uuid_INCLUDE_DIRS uuid_LIBRARIES
)

set(uuid_FOUND ${uuid_FOUND} CACHE BOOL "uuid status" FORCE)
set(uuid_ROOT ${uuid_ROOT} CACHE PATH "base dir for uuid" FORCE)
set(uuid_INCLUDE_DIRS ${uuid_INCLUDE_DIRS} CACHE STRING "uuid include dirs" FORCE)
set(uuid_LIBRARIES ${uuid_LIBRARIES} CACHE STRING "uuid libraries" FORCE)
set(uuid_DEFINITIONS ${uuid_DEFINITIONS} CACHE STRING "uuid preprocessor defines" FORCE)
