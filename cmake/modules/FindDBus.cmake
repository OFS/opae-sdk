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

# - Try to find DBus
# Once done, this will define
#
#  libdbus_FOUND - system has DBus
#  libdbus_INCLUDE_DIRS - the DBus include directories
#  libdbus_LIBRARIES - link these to use DBus
#

find_package(PkgConfig)
pkg_check_modules(PC_DBUS QUIET dbus-1)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags dbus-1 --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE DBUS_PKG_CONFIG_INCLUDE_DIRS)
set(DBUS_PKG_CONFIG_INCLUDE_DIRS "${DBUS_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for DBus library")

find_path(libdbus_INCLUDE_DIR
  NAMES dbus/dbus.h
  HINTS
  ${DBUS_PKG_CONFIG_INCLUDE_DIRS}
  ${PC_DBUS_INCLUDEDIR}
  ${PC_DBUS_INCLUDE_DIRS})

find_library(libdbus_LIBRARIES
  NAMES dbus-1
  HINTS ${PC_DBUS_LIBDIR}
  ${PC_DBUS_LIBRARY_DIRS})

get_filename_component(_libdbus_LIBRARY_DIR ${libdbus_LIBRARIES} PATH)
find_path(libdbus_ARCH_INCLUDE_DIR
  NAMES dbus/dbus-arch-deps.h
  HINTS ${PC_DBUS_INCLUDEDIR}
  ${PC_DBUS_INCLUDE_DIRS}
  ${_libdbus_LIBRARY_DIR}
  ${libdbus_INCLUDE_DIR}
  /usr/lib/x86_64-linux-gnu
  PATH_SUFFIXES include)
set(libdbus_INCLUDE_DIRS
  ${libdbus_INCLUDE_DIR}
  ${libdbus_ARCH_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libdbus REQUIRED_VARS libdbus_INCLUDE_DIRS libdbus_LIBRARIES)
