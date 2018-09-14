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
## POSSIBILITY OF SUCH DAMAGE

function (Build_GTEST)

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
  add_library(libgtest_main IMPORTED STATIC GLOBAL)
  add_dependencies(libgtest gtest)
  add_dependencies(libgtest_main gtest)

  # Get GTest source and binary directories from CMake project
  ExternalProject_Get_Property(gtest source_dir binary_dir)

  # Set libgtest properties
  set_target_properties(libgtest PROPERTIES
      "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
      "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
  # Set libgtest_main properties
  set_target_properties(libgtest_main PROPERTIES
      "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest_main.a"
      "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

  # Export gtest variables
  set(GTEST_ROOT ${gtest_root} PARENT_SCOPE)
  set(GTEST_INCLUDE_DIRS ${gtest_root}/include PARENT_SCOPE)
  set(GTEST_MAIN_LIBRARY libgtest_main PARENT_SCOPE)
  set(GTEST_LIBRARIES libgtest PARENT_SCOPE)
  set(GTEST_BOTH_LIBRARIES libgtest_main libgtest PARENT_SCOPE)
  set(GTEST_FOUND true PARENT_SCOPE)
  message(STATUS "gtest include dir: ${GTEST_INCLUDE_DIRS}")

endfunction(Build_GTEST)

function (Build_MOCK_DRV)
  # build mock driver fake directory structure
  add_custom_target(mock-sysfs-prepare)
  add_custom_command(TARGET mock-sysfs-prepare POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz /tmp
    COMMAND tar xzvf /tmp/mock_sys_tmp-1socket-nlb0.tar.gz -C /tmp --strip 1)

  # build mock driver
  add_library(mock SHARED EXCLUDE_FROM_ALL mock.c)
  target_include_directories(mock PUBLIC
    $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae/src>)
  add_dependencies(mock mock-sysfs-prepare)
  target_link_libraries(mock dl safestr)
endfunction(Build_MOCK_DRV)

function(Build_Test_Target Target_Name Target_LIB)

    include_directories(
                    ${OPAE_SDK_SOURCE}/tests
                    ${Boost_INCLUDE_DIRS}
                    ${libjson-c_INCLUDE_DIRS}
                    ${OPAE_SDK_SOURCE}/common/include
                    ${OPAE_SDK_SOURCE}/tools/extra/libopae++
                    ${OPAE_SDK_SOURCE}/tools/extra/c++utils)

    set(COMMON_SRC gtmain.cpp jsonParser.cpp
                unit/gtOpenClose_base.cpp
                unit/gtProperties_base.cpp
                unit/gtOpen.cpp
                unit/gtEnumerate.cpp
                unit/gtOptionParser.cpp
                unit/gtAnyValue.cpp
                unit/gtsysfs.cpp
                unit/gtUsrclk.cpp
                unit/gtBSMetadata.cpp
                unit/gtBuffer.cpp
                unit/gtReconf.cpp
                unit/gtMockErrInj.cpp
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
                function/gtError.cpp
                function/gtMMIO.cpp
                function/gtVersion.cpp
                function/gtOpenClose.cpp
		function/gtGetProperties.cpp
		function/gtCommon.cpp)

    if(BUILD_ASE_TEST)
        add_definitions(-DBUILD_ASE)
        set(LIB_SRC_PATH ${OPAE_SDK_SOURCE}/ase/api/src)
        set(TARGET_SRC ${COMMON_SRC})
    else()
        set(LIB_SRC_PATH ${OPAE_SDK_SOURCE}/libopae/src)
        set(TARGET_SRC ${COMMON_SRC}
            function/gtUmsg.cpp
            function/gtHostif.cpp
            function/gtEvent.cpp)
    endif()

    target_link_libraries(commonlib ${Target_LIB} ${GTEST_BOTH_LIBRARIES}
                          ${libjson-c_LIBRARIES} dl)
    target_include_directories(commonlib PUBLIC
                               $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
                               $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>
                               $<BUILD_INTERFACE:${LIB_SRC_PATH}>)

    add_executable(${Target_Name} ${TARGET_SRC})
    target_include_directories(${Target_Name} PUBLIC
                               $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
                               $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>
                               $<BUILD_INTERFACE:${LIB_SRC_PATH}>)
                      
    target_link_libraries(${Target_Name} commonlib safestr ${Target_LIB} ${libjson-c_LIBRARIES} 
                              uuid ${GTEST_BOTH_LIBRARIES} dl opae-c++-utils opae-c++ opae-cxx-core)
	  						
    if(CMAKE_THREAD_LIBS_INIT)
       target_link_libraries(${Target_Name} "${CMAKE_THREAD_LIBS_INIT}")
    endif()

    if(THREADS_HAVE_PTHREAD_ARG)
        target_compile_options(PUBLIC commonlib "-pthread")
        target_compile_options(PUBLIC ${Target_Name} "-pthread")
    endif()

endfunction(Build_Test_Target)

function(Exe_Tests Test_Name Test_To_Be_Exe)

   #Filter test list to preload ib/libmock.so
   string(FIND ${Test_To_Be_Exe} "MOCK" pos)
   string(FIND ${Test_To_Be_Exe} "ALL"  pos1)

   add_test(NAME ${Test_Name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND gtapi  ${CMAKE_BINARY_DIR} -v --gtest_filter=${Test_To_Be_Exe})

  if(${pos} OR ${pos1})
     set_tests_properties(
         ${Test_Name}
         PROPERTIES
         ENVIRONMENT "LD_PRELOAD=lib/libmock.so")
   endif()

endfunction(Exe_Tests)
