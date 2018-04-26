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


find_library(libdbus_LIBRARIES
  NAMES dbus-1
  HINTS ${PC_DBUS_LIBDIR}
  ${PC_DBUS_LIBRARY_DIRS})

find_path(libdbus_INCLUDE_DIR
  NAMES dbus/dbus.h
  HINTS
  ${DBUS_PKG_CONFIG_INCLUDE_DIRS}
  ${PC_DBUS_INCLUDEDIR}
  ${PC_DBUS_INCLUDE_DIRS})

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
