## Copyright(c) 2017, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 2.8.11)
project(opae-tests)

# Bootstrap gtest
option(FETCH_AND_BUILD_GTEST "Enable gtest compilation" ON)
mark_as_advanced(FETCH_AND_BUILD_GTEST)

if (NOT FETCH_AND_BUILD_GTEST)
  # Find gtest prevents compilation in travis CI
  # Ubuntu gmock libraries are non-position relocatable
  find_package(GTest)
else()
  message(STATUS "Trying to fetch gtest throught git")
  find_package(Git REQUIRED)

  # Enable ExternalProject CMake module
  include(ExternalProject)

  # Download and install GoogleTest
  ExternalProject_Add(
    gtest
    GIT_REPOSITORY "https://github.com/google/googletest"
    GIT_TAG "release-1.8.0"
    UPDATE_COMMAND ""
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    # Disable install step
    INSTALL_COMMAND "")
  set (gtest_root "${CMAKE_CURRENT_BINARY_DIR}/gtest/src/gtest/googletest")
  message(STATUS "gtest locatet at: ${gtest_root}")

  # Create a libgtest target to be used as a dependency by test programs
  add_library(libgtest IMPORTED STATIC GLOBAL)
  add_dependencies(libgtest gtest)

  # Get GTest source and binary directories from CMake project
  ExternalProject_Get_Property(gtest source_dir binary_dir)

  # Set libgtest properties
  set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

  # Export gtest variables
  set(GTEST_ROOT ${gtest_root} PARENT_SCOPE)
  set(GTEST_INCLUDE_DIRS ${gtest_root}/include)
  set(GTEST_MAIN_LIBRARY gtest_main PARENT_SCOPE)
  set(GTEST_BOTH_LIBRARIES libgtest)
  set(GTEST_FOUND true)
  message(STATUS "gtest include dir: ${GTEST_INCLUDE_DIRS}")
endif()

# Include directories
include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${OPAE_INCLUDE_DIR}
  ${OPAE_SDK_SOURCE}/libopae/src
  ${OPAE_SDK_SOURCE}/tools/libopae++
  ${OPAE_SDK_SOURCE}/tools/c++utils
  ${Boost_INCLUDE_DIRS}
  ${OPAE_SDK_SOURCE}/common/include
  ${OPAE_SDK_SOURCE}/tests
  ${OPAE_SDK_SOURCE}/libopae++/include
  ${libjson-c_INCLUDE_DIRS})

add_library(commonlib SHARED common_test.h common_test.cpp)
target_link_libraries(commonlib opae-c ${GTEST_BOTH_LIBRARIES} ${libjson-c_LIBRARIES})
target_include_directories(commonlib PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)

# Test binary
set(SRC gtmain.cpp
  jsonParser.cpp
  unit/gtOpenClose_base.cpp
  unit/gtCxxEnumerate.cpp
  unit/gtCxxOpenClose.cpp
  unit/gtCxxProperties.cpp
  unit/gtCxxExcept.cpp
  unit/gtCxxBuffer.cpp
  unit/gtCxxLog.cpp)

add_executable(gtapi ${SRC})
target_include_directories(gtapi PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)
target_link_libraries(gtapi commonlib safestr opae-c opae-cxx-core ${libjson-c_LIBRARIES} uuid ${GTEST_BOTH_LIBRARIES}
  opae-c++-utils opae-c++)
configure_file(${PROJECT_SOURCE_DIR}/configuration.json
  ${CMAKE_BINARY_DIR}/tests/configuration.json
  COPYONLY)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC commonlib "-pthread")
  target_compile_options(PUBLIC gtapi "-pthread")
endif()

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(gtapi "${CMAKE_THREAD_LIBS_INIT}")
endif()

# build mock driver fake directory structure
add_custom_target(mock-sysfs-prepare)
add_custom_command(TARGET mock-sysfs-prepare POST_BUILD
  COMMAND cmake -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz /tmp
  COMMAND tar xzvf /tmp/mock_sys_tmp-1socket-nlb0.tar.gz -C /tmp --strip 1)

# build mock driver
add_library(mock SHARED mock.c)
target_include_directories(mock PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae/src>)
add_dependencies(mock mock-sysfs-prepare)
target_link_libraries(mock dl safestr)

############################################################################
## Add testing targets #####################################################
############################################################################
macro(add_gtfilter test_name gtfilter is_mock)
    add_test(NAME ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND $<TARGET_FILE:gtapi> "--gtest_filter=${gtfilter}")
    if(${is_mock})
        set_tests_properties(
            ${test_name}
            PROPERTIES
            ENVIRONMENT "LD_PRELOAD=./lib/libmock.so")
    endif()
endmacro()

add_test(NAME test_open_common_mock_drv
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi -j ${CMAKE_BINARY_DIR}/tests/configuration.json --gtest_filter=*OpenCommonHW*)

set_tests_properties(
  test_open_common_mock_drv
  PROPERTIES
  ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

add_gtfilter(CxxProperties "CxxProperties*" False)
add_gtfilter(CxxEnum "CxxEnum*" True)
add_gtfilter(CxxOpenClose "CxxOpen*" True)
add_gtfilter(CxxBuffer "CxxBuffer*" True)
add_gtfilter(CxxExcept "CxxExcept*" False)
add_gtfilter(CxxLog "CxxLog*" False)
############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_TESTS AND GTEST_FOUND)
    # targetname     The name of original target from which coverage is created for
    # testrunner     The name of the target which runs the tests.
    set_target_for_coverage(opae-c ctest)

    add_dependencies(coverage_opae-c gtapi)
  endif(BUILD_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
