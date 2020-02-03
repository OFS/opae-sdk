cmake_minimum_required (VERSION 2.8)

project(opae-test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_LIBS_ROOT}/cmake/modules")
include(OPAE)
# projectname is the same as the main-executable

add_subdirectory(framework)

