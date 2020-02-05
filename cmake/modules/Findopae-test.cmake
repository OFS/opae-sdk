#!/usr/bin/cmake -P
## Copyright(c) 2017-2020, Intel Corporation
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

find_path(opae_test
    NAMES framework/mock/test_system.cpp
    PATHS ${opae_test_framework}
          ${CMAKE_BINARY_DIR}/opae-test/src/opae-test
          ${CMAKE_SOURCE_DIR})

if(NOT ${opae_test})
    include(ExternalProject)
    ExternalProject_Add(opae-test
          GIT_REPOSITORY https://github.com/OPAE/opae-test.git
          PREFIX ${CMAKE_BINARY_DIR}/opae-test
          CMAKE_ARGS -DOPAE_INCLUDE_PATH=${OPAE_INCLUDE_PATH} -DOPAE_LIBS_ROOT=${OPAE_LIBS_ROOT}
          INSTALL_COMMAND ""
          BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target test_system
          COMMENT "adding opae-test"
    )
    set(OPAE_TEST_ROOT "${CMAKE_BINARY_DIR}/opae-test/src/opae-test")

    add_library(test_system IMPORTED SHARED GLOBAL)
    add_dependencies(test_system opae-test)

    add_library(fpga_db IMPORTED SHARED GLOBAL)
    add_dependencies(fpga_db opae-test)

    # Get source and binary directories from CMake project.
    ExternalProject_Get_Property(opae-test source_dir binary_dir)

    # Set libtest_system properties.
    set_target_properties(test_system PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/framework/libtest_system.so")
    set_target_properties(fpga_db PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/framework/libfpga_db.so")
else()
    set(OPAE_TEST_ROOT "${opae_test}")
endif()

set(OPAE_TEST_INCLUDE_DIRS ${OPAE_TEST_ROOT}/framework)
set(OPAE_TEST_LIBRARIES test_system fpga_db)

add_custom_command(TARGET opae-test
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-1socket-nlb0.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-1socket-nlb0-vf.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-dcp-rc-nlb3.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-dfl0-nlb0.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-dcp-vc.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-dfl0_patchset2-nlb0.tar.gz
        ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${OPAE_TEST_ROOT}/framework/mock_sys_tmp-dcp-rc-dfl0_patchset2-nlb0.tar.gz
        ${CMAKE_BINARY_DIR}
)

if(${opae_test_framework})
    set(opae_test_framework_FOUND true)
    message(FATAL_ERROR "TBD")
endif()

function(opae_load_gtest)
    message(STATUS "Trying to fetch gtest through git...")
    find_package(Git REQUIRED)

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

    set(gtest_root "${CMAKE_CURRENT_BINARY_DIR}/gtest/src/gtest/googletest")
    message(STATUS "gtest located at: ${gtest_root}")

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
endfunction()

function(opae_test_add)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_TEST_ADD "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${OPAE_TEST_ADD_TARGET} ${OPAE_TEST_ADD_SOURCE})

    set_target_properties(${OPAE_TEST_ADD_TARGET}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO)

    target_include_directories(${OPAE_TEST_ADD_TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${OPAE_LIBS_ROOT}/plugins/xfpga
            ${OPAE_LIBS_ROOT}/libopae-c
            ${OPAE_LIBS_ROOT}/libbitstream
            ${OPAE_TEST_INCLUDE_DIRS}
            ${GTEST_INCLUDE_DIRS})

    target_link_libraries(${OPAE_TEST_ADD_TARGET}
        ${CMAKE_THREAD_LIBS_INIT}
        safestr
        opae-c
        ${OPAE_TEST_LIBRARIES}
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
        ${GTEST_BOTH_LIBRARIES}
        ${OPAE_TEST_ADD_LIBS})

    add_dependencies(${OPAE_TEST_ADD_TARGET} opae-test)

    add_test(
        NAME ${OPAE_TEST_ADD_TARGET}
        COMMAND $<TARGET_FILE:${OPAE_TEST_ADD_TARGET}>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

function(opae_test_add_static_lib)
    set(options )
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_TEST_ADD_STATIC_LIB "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${OPAE_TEST_ADD_STATIC_LIB_TARGET} STATIC
        ${OPAE_TEST_ADD_STATIC_LIB_SOURCE})

    target_include_directories(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE ${OPAE_LIBS_ROOT}/plugins/xfpga
        PRIVATE ${OPAE_LIBS_ROOT}/libopae-c)

    set_target_properties(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        PROPERTIES
            COMPILE_FLAGS "-DSTATIC=''")

    target_link_libraries(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        ${OPAE_TEST_ADD_STATIC_LIB_LIBS})
endfunction()
