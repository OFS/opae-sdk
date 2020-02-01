cmake_minimum_required(VERSION 2.8.12)
project(opae)

set(OPAE_VERSION_MAJOR    1 CACHE STRING "OPAE major version")
set(OPAE_VERSION_MINOR    4 CACHE STRING "OPAE minor version")
set(OPAE_VERSION_REVISION 0 CACHE STRING "OPAE revision version")
set(OPAE_VERSION ${OPAE_VERSION_MAJOR}.${OPAE_VERSION_MINOR}.${OPAE_VERSION_REVISION})

set(OPAE_LIBS_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-libs project" FORCE)
set(OPAE_INCLUDE_PATH ${OPAE_LIBS_ROOT}/include CACHE PATH "Include directory for opae-libs")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_LIBS_ROOT}/cmake/modules")
include(OPAE)


get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
    set(OPAE_LIB_INSTALL_DIR "lib64" CACHE INTERNAL "arch specific library")
else()
    set(OPAE_LIB_INSTALL_DIR "lib" CACHE INTERNAL "arch specific library")
endif()

option(OPAE_INSTALL_RPATH "Enable to create RPATH during install" OFF)
mark_as_advanced(OPAE_INSTALL_RPATH)
############################################################################
## RPATH Handling ##########################################################
############################################################################
set(CMAKE_SKIP_BUILD_RPATH FALSE)
if(OPAE_INSTALL_RPATH)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${OPAE_LIB_INSTALL_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

add_subdirectory(safe_string)
add_subdirectory(libopae-c)
add_subdirectory(plugins)
add_subdirectory(libbitstream)
add_subdirectory(libopaecxx)


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/config.h.in"
               "${PROJECT_BINARY_DIR}/include/config.h")
