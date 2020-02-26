# - Try to find libopae-c
# Once done, this will define
#
#  libopae-c_FOUND - system has libopae-c
#  libopae-c_INCLUDE_DIRS - the libopae-c include directories
#  libopae-c_LIBRARIES - link these to use libopae-c

find_package(PkgConfig)
pkg_check_modules(PC_OPAE QUIET opae-c)

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags opae-c --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE OPAE-C_PKG_CONFIG_INCLUDE_DIRS)
set(OPAE-C_PKG_CONFIG_INCLUDE_DIRS "${OPAE-C_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for OPAE-C library")

# Include dir
find_path(libopae-c_INCLUDE_DIRS
  NAMES opae/fpga.h
  PATHS ${LIBOPAE-C_ROOT}/include
  ${OPAE-C_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libopae-c_LIBRARIES
  NAMES opae-c
  PATHS ${LIBOPAE-C_ROOT}/lib
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(libopae-c_LIBRARIES AND libopae-c_INCLUDE_DIRS)
  set(libopae-c_FOUND true)
endif(libopae-c_LIBRARIES AND libopae-c_INCLUDE_DIRS)
