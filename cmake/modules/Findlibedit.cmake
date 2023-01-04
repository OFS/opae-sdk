#!/usr/bin/cmake -P
## Copyright(c) 2020-2022, Intel Corporation
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

# - Try to find libedit
# Once done, this will define
#
#  libedit_ROOT - root dir for libedit
#  libedit_FOUND - system has libedit
#  libedit_INCLUDE_DIRS - the libedit include directories
#  libedit_LIBRARIES - link these to use libedit
#  libedit_DEFINITIONS - preprocessor definitions for libedit

find_package(PkgConfig)
pkg_check_modules(PC_LIBEDIT QUIET libedit)

execute_process(COMMAND pkg-config --cflags-only-I libedit --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE LIBEDIT_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other libedit --silence-errors
    OUTPUT_VARIABLE LIBEDIT_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (LIBEDIT_PKG_CONFIG_DEFINITIONS)
    set(libedit_DEFINITIONS ${LIBEDIT_PKG_CONFIG_DEFINITIONS})
else(LIBEDIT_PKG_CONFIG_DEFINITIONS)
    set(libedit_DEFINITIONS "")
endif(LIBEDIT_PKG_CONFIG_DEFINITIONS)

find_path(libedit_ROOT NAMES include/editline/readline.h)

find_path(libedit_INCLUDE_DIRS
    NAMES editline/readline.h
    HINTS ${libedit_ROOT}/include
    PATHS ${libedit_ROOT}/include
    ${LIBEDIT_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_LIBEDIT_libedit)
    set(libedit_LIBRARIES ${pkgcfg_lib_PC_LIBEDIT_libedit})
else (pkgcfg_lib_PC_LIBEDIT_libedit)
    find_library(libedit_LIBRARIES
        NAMES edit libedit
        PATHS ${LIBEDIT_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_LIBEDIT_libedit)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libedit
    FOUND_VAR
        libedit_FOUND
    REQUIRED_VARS
        libedit_ROOT libedit_INCLUDE_DIRS libedit_LIBRARIES
)

set(libedit_FOUND ${libedit_FOUND} CACHE BOOL "libedit status" FORCE)
set(libedit_ROOT ${libedit_ROOT} CACHE PATH "base dir for libedit" FORCE)
set(libedit_INCLUDE_DIRS ${libedit_INCLUDE_DIRS} CACHE STRING "libedit include dirs" FORCE)
set(libedit_LIBRARIES ${libedit_LIBRARIES} CACHE STRING "libedit libraries" FORCE)
set(libedit_DEFINITIONS ${libedit_DEFINITIONS} CACHE STRING "libedit preprocessor defines" FORCE)
