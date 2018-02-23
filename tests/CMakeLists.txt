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

option(BUILD_ASE_TEST "Build GTASE." OFF)
mark_as_advanced(BUILD_ASE_TEST)

##############################################################################
## Setup GTEST ###############################################################
##############################################################################
include(test_config)
find_package(GTest)

If(${GTest_FOUND})
  message(STATUS Found GTest)
else()
  Build_GTEST()
endif()

#############################################################################
## Build GTASE OR GTAPI  ####################################################
#############################################################################
if(BUILD_ASE_TEST)
     Build_Test_Target(gtase opae-c-ase)
else()
    Build_Test_Target(gtapi opae-c)
endif()

############################################################################
#### COMMON for GTAPI & GTASE ##############################################
############################################################################
configure_file(${PROJECT_SOURCE_DIR}/configuration.json
               ${CMAKE_BINARY_DIR}/tests/configuration.json
               COPYONLY)

############################################################################
## Build MOCK Driver   #####################################################
############################################################################
# build mock driver and unpack fake sysfs to /tmp folder
if (NOT BUILD_ASE_TEST)
  Build_MOCk_DRV()
endif()

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

add_test(NAME test_any_value
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND gtapi  ${CMAKE_BINARY_DIR} --gtest_filter=Cxx*any*)


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
