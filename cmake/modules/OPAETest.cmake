#!/usr/bin/cmake -P
## Copyright(c) 2017-2022, Intel Corporation
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

cmake_minimum_required(VERSION 3.11)

check_cxx_compiler_flag("-Wno-sign-compare" CXX_SUPPORTS_NO_SIGN_COMPARE)

set(OPAE_TEST_LIBRARIES test_system fpga_db
    CACHE INTERNAL "OPAE test libs." FORCE)

function(opae_load_gtest)
    FetchContent_Declare(gtest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG release-${GTEST_VERSION}
    )

    FetchContent_MakeAvailable(gtest)

    set(gtest_ROOT ${FETCHCONTENT_BASE_DIR}/gtest-src CACHE PATH "gtest root directory" FORCE)

    set(GTEST_INCLUDE_DIR ${gtest_ROOT}/googletest/include CACHE PATH "gtest include directory" FORCE)
    set(GTEST_LIBRARY ${LIBRARY_OUTPUT_PATH}/libgtest.a CACHE PATH "path to gtest library" FORCE)
    set(GTEST_MAIN_LIBRARY ${LIBRARY_OUTPUT_PATH}/libgtest_main.a CACHE PATH "path to gtest main library" FORCE)
    set(GTEST_LIBRARY_DEBUG ${LIBRARY_OUTPUT_PATH}/libgtestd.a CACHE PATH "path to (debug) gtest library" FORCE)
    set(GTEST_MAIN_LIBRARY_DEBUG ${LIBRARY_OUTPUT_PATH}/libgtest_maind.a CACHE PATH "path to (debug) gtest main library" FORCE)
endfunction()

function(opae_test_add)
    set(options TEST_FPGAD)
    set(oneValueArgs TARGET)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_TEST_ADD "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(OPAE_ENABLE_MOCK)
        set(MOCK_CPP ${opae-test_ROOT}/framework/mock/opae_mock.cpp)
    else()
        set(MOCK_CPP ${opae-test_ROOT}/framework/mock/opae_std.c)
    endif()

    add_executable(${OPAE_TEST_ADD_TARGET}
        ${OPAE_TEST_ADD_SOURCE} ${MOCK_CPP})

    set_target_properties(${OPAE_TEST_ADD_TARGET}
        PROPERTIES
            CXX_STANDARD 11
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
            ENABLE_EXPORTS ON)
    target_compile_definitions(${OPAE_TEST_ADD_TARGET}
        PRIVATE
            HAVE_CONFIG_H=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_TEST_ADD_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()
    if(CXX_SUPPORTS_NO_SIGN_COMPARE)
        target_compile_options(${OPAE_TEST_ADD_TARGET}
            PRIVATE -Wno-sign-compare)
    endif()

    target_include_directories(${OPAE_TEST_ADD_TARGET}
        PUBLIC
            $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${OPAE_LIB_SOURCE}
	    ${OPAE_LIB_SOURCE}/plugins/xfpga
	    ${OPAE_LIB_SOURCE}/libopae-c
            ${opae-test_ROOT}/framework
            ${GTEST_INCLUDE_DIR})

    if(${OPAE_TEST_ADD_TEST_FPGAD})
        target_include_directories(${OPAE_TEST_ADD_TARGET}
            PRIVATE
                ${opae-test_ROOT}/framework/mock/test_fpgad)
    endif(${OPAE_TEST_ADD_TEST_FPGAD})

    target_link_libraries(${OPAE_TEST_ADD_TARGET}
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPAE_TEST_LIBRARIES}
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
        ${GTEST_LIBRARIES}
        ${OPAE_TEST_ADD_LIBS})

    opae_coverage_build(TARGET ${OPAE_TEST_ADD_TARGET}
        SOURCE ${OPAE_TEST_ADD_SOURCE})

    if (OPAE_GTEST_OUTPUT)
        set(test_args
            "--gtest_output=xml:${OPAE_GTEST_OUTPUT}/${OPAE_TEST_ADD_TARGET}.xml")
    endif (OPAE_GTEST_OUTPUT)

    add_test(
        NAME ${OPAE_TEST_ADD_TARGET}
        COMMAND $<TARGET_FILE:${OPAE_TEST_ADD_TARGET}> ${test_args}
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
        PRIVATE
            ${OPAE_LIB_SOURCE}
	    ${OPAE_LIB_SOURCE}/plugins/xfpga
	    ${OPAE_LIB_SOURCE}/libopae-c
            ${opae-test_ROOT}/framework
    )

    set_property(TARGET ${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        PROPERTY
            POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        PRIVATE
            HAVE_CONFIG_H=1
            PIC=1
            STATIC=)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()

    target_link_libraries(${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        ${OPAE_TEST_ADD_STATIC_LIB_LIBS})

    opae_coverage_build(TARGET ${OPAE_TEST_ADD_STATIC_LIB_TARGET}
        SOURCE ${OPAE_TEST_ADD_STATIC_LIB_SOURCE})
endfunction()
