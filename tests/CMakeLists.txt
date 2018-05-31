## Copyright(c) 2014-2018, Intel Corporation
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

add_library(commonlib SHARED common_test.h common_test.cpp)
if(BUILD_ASE_TESTS)
  Build_Test_Target(gtase opae-c-ase)
endif()
Build_Test_Target(gtapi opae-c)

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
Build_Mock_DRV()

############################################################################
## add_gtfilter macro  #####################################################
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

############################################################################
## ASE compatible version of gtapi (gtase)  ################################
############################################################################

set(gtest_filter_include)
set(gtest_filter_exclude)

# inclusion pattern
list(APPEND gtest_filter_include *ALL.*)

# exclusion pattern
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
  add_test(NAME ase_all
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMMAND gtase -v --gtest_filter=${gtest_filter_include_str}:-${gtest_filter_exclude_str})

  set_tests_properties(ase_all
    PROPERTIES
    ENVIRONMENT "ASE_WORKDIR=${CMAKE_BINARY_DIR}/samples/intg_xeon_nlb/hw")

  set_property(TEST ase_all
    APPEND
    PROPERTY
    ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1")

  set_property(TEST ase_all
    APPEND
    PROPERTY
    ENVIRONMENT "LD_PRELOAD=${CMAKE_BINARY_DIR}/lib/libopae-c-ase.so")
endif()

############################################################################
## Add testing targets #####################################################
############################################################################
#  Exe_Tests (A B) Usage:
#  A         test group name
#  B         regular expression of test name)
############################################################################
set(Test_Names "*MOCK*:*ALL*")
set(Ignored_Tests_1 ":-*event_drv_05*:*nodrv*:*Open*.03:*Open*.06:*Open*.open_drv_09:*Open*.07:*Enum*.18")
set(Ignored_Tests_2 ":*Enum*.19:*Buf*.PrepRel2MB01:*Buf*.Write01:*Buf*.WriteRead01:*Close*.03")
set(Test_Names "${Test_Names}${Ignored_Tests_1}${Ignored_Tests_2}")

add_gtfilter(CxxProperties "LibopaecppProps*" False)
add_gtfilter(CxxEnum "LibopaecppEnum*" True)
add_gtfilter(CxxOpenClose "LibopaecppOpen*" True)
add_gtfilter(CxxBuffer    "LibopaecppBuffer*" True)
add_gtfilter(CxxExcept    "LibopaecppExcept*" False)
add_gtfilter(CxxEvent     "LibopaecppEvent*" True)
add_gtfilter(CxxMMIO      "LibopaecppMMIO*" True)
add_gtfilter(CxxVersion   "LibopaecppVersion*" False)
Exe_Tests("Mock_All" ${Test_Names})
Exe_tests("Any_Value" "Cxx*any*")

############################################################################
## Add Coverage Analysis ###################################################
############################################################################

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_target_for_coverage(opae-c ctest)
  add_dependencies(coverage_opae-c gtapi)
  if(BUILD_ASE_TESTS)
    set_target_for_coverage_local(opae-c-ase
      TESTRUNNER ctest
      TESTRUNNER_ARGS "-R;ase_all"
      COVERAGE_EXTRA_COMPONENTS "opae-c-ase-server-intg_xeon_nlb")
    add_dependencies(coverage_opae-c-ase gtase)
  endif()
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
