# - Try to find libjson-c
# Once done, this will define
#
#  libjson-c_FOUND - system has libjson-c
#  libjson-c_INCLUDE_DIRS - the libjson-c include directories
#  libjson-c_LIBRARIES - link these to use libjson-c

find_package(PkgConfig)
pkg_check_modules(PC_JSON_C QUIET json-c)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags json-c --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE JSON-C_PKG_CONFIG_INCLUDE_DIRS)
set(JSON-C_PKG_CONFIG_INCLUDE_DIRS "${JSON-C_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for JSON-C library")

# Include dir
find_path(libjson-c_INCLUDE_DIRS
  NAMES json-c/json.h
  PATHS ${LIBJSON-C_ROOT}/include
  ${JSON-C_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libjson-c_LIBRARIES
  NAMES json-c
  PATHS ${LIBJSON-C_ROOT}/lib
  ${PC_JSON_C_LIBDIR}
  ${PC_JSON_C_LIBRARY_DIRS}
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(libjson-c_LIBRARIES AND libjson-c_INCLUDE_DIRS)
  set(libjson-c_FOUND true)
endif(libjson-c_LIBRARIES AND libjson-c_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libjson-c REQUIRED_VARS libjson-c_INCLUDE_DIRS libjson-c_LIBRARIES)
