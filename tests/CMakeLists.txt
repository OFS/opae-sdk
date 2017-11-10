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

find_package(Doxygen
  REQUIRED dot
  OPTIONAL_COMPONENTS mscgen dia)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(docs
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating test lists and descriptions with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

# Bootstrap gtest
if (NOT DEFINED gtest_root)
  message(STATUS "Trying to fetch gtest throught git")
  find_package(Git)
  execute_process(COMMAND "${GIT_EXECUTABLE}"
    "clone"
    "--branch"
    "release-1.8.0"
    "https://github.com/google/googletest.git"
    "gtest" WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set (gtest_root "${CMAKE_BINARY_DIR}/gtest")
endif()
message(STATUS "gtest is in ${gtest_root}")
if (NOT TARGET ${gtest_root})
  add_subdirectory(${gtest_root} ${CMAKE_BINARY_DIR}/gtest)
  set_property(TARGET gtest_main PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET gtest PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# Export gtest variables; do not use find gtest!
# Find gtest breaks building on travis CI
set(GTEST_ROOT ${gtest_root} PARENT_SCOPE)
set(GTEST_INCLUDE_DIRS ${gtest_root}/include PARENT_SCOPE)
set(GTEST_MAIN_LIBRARIES gtest_main PARENT_SCOPE)
set(GTEST_MAIN_LIBRARY gtest_main PARENT_SCOPE)
set(GTEST_LIBRARY gtest_main PARENT_SCOPE)
set(GTEST_BOTH_LIBRARIES gtest gtest_main)
set(GTEST_FOUND true)

# Include directories
include_directories(
  ${GTEST_INCLUDE_DIRS}
  ${OPAE_INCLUDE_DIR}
  ${OPAE_SDK_SOURCE}/libopae/src
  ${OPAE_SDK_SOURCE}/tools/libopae++
  ${OPAE_SDK_SOURCE}/tools/c++utils
  ${Boost_INCLUDE_DIRS}
  ${OPAE_SDK_SOURCE}/common/include
  ${OPAE_SDK_SOURCE}/tests)

message("jsoncpp=${jsoncpp_INCLUDE_DIRS}")
add_library(commonlib SHARED common_test.h common_test.cpp)
target_link_libraries(commonlib opae-c ${GTEST_BOTH_LIBRARIES} ${jsoncpp_LIBRARIES})
target_include_directories(commonlib PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${jsoncpp_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)

add_executable(foapp
  fpgaopen_app.cpp)
target_include_directories(foapp PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)
target_link_libraries(foapp commonlib safestr opae-c uuid ${GTEST_BOTH_LIBRARIES})

set(SRC gtmain.cpp
  jsonParser.cpp
  unit/gtProperties_base.cpp
  unit/gtOpenClose_base.cpp
  unit/gtEnumerate.cpp
  unit/gtOpen.cpp
  unit/gtOptionParser.cpp
  function/gtEnumerate.cpp
  function/gtHostif.cpp
  function/gtOpenClose.cpp
  function/gtReconf.cpp
  function/gtUmsg.cpp
  function/gtBuffer.cpp
  function/gtEvent.cpp
  function/gtMMIO.cpp
  function/gtReset.cpp
  function/gtMMLink.cpp
  stress/gtBuf_stress.cpp
  stress/gtMMIO_stress.cpp
  stress/gtOpen_stress.cpp
  stress/gtPR_stress.cpp
  stress/gtPerfCounters_stress.cpp
  stress/gtRASPower_stress.cpp)
add_executable(gtapi ${SRC})
target_include_directories(gtapi PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>
  PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)
target_link_libraries(gtapi commonlib safestr opae-c json-c uuid ${GTEST_BOTH_LIBRARIES}
  opae-c++-utils opae-c++)
configure_file(${PROJECT_SOURCE_DIR}/configuration.json
  ${CMAKE_BINARY_DIR}/tests/configuration.json
  COPYONLY)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC commonlib "-pthread")
  target_compile_options(PUBLIC foapp "-pthread")
  target_compile_options(PUBLIC gtapi "-pthread")
endif()

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(foapp "${CMAKE_THREAD_LIBS_INIT}")
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

# Add discrete tests
add_subdirectory(dcp)

############################################################################
## Add testing targets #####################################################
############################################################################

add_test(NAME gtapi_enum
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi -j ${CMAKE_BINARY_DIR}/tests/configuration.json --gtest_filter=*EnumCommon*enum*)

add_test(NAME gtapi_mmio
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi -j ${CMAKE_BINARY_DIR}/tests/configuration.json --gtest_filter=*MmioCommon*mmio*)

add_test(NAME gtapi_open
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi -j ${CMAKE_BINARY_DIR}/tests/configuration.json --gtest_filter=*OpenCommon*open*)

set_tests_properties(
  gtapi_enum
  gtapi_open
  gtapi_mmio
  PROPERTIES
  ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

############################################################################
## Add 'coverage' ##########################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_TESTS AND GTEST_FOUND)
    set_target_for_coverage(opae-c ctest)
    add_dependencies(coverage_opae-c gtapi)
  endif(BUILD_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
