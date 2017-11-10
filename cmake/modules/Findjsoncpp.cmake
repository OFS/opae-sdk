# - Try to find libjson-c
# Once done, this will define
#
#  libjsoncpp_FOUND - system has libjsoncpp
#  libjsoncpp_INCLUDE_DIRS - the libjsoncpp include directories
#  libjsoncpp_LIBRARIES - link these to use libjsoncpp

# Use pkg-config to get hints about paths
execute_process(COMMAND pkg-config --cflags jsoncpp --silence-errors
  COMMAND cut -d I -f 2
  OUTPUT_VARIABLE JSONCPP_PKG_CONFIG_INCLUDE_DIRS)
set(JSONCPP_PKG_CONFIG_INCLUDE_DIRS "${JSONCPP_PKG_CONFIG_INCLUDE_DIRS}" CACHE STRING "Compiler flags for JSONCPP library")

# Include dir
find_path(jsoncpp_INCLUDE_DIRS
  NAMES json/json.h
  PATHS ${JSONCPP_ROOT}/include
  ${JSONCPP_PKG_CONFIG_INCLUDE_DIRS}
  /usr/local/include
  /usr/include
  /usr/include/jsoncpp
  ${CMAKE_EXTRA_INCLUDES})

# The library itself
find_library(jsoncpp_LIBRARIES
  NAMES jsoncpp
  PATHS ${JSONCPP_ROOT}/lib
  /usr/local/lib
  /usr/lib
  /lib
  /usr/lib/x86_64-linux-gnu
  ${CMAKE_EXTRA_LIBS})

if(jsoncpp_LIBRARIES AND jsoncpp_INCLUDE_DIRS)
  set(jsoncpp_FOUND true)
endif(jsoncpp_LIBRARIES AND jsoncpp_INCLUDE_DIRS)
