cmake_minimum_required(VERSION 2.8.12)
project(opae)

set(OPAE_VERSION_MAJOR    1 CACHE STRING "OPAE major version")
set(OPAE_VERSION_MINOR    4 CACHE STRING "OPAE minor version")
set(OPAE_VERSION_REVISION 0 CACHE STRING "OPAE revision version")
set(OPAE_VERSION ${OPAE_VERSION_MAJOR}.${OPAE_VERSION_MINOR}.${OPAE_VERSION_REVISION})

set(OPAE_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-sdk project" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_SDK_ROOT}/cmake/modules")
include(OPAE)



add_subdirectory(safe_string)


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/config.h.in"
               "${PROJECT_BINARY_DIR}/include/config.h")
