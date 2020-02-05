cmake_minimum_required (VERSION 2.8)

project(opae-test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_LIBS_ROOT}/cmake/modules")
include(OPAE)
# projectname is the same as the main-executable

option(OPAE_ENABLE_MOCK "Enable building of test infrastructure with mock" OFF)
mark_as_advanced(OPAE_ENABLE_MOCK)
add_subdirectory(framework)

