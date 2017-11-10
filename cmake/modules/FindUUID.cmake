# - Try to find uuid
# Once done, this will define
#
#  libuuid_FOUND - system has libuuid
#  libuuid_INCLUDE_DIRS - the libuuid include directories
#  libuuid_LIBRARIES - link these to use libuuid

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags uuid --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE UUID_PKG_CONFIG_INCLUDE_DIRS)
set(UUID_PKG_CONFIG_INCLUDE_DIRS "${UUID_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for UUID library")

# Include dir
find_path(libuuid_INCLUDE_DIRS
  NAMES uuid/uuid.h
  PATHS ${LIBUUID_ROOT}/include
  ${UUID_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(libuuid_LIBRARIES
  NAMES uuid
  PATHS ${LIBUUID_ROOT}/lib
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(libuuid_LIBRARIES AND libuuid_INCLUDE_DIRS)
  set(libuuid_FOUND true)
endif(libuuid_LIBRARIES AND libuuid_INCLUDE_DIRS)
