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

If(${GTest_FOUND})
  message(STATUS Found GTest)
else()
  Build_GTEST()
endif()

#############################################################################
## Build GTASE OR GTAPI  ####################################################
#############################################################################
if(BUILD_ASE_TESTS)
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
if (NOT BUILD_ASE_TESTS)
  Build_MOCk_DRV()
endif()

############################################################################
## Add testing targets #####################################################
############################################################################
set(gtest_filter_include)
set(gtest_filter_exclude)

# list(APPEND gtest_filter *MOCK*)

#inclusion pattern
list(APPEND gtest_filter_include *ALL.*)

#exclusion pattern
list(APPEND gtest_filter_exclude *event_drv_05*)
list(APPEND gtest_filter_exclude *Properties*._id.*)
list(APPEND gtest_filter_exclude *Open*.03)
list(APPEND gtest_filter_exclude *Open*.06)
list(APPEND gtest_filter_exclude *Open*.open_drv_09)
list(APPEND gtest_filter_exclude *Open*.07)
list(APPEND gtest_filter_exclude *Enum*.18)
list(APPEND gtest_filter_exclude *Enum*.19)
list(APPEND gtest_filter_exclude *Buf*.PrepRel2MB01)
list(APPEND gtest_filter_exclude *Buf*.Write01)
list(APPEND gtest_filter_exclude *Buf*.WriteRead01)
list(APPEND gtest_filter_exclude *Close*.03)

set(gtest_filter_include_str "")
foreach(filter ${gtest_filter_include})
  set(gtest_filter_include_str "${gtest_filter}:${filter}")
endforeach(filter ${gtest_filter_include})

set(gtest_filter_exclude_str "")
foreach(filter ${gtest_filter_exclude})
  set(gtest_filter_exclude_str "${gtest_filter}:${filter}")
endforeach(filter ${gtest_filter_exclude})

if(BUILD_ASE_TESTS)
  add_test(NAME test_open_common_ase
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND gtase  ${CMAKE_BINARY_DIR} -v --gtest_filter=${gtest_filter_include_str}:-${gtest_filter_exclude_str})

  set_tests_properties(
    test_open_common_ase
    PROPERTIES
    ENVIRONMENT "LD_PRELOAD=lib/libopae-c-ase.so;ASE_WORKDIR=${CMAKE_BINARY_DIR}/ase/samples/intg_xeon_nlb")

else()
  add_test(NAME test_open_common_mock_drv
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND gtapi  ${CMAKE_BINARY_DIR} -v --gtest_filter=*MOCK*:*ALL*:-*event_drv_05*:*nodrv*:*Open*.03:*Open*.06:*Open*.open_drv_09:*Open*.07:*Enum*.18:*Enum*.19:*Buf*.PrepRel2MB01:*Buf*.Write01:*Buf*.WriteRead01:*Close*.03)

  set_tests_properties(
    test_open_common_mock_drv
    PROPERTIES
    ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

  # FIXME: roll into more generic target
  add_test(NAME test_version
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND gtapi  ${CMAKE_BINARY_DIR} --gtest_filter=*LibopaecVersion*)

  set_tests_properties(
    test_version
    PROPERTIES
    ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

  add_test(NAME test_get_properties
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND gtapi  ${CMAKE_BINARY_DIR} --gtest_filter=*LibopaecGetProperties*)

  set_tests_properties(
    test_get_properties
    PROPERTIES
    ENVIRONMENT "LD_PRELOAD=lib/libmock.so")

  add_test(NAME test_any_value
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND gtapi  ${CMAKE_BINARY_DIR} --gtest_filter=Cxx*any*)
endif()

############################################################################
## Add Coverage Analysis ###################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  if(BUILD_ASE_TESTS AND GTEST_FOUND)
      set_target_for_coverage_local(opae-c-ase ctest)
      add_dependencies(coverage_opae-c-ase gtase)
  else(BUILD_ASE_TESTS AND GTEST_FOUND)
    if(BUILD_TESTS AND GTEST_FOUND)
      set_target_for_coverage(opae-c ctest)
      add_dependencies(coverage_opae-c gtapi)
    endif(BUILD_TESTS AND GTEST_FOUND)
  endif(BUILD_ASE_TESTS AND GTEST_FOUND)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
