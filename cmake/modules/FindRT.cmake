# - Try to find librt
# Once done, this will define
#
#  librt_FOUND - system has librt
#  librt_INCLUDE_DIRS - the librt include directories
#  librt_LIBRARIES - link these to use librt

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags rt --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE RT_PKG_CONFIG_INCLUDE_DIRS)
set(RT_PKG_CONFIG_INCLUDE_DIRS "${RT_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for RT library")

# Include dir
find_path(librt_INCLUDE_DIRS
  NAMES time.h
  PATHS ${LIBRT_ROOT}/include
  ${RT_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(librt_LIBRARIES
  NAMES rt
  PATHS ${LIBRT_ROOT}/lib
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(librt_LIBRARIES AND librt_INCLUDE_DIRS)
  set(librt_FOUND true)
endif(librt_LIBRARIES AND librt_INCLUDE_DIRS)
