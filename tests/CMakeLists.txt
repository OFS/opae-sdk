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

cmake_minimum_required(VERSION 2.8.12)
project(opae-tests)

##############################################################################
## Setup GTEST ###############################################################
##############################################################################
include(test_config)
find_package(GTest)

If (${GTest_FOUND})
  message(STATUS Found GTest)
else()
  Build_GTEST()
endif()
############################################################################
## Generate GTAPI ##########################################################
############################################################################
# Include directories
include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${OPAE_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${libjson-c_INCLUDE_DIRS}
  ${OPAE_SDK_SOURCE}/common/include
  ${OPAE_SDK_SOURCE}/libopae/src
  ${OPAE_SDK_SOURCE}/tools/libopae++
  ${OPAE_SDK_SOURCE}/tools/c++utils
  ${OPAE_SDK_SOURCE}/tests)

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
  unit/gtProperties_base.cpp
  unit/gtOpen.cpp
  unit/gtEnumerate.cpp
  unit/gtOptionParser.cpp
  function/gtReset.cpp
  function/gtUmsg.cpp
  function/gtBuffer.cpp
  function/gtEnumerate.cpp
  function/gtHostif.cpp
  function/gtMMIO.cpp
  function/gtEvent.cpp)

add_executable(gtapi ${SRC})
target_include_directories(gtapi PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)
target_link_libraries(gtapi commonlib safestr opae-c ${libjson-c_LIBRARIES} uuid ${GTEST_BOTH_LIBRARIES}
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

############################################################################
## Build MOCK Driver   #####################################################
############################################################################
# build mock driver and unpack fake sysfs to /tmp folder
Build_MOCk_DRV()

############################################################################
## Add testing targets #####################################################
############################################################################
add_test(NAME test_open_common_mock_drv
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi  ${CMAKE_BINARY_DIR} --gtest_filter="*MOCK*:-*event_drv_05*:-*nodrv*")

set_tests_properties(
  test_open_common_mock_drv
  PROPERTIES
  ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

############################################################################
## Add Coverage Analysis ###################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_TESTS AND GTEST_FOUND)
    # targetname     The name of original target from which coverage is created for
    # testrunner     The name of the target which runs the tests.
    set_target_for_coverage(opae-c ctest)

    add_dependencies(coverage_opae-c gtapi)
  endif(BUILD_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
