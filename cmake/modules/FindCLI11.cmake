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

# - Try to find CLI11
# Once done, this will define
#
#  CLI11_FOUND - system has CLI11
#  CLI11_ROOT - base dir for CLI11
#  CLI11_INCLUDE_DIRS - the CLI11 include directories
#  CLI11_LIBRARIES - libraries for CLI11
#  CLI11_DEFINITIONS - preprocessor definitions for CLI11

find_package(PkgConfig)
pkg_check_modules(PC_CLI11 QUIET CLI11)

execute_process(COMMAND pkg-config --cflags-only-I CLI11 --silence-errors
    COMMAND cut -d I -f 2
    OUTPUT_VARIABLE CLI11_PKG_CONFIG_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND pkg-config --cflags-only-other CLI11 --silence-errors
    OUTPUT_VARIABLE CLI11_PKG_CONFIG_DEFINITIONS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (CLI11_PKG_CONFIG_DEFINITIONS)
    set(CLI11_DEFINITIONS ${CLI11_PKG_CONFIG_DEFINITIONS})
else(CLI11_PKG_CONFIG_DEFINITIONS)
    set(CLI11_DEFINITIONS "")
endif(CLI11_PKG_CONFIG_DEFINITIONS)

find_path(CLI11_ROOT NAMES include/CLI/CLI.hpp)

find_path(CLI11_INCLUDE_DIRS
    NAMES CLI/CLI.hpp
    HINTS ${CLI11_ROOT}/include
    PATHS ${CLI11_ROOT}/include
    ${CLI11_PKG_CONFIG_INCLUDE_DIRS}
    /usr/local/include
    /usr/include
    ${CMAKE_EXTRA_INCLUDES}
)

if (pkgcfg_lib_PC_CLI11_CLI11)
    set(CLI11_LIBRARIES ${pkgcfg_lib_PC_CLI11_CLI11})
else (pkgcfg_lib_PC_CLI11_CLI11)
    find_library(CLI11_LIBRARIES
        NAMES CLI11
	PATHS ${CLI11_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /lib
        /usr/lib/x86_64-linux-gnu
        ${CMAKE_EXTRA_LIBS}
    )
endif (pkgcfg_lib_PC_CLI11_CLI11)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CLI11
    FOUND_VAR
        CLI11_FOUND
    REQUIRED_VARS
    CLI11_ROOT CLI11_INCLUDE_DIRS
)

set(CLI11_FOUND ${CLI11_FOUND} CACHE BOOL "CLI11 status" FORCE)
set(CLI11_ROOT ${CLI11_ROOT} CACHE PATH "base dir for CLI11" FORCE)
set(CLI11_INCLUDE_DIRS ${CLI11_INCLUDE_DIRS} CACHE STRING "CLI11 include dirs" FORCE)
set(CLI11_LIBRARIES ${CLI11_LIBRARIES} CACHE STRING "CLI11 libraries" FORCE)
set(CLI11_DEFINITIONS ${CLI11_DEFINITIONS} CACHE STRING "CLI11 preprocessor defines" FORCE)
