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
include_directories(
	${CMAKE_SOURCE_DIR}/ase/api/src
	${CMAKE_SOURCE_DIR}/libopae/plugins/xfpga
	${CMAKE_SOURCE_DIR}/tests
	${CMAKE_SOURCE_DIR}/tools/extra/libopae++
	${CMAKE_SOURCE_DIR}/tools/extra/c++utils
	)

if(BUILD_ASE_TESTS)
     include(test_config_ase)
endif()


###########################################################################
#### COMMON for GTAPI & GTASE ##############################################
############################################################################
configure_file(${PROJECT_SOURCE_DIR}/configuration.json
               ${CMAKE_BINARY_DIR}/tests/configuration.json
               COPYONLY)

############################################################################
## Build MOCK Driver   #####################################################
############################################################################

# build mock driver and unpack fake sysfs to /tmp folder
Build_MOCk_DRV()

############################################################################
## add_gtfilter macro  #####################################################
############################################################################
macro(add_gtfilter test_name gtfilter is_mock)
    add_test(NAME ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND $<TARGET_FILE:gtase> "--gtest_filter=${gtfilter}")
    if(${is_mock})
        set_tests_properties(
            ${test_name}
            PROPERTIES
            ENVIRONMENT "LD_PRELOAD=./lib/libmock.so")
    endif()
endmacro()

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
add_gtfilter(CxxErrors    "LibopaecppErrors*" True)
Exe_Tests("Mock_All" ${Test_Names})
Exe_tests("Any_Value" "Cxx*any*")

############################################################################
## pyopae tests ######## ###################################################
############################################################################
add_test(NAME pyopae
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE} -m nose2 test_pyopae)
set_tests_properties(
    pyopae
    PROPERTIES
    ENVIRONMENT
    "LD_PRELOAD=./lib/libmock.so;PYTHONPATH=./lib/python${OPAE_PYTHON_VERSION}")


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
