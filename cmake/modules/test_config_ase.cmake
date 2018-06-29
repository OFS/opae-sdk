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
## POSSIBILITY OF SUCH DAMAGE
include(coverage)
add_library(commonlib-ase SHARED common_test.h common_test.cpp)
set(COMMON_SRC gtmain.cpp jsonParser.cpp
  unit/gtOpenClose_base.cpp
  unit/gtProperties_base.cpp
  unit/gtOpen.cpp
  unit/gtEnumerate.cpp
  unit/gtOptionParser.cpp
  unit/gtAnyValue.cpp
  unit/gtsysfs.cpp
  function/gtCxxEnumerate.cpp
  function/gtCxxEvents.cpp
  function/gtCxxOpenClose.cpp
  function/gtCxxProperties.cpp
  function/gtCxxExcept.cpp
  function/gtCxxBuffer.cpp
  function/gtCxxReset.cpp
  function/gtCxxMMIO.cpp
  function/gtCxxVersion.cpp
  function/gtCxxErrors.cpp
  function/gtReset.cpp
  function/gtBuffer.cpp
  function/gtEnumerate.cpp
  # function/gtError.cpp
  function/gtMMIO.cpp
  function/gtVersion.cpp
  function/gtOpenClose.cpp
  function/gtGetProperties.cpp)
set(TARGET_SRC_ASE ${COMMON_SRC})
add_executable(gtase ${TARGET_SRC_ASE})

set(LIB_SRC_PATH_ASE ${OPAE_SDK_SOURCE}/ase/api/src)
target_compile_definitions(commonlib-ase PUBLIC BUILD_ASE)

target_link_libraries(commonlib-ase ${Target_LIB} ${GTEST_BOTH_LIBRARIES}
  ${libjson-c_LIBRARIES})
target_include_directories(commonlib-ase PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${LIB_SRC_PATH_ASE}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)

target_compile_definitions(gtase PRIVATE BUILD_ASE)
target_include_directories(gtase PUBLIC
  $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
  $<BUILD_INTERFACE:${LIB_SRC_PATH_ASE}>
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(gtase commonlib-ase safestr dl opae-c-ase ${libjson-c_LIBRARIES}
  uuid ${GTEST_BOTH_LIBRARIES} opae-c++-utils opae-c++ opae-cxx-core)

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
list(APPEND gtest_filter_exclude *cpp*except_03)

set(gtest_filter_include_str "")
foreach(filter ${gtest_filter_include})
  set(gtest_filter_include_str "${gtest_filter}:${filter}")
endforeach(filter ${gtest_filter_include})

set(gtest_filter_exclude_str "")
foreach(filter ${gtest_filter_exclude})
  set(gtest_filter_exclude_str "${gtest_filter}:${filter}")
endforeach(filter ${gtest_filter_exclude})

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

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  set_target_for_coverage_local(opae-c-ase
    TESTRUNNER ctest
    TESTRUNNER_ARGS "-R;ase_all"
    COVERAGE_EXTRA_COMPONENTS "opae-c-ase-server-intg_xeon_nlb")
  add_dependencies(coverage_opae-c-ase gtase)
endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
